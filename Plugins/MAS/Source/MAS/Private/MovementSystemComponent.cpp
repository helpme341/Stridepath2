
#include "MovementSystemComponent.h"

#include "Utility/TraceUtility.h"

UMovementSystemComponent::UMovementSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMovementSystemComponent::BeginPlay()
{
	Super::BeginPlay();
	DynamicSet = DefaultSet;
	
	check(GetOwner())
	SetUpdatedComponent(GetOwner()->GetRootComponent());
	CapsuleComp = Cast<UCapsuleComponent>(UpdatedComponent);
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
	UpdatedComponent->ComponentVelocity = DynamicSet.Velocity;
}

bool UMovementSystemComponent::PredictiveGround(FHitResult& OutHit, const float DeltaTime) const
{
	const FVector H = FVector(DynamicSet.Velocity.X, DynamicSet.Velocity.Y, 0.f);
	float HSpeed = H.Size();
	if (HSpeed <= KINDA_SMALL_NUMBER) return false;

	const FVector Dir = H / HSpeed;
	const float Ahead = FMath::Clamp(HSpeed * DeltaTime * 1.5f, 6.f, DynamicSet.MaxPredictAhead);

	const FVector Start = UpdatedComponent->GetComponentLocation() + Dir * Ahead;
	const FVector End   = Start + FVector(0,0, -(DynamicSet.GroundProbeLength + 6.f));

	if (!SweepSingleByChannel(OutHit, Start, End) || !IsWalkable(OutHit.Normal)) return false;

	const float ComputeSnapDistance = DynamicSet.BaseSnapDistance + FMath::Clamp(HSpeed * DeltaTime * 1.5f, 0.f, 30.f);
	return OutHit.Distance <= ComputeSnapDistance + 2.f; // допускаем небольшой зазор, растущий со скоростью/Δt
}

void UMovementSystemComponent::PrePhysics_UpdateGroundState()
{
	const float DT = GetWorld()->GetDeltaSeconds();
	SweepForGround(GroundHit); // Длинный зонд оставляем для проекции ввода по уклону
	
	FHitResult ContactHit; // Контакт
	const bool bContact = ShortGroundContact(ContactHit);

	FHitResult PredHit; 
	const bool bPredict = PredictiveGround(PredHit, DT); // Предиктивная опора впереди

	if (bContact) DynamicSet.TimeSinceLostGround = 0.f; // Койот-тайм
	else          DynamicSet.TimeSinceLostGround += DT;
	const bool bGrace = DynamicSet.TimeSinceLostGround <= DynamicSet.GroundGraceTime && DynamicSet.Velocity.Z <= 150.f;

	bIsOnGround = bContact || bPredict || bGrace;

	// Нормаль для проекции ввода — контактная приоритетнее, потом предиктивная
	if      (bContact) GroundHit = ContactHit;
	else if (bPredict) GroundHit = PredHit;

	// Прижим по Z — ТОЛЬКО при реальном контакте
	if (bIsOnGround && DynamicSet.Velocity.Z < 0.f) DynamicSet.Velocity.Z = FMath::Max(DynamicSet.Velocity.Z, DynamicSet.StickVelocityZ);
}

bool UMovementSystemComponent::ShortGroundContact(FHitResult& OutHit) const
{
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FVector End   = Start + FVector(0,0,-(DynamicSet.GroundContactTolerance + 0.5f));
	
	return SweepSingleByChannel(OutHit, Start, End) && IsWalkable(OutHit.Normal);
}

void UMovementSystemComponent::ApplyInputAcceleration(const float DeltaTime)
{
	if (const FVector Input = ConsumeInputVector(); !Input.IsNearlyZero())
	{
		const FVector InputDir = Input.GetSafeNormal();
		const bool bGround = bIsOnGround;

		const float TargetAccel = bGround ? DynamicSet.GroundAcceleration : DynamicSet.AirAcceleration;
		const float MaxSpeed = bGround ? DynamicSet.MaxGroundSpeed : DynamicSet.MaxAirSpeed;

		FVector DesiredDir = InputDir; 	// Проекция на плоскость земли (движение по склонам)
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

void UMovementSystemComponent::ApplyFrictionAndBraking(const float DeltaTime)
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
	// Прямая гравитация
	DynamicSet.Velocity.Z += DynamicSet.GravityZ * -1000.f * DeltaTime; // умножение тут на -1000.f нужно, что бы было удобно настаивать не по 1000 а по 1 2 3….
}

void UMovementSystemComponent::MoveWithCollisions(const float DeltaTime)
{
    const FVector Delta = DynamicSet.Velocity * DeltaTime;

    FHitResult Hit;
    SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentRotation(), true, Hit);

    if (Hit.IsValidBlockingHit())
    {
        SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit);

        // Вертикальная стена → проекция горизонтальной скорости
        if (Hit.Normal.Z < 0.2f)
        {
            const FVector HVel   = FVector(DynamicSet.Velocity.X, DynamicSet.Velocity.Y, 0.f);
            const FVector HSlide = FVector::VectorPlaneProject(HVel, Hit.Normal);
            DynamicSet.Velocity.X = HSlide.X;
            DynamicSet.Velocity.Y = HSlide.Y;
        }

        // Удар о поверхность снизу — гасим отрицательный Z
        if (DynamicSet.Velocity.Z < 0.f && Hit.Normal.Z > 0.2f) DynamicSet.Velocity.Z = 0.f;
    }
}

bool UMovementSystemComponent::SweepSingleByChannel(FHitResult& OutHit, const FVector& Start, const FVector& End) const 
{
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(PredictGround), false, GetOwner());
	const FCollisionShape Shape(FCollisionShape::MakeCapsule(GetCapsuleRadius(CapsuleComp.Get()), GetCapsuleHalfHeight(CapsuleComp.Get())));
	
	const bool bHit = GetWorld()->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, ECC_WorldStatic, Shape, Params);
	FTraceUtility::DrawDebugSweep(GetWorld(), Start, End, Shape, bHit);
	return bHit;
}

bool UMovementSystemComponent::IsWalkable(const FVector& Normal) const
{
	const float CosWalkable = FMath::Cos(FMath::DegreesToRadians(DynamicSet.MaxSlopeAngleDeg));
	return FVector::DotProduct(Normal, FVector::UpVector) >= CosWalkable;
}

bool UMovementSystemComponent::SweepForGround(FHitResult& OutHit) const
{
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const float Speed     = DynamicSet.Velocity.Size();
	const float Ahead     = FMath::Clamp(Speed * GetWorld()->GetDeltaSeconds() * 1.5f, 0.f, 120.f);
	constexpr float StepSlack = 6.f;
	const float ProbeDown = DynamicSet.GroundProbeLength + Ahead + StepSlack;

	const FVector End = Start + FVector(0, 0, -ProbeDown);
	if (!SweepSingleByChannel(OutHit, Start, End)) return false;

	const float SlopeDeg = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(OutHit.Normal, FVector::UpVector)));
	return SlopeDeg <= DynamicSet.MaxSlopeAngleDeg;
}