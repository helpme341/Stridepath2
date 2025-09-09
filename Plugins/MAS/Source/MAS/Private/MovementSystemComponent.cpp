
#include "MovementSystemComponent.h"

UMovementSystemComponent::UMovementSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	DynamicSet = DefaultSet;
}

void UMovementSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (ShouldSkipUpdate(DeltaTime)) return;

	ApplyMovementEdits();

	PrePhysics_UpdateGroundState();
	
	ApplyInputAcceleration(DeltaTime);
	ApplyFrictionAndBraking(DeltaTime);
	ApplyGravity(DeltaTime);
	
	MoveWithCollisions(DeltaTime);
	SetOwnerCompVelocity();
}

void UMovementSystemComponent::SetOwnerCompVelocity()
{
	if (UpdatedComponent)
	{
		const auto CurrentVelocity = DynamicSet.Velocity * DynamicSet.PlaneConstraintNormal;
		DynamicSet.Velocity = CurrentVelocity;
		UpdatedComponent->ComponentVelocity = CurrentVelocity;
	}
}

void UMovementSystemComponent::SetIsOnGround(const bool bInIsOnGround)
{
	bIsOnGround = bInIsOnGround;
}

void UMovementSystemComponent::PrePhysics_UpdateGroundState()
{
	SetIsOnGround(SweepForGround(GroundHit));

	// Если стоим на поверхности и заходили в неё по Z — обнуляем вертикальную «просадку»
	if (bIsOnGround && DynamicSet.Velocity.Z < 0.f)
	{
		// Небольшой «прилип» к земле вместо отрицательного Z
		DynamicSet.Velocity.Z = FMath::Max(DynamicSet.Velocity.Z, -2.f); // Небольшой «прилип» к земле вместо отрицательного Z
	}
}

void UMovementSystemComponent::ApplyInputAcceleration(const float DeltaTime)
{
	if (const FVector Input = ConsumeInputVector(); !Input.IsNearlyZero())
	{
		const FVector InputDir = Input.GetSafeNormal();
		const bool bGround = bIsOnGround;

		const float TargetAccel = bGround ? DynamicSet.GroundAcceleration : DynamicSet.AirAcceleration;
		const float MaxSpeed = bGround ? DynamicSet.MaxGroundSpeed : DynamicSet.MaxAirSpeed;

		// Проекция на плоскость земли (движение по склонам)
		FVector DesiredDir = InputDir;
		if (bGround && GroundHit.IsValidBlockingHit())
		{
			const FVector Normal = GroundHit.Normal;
			DesiredDir = FVector::VectorPlaneProject(InputDir, Normal).GetSafeNormal();
		}

		// Ускорение
		DynamicSet.Velocity += DesiredDir * TargetAccel * DeltaTime;

		// Ограничение горизонтальной скорости
		const FVector Horizontal = FVector(DynamicSet.Velocity.X, DynamicSet.Velocity.Y, 0.f);
		const float HSpeed = Horizontal.Size();
		const float HMax = MaxSpeed;

		if (HSpeed > HMax && HSpeed > KINDA_SMALL_NUMBER)
		{
			const FVector ClampedH = Horizontal * (HMax / HSpeed);
			DynamicSet.Velocity.X = ClampedH.X;
			DynamicSet.Velocity.Y = ClampedH.Y;
		}
	}
}

void UMovementSystemComponent::ApplyFrictionAndBraking(float DeltaTime)
{
	if (!bIsOnGround) return;

	// Горизонтальная часть
	FVector H = FVector(DynamicSet.Velocity.X, DynamicSet.Velocity.Y, 0.f);

	// Если нет входа (после ConsumeInputVector) — тормозим и трёмся
	const float Friction = DynamicSet.GroundFriction;
	const float Braking  = DynamicSet.BrakingDeceleration;

	const float Speed = H.Size();
	if (Speed <= KINDA_SMALL_NUMBER) return;

	// Эквивалент CharacterMovement: dV ~ (Friction*Speed + Braking) * dt
	const float Drop = (Friction * Speed + Braking) * DeltaTime;
	const float NewSpeed = FMath::Max(Speed - Drop, 0.f);
	
	if (NewSpeed != Speed)
	{
		H *= NewSpeed / Speed;
		DynamicSet.Velocity.X = H.X;
		DynamicSet.Velocity.Y = H.Y;
	}
}

void UMovementSystemComponent::ApplyGravity(const float DeltaTime)
{
	// Прямая гравитация; если на земле — минимальный «прилип» уже установлен в PrePhysics
	DynamicSet.Velocity.Z += DynamicSet.GravityZ * DeltaTime;
}

void UMovementSystemComponent::MoveWithCollisions(const float DeltaTime)
{
	const FVector Delta = DynamicSet.Velocity * DeltaTime;

	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentRotation(), true, Hit);

	if (Hit.IsValidBlockingHit())
	{
		// Слайдимся по поверхности
		SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit);

		// Если лбом в вертикальную стену — обнуляем соответствующую проекцию горизонтали
		if (Hit.Normal.Z < 0.2f)	
		{
			const FVector HVel = FVector(DynamicSet.Velocity.X, DynamicSet.Velocity.Y, 0.f);
			const FVector HSlide = FVector::VectorPlaneProject(HVel, Hit.Normal);
			DynamicSet.Velocity.X = HSlide.X;
			DynamicSet.Velocity.Y = HSlide.Y;
		}

		// Ступеньки/бордюры — упрощённо: если удар снизу, обнуляем отрицательный Z
		if (DynamicSet.Velocity.Z < 0.f && Hit.Normal.Z > 0.2f) DynamicSet.Velocity.Z = 0.f;
	}

	// После движения — перепроверяем «землю» (могли слететь с уступа)
    SetIsOnGround(SweepForGround(GroundHit));
}

bool UMovementSystemComponent::SweepForGround(FHitResult& OutHit) const
{
	auto CapsuleComponent = Cast<UCapsuleComponent>(UpdatedComponent);
	
	const float Radius = GetCapsuleRadius(CapsuleComponent);
	const float HalfHeight = GetCapsuleHalfHeight(CapsuleComponent);

	const FVector Start = UpdatedComponent->GetComponentLocation();
	const float ProbeDown = DynamicSet.GroundProbeLength;
	const FVector End = Start + FVector(0,0, -ProbeDown);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(SweepForGround), false, GetOwner());
	FCollisionResponseParams Response;
	FCollisionShape Shape;

	// Пытаемся свипать капсулой, если рут — капсула; иначе сферой.
	if (CapsuleComponent) Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
	else Shape = FCollisionShape::MakeSphere(FMath::Max(10.f, Radius));

	if (!GetWorld()->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, ECC_Visibility, Shape, Params, Response)) return false;

	// Проверяем проходимость по уклону
	const float SlopeDeg = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(OutHit.Normal, FVector::UpVector)));
	return SlopeDeg <= DynamicSet.MaxSlopeAngleDeg;
}