
#include "MovementSystemComponent.h"

#include "Settings/MovementSystemSettings.h"
#include "Utility/TraceUtility.h"

UMovementSystemComponent::UMovementSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMovementSystemComponent::BeginPlay()
{
	Super::BeginPlay();
	check(MovementSettings)
	DynamicSet = MovementSettings->DefaultSettings;
	
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
	if (LastInputDir = ConsumeInputVector(); !LastInputDir.IsNearlyZero())
	{
		const bool bGround = bIsOnGround;
		const float MaxSpeed  = bGround ? DynamicSet.MaxGroundSpeed : DynamicSet.MaxAirSpeed;
		const float TargetAccel = bGround ? DynamicSet.GroundAcceleration : DynamicSet.AirAcceleration;
	
		FVector InputDir = LastInputDir.GetSafeNormal();
		if (bGround && GroundHit.IsValidBlockingHit()) InputDir = FVector::VectorPlaneProject(InputDir, GroundHit.Normal).GetSafeNormal();

		FVector NewH = FVector(DynamicSet.Velocity.X, DynamicSet.Velocity.Y, 0.f);
		const float SpeedFrac = FMath::Clamp(NewH.Size() / MaxSpeed, 0.f, 1.f);
		const float AccelScale = DynamicSet.GroundAccelerationCurve->GetFloatValue(SpeedFrac);

		// 5) Применить ускорение
		const FVector Accel = InputDir * TargetAccel * AccelScale;
		DynamicSet.Velocity += Accel * DeltaTime;

		// 6) Кламп горизонтальной составляющей под MaxSpeed
		NewH = FVector(DynamicSet.Velocity.X, DynamicSet.Velocity.Y, 0.f);
		const float   NewHSpeed = NewH.Size();
		if (NewHSpeed > MaxSpeed && NewHSpeed > KINDA_SMALL_NUMBER)
		{
			const FVector ClampedH = NewH * (MaxSpeed / NewHSpeed);
			DynamicSet.Velocity.X = ClampedH.X;
			DynamicSet.Velocity.Y = ClampedH.Y;
		}
	}
}

void UMovementSystemComponent::ApplyFrictionAndBraking(const float DeltaTime)
{
    if (!bIsOnGround) return;

    // Горизонтальная скорость
    FVector H(DynamicSet.Velocity.X, DynamicSet.Velocity.Y, 0.f);
    float Speed = H.Size();
    if (Speed <= KINDA_SMALL_NUMBER) return;

    // Есть ли ввод (используй тот же InputDir, что и в ApplyInputAcceleration)
    // Сохрани последний нормализованный InputDir где-то в компоненте при обработке ввода.
    const bool bHasInput = !LastInputDir.IsNearlyZero();

    if (!bHasInput)
    {
        // Тормозим к нулю БЕЗ капа в присутствии ввода
        const float Drop = DynamicSet.BrakingDeceleration * DeltaTime;
        const float NewSpeed = FMath::Max(Speed - Drop, 0.f);

        if (NewSpeed <= DynamicSet.StopSpeedEpsilon)
        {
            H = FVector::ZeroVector;
        }
        else
        {
            H *= (NewSpeed / Speed);
        }
    }
    else
    {
        // Разложим скорость на вдоль/поперёк желаемого направления
        const FVector Dir = LastInputDir; // уже нормализованный (и спроецированный по склону раньше)
        const float   AlongMag = FVector::DotProduct(H, Dir);
        const FVector Along    = Dir * AlongMag;
        const FVector Side     = H - Along;

        // НЕ трогаем составляющую вдоль ввода (чтобы не было терминальной скорости от трения)
        // Лёгкий дамп только боковой части — убирает «занос»
        const float SideSpeed = Side.Size();
        if (SideSpeed > KINDA_SMALL_NUMBER)
        {
            const float SideDrop   = DynamicSet.LateralFriction * DeltaTime;
            const float NewSideMag = FMath::Max(SideSpeed - SideDrop, 0.f);
            const FVector NewSide  = Side * (NewSideMag / SideSpeed);
            H = Along + NewSide;
        }

        // Если игрок давит, но текущая скорость горизонтально идёт ПРОТИВ ввода — разрешим тормозить «встречную»
        if (AlongMag < 0.f)
        {
            const float DropOpp = DynamicSet.BrakingDeceleration * DeltaTime;
            const float NewSpeed = FMath::Max(H.Size() - DropOpp, 0.f);
            if (NewSpeed > KINDA_SMALL_NUMBER)
            {
                H *= (NewSpeed / H.Size());
            }
            else
            {
                H = FVector::ZeroVector;
            }
        }
    }

    // Анти-дрожь около нуля
    if (H.SizeSquared() <= FMath::Square(DynamicSet.StopSpeedEpsilon))
    {
        H = FVector::ZeroVector;
    }

    DynamicSet.Velocity.X = H.X;
    DynamicSet.Velocity.Y = H.Y;
}

/*

void UMovementSystemComponent::ApplyFrictionAndBraking(const float DeltaTime)
{
	if (!bIsOnGround) return;

	// Горизонтальная часть
	FVector H = FVector(DynamicSet.Velocity.X, DynamicSet.Velocity.Y, 0.f);

	const float Speed = H.Size();
	if (Speed <= KINDA_SMALL_NUMBER) return;

	// Эквивалент CharacterMovement: dV ~ (Friction*Speed + Braking) * dt
	const float Drop = (DynamicSet.GroundFriction * Speed + DynamicSet.BrakingDeceleration) * DeltaTime;
	const float NewSpeed = FMath::Max(Speed - Drop, 0.f);
	
	if (NewSpeed != Speed)
	{
		H *= NewSpeed / Speed;
		DynamicSet.Velocity.X = H.X;
		DynamicSet.Velocity.Y = H.Y;
	}
}

*/

void UMovementSystemComponent::ApplyGravity(const float DeltaTime)
{
	// Прямая гравитация
	DynamicSet.Velocity.Z += DynamicSet.GravityZ * -750.f * DeltaTime;
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