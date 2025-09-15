
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

	check(MovementSettings);
	DynamicSet = MovementSettings->DefaultSettings;

	check(GetOwner());
	SetUpdatedComponent(GetOwner()->GetRootComponent());

	CapsuleComp = Cast<UCapsuleComponent>(UpdatedComponent);
}

void UMovementSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (ShouldSkipUpdate(DeltaTime)) return;

	DynamicSet.TimeSinceStepFail += DeltaTime;
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
	if (DynamicSet.TimeSinceStepFail < DynamicSet.PredictBlockAfterStepFail) return false;

	const FVector H(DynamicSet.Velocity.X, DynamicSet.Velocity.Y, 0.f);
	const float   HSpeed = H.Size();
	if (HSpeed <= KINDA_SMALL_NUMBER) return false;

	const FVector Dir   = H / HSpeed;
	const float   Ahead = FMath::Clamp(HSpeed * DeltaTime * 1.5f, 6.f, DynamicSet.MaxPredictAhead);

	const FVector Start = UpdatedComponent->GetComponentLocation() + Dir * Ahead;
	const FVector End   = Start + FVector(0, 0, -(DynamicSet.GroundProbeLength + 6.f));

	if (!SweepSingleByChannel(OutHit, Start, End) || !IsWalkable(OutHit.Normal)) return false;

	const float ComputeSnapDistance = DynamicSet.BaseSnapDistance + FMath::Clamp(HSpeed * DeltaTime * 1.5f, 0.f, 30.f);
	const float AllowedStepDown     = DynamicSet.MaxStepHeight + DynamicSet.MinLedgeDown;

	if (OutHit.IsValidBlockingHit())
	{
		const float Dz = Start.Z - OutHit.ImpactPoint.Z;
		if (Dz < 0.f || Dz > AllowedStepDown) return false;
	}

	// допускаем небольшой зазор, растущий со скоростью/Δt
	return OutHit.Distance <= ComputeSnapDistance + 2.f;
}

void UMovementSystemComponent::PrePhysics_UpdateGroundState()
{
	const float DT = GetWorld()->GetDeltaSeconds();

	SweepForGround(GroundHit);

	// Длинный зонд оставляем для проекции ввода по уклону
	FHitResult ContactHit;
	const bool bContact = ShortGroundContact(ContactHit);

	// Предиктивная опора впереди
	FHitResult PredHit;
	const bool bPredict = PredictiveGround(PredHit, DT);

	// Койот-тайм
	if (bContact) DynamicSet.TimeSinceLostGround = 0.f;
	else DynamicSet.TimeSinceLostGround += DT;

	const bool bGrace = DynamicSet.TimeSinceLostGround <= DynamicSet.GroundGraceTime && DynamicSet.Velocity.Z <= 150.f;
	bIsOnGround = bContact || bPredict || bGrace;

	// Нормаль для проекции ввода — контактная приоритетнее, потом предиктивная
	if (bContact) GroundHit = ContactHit;
	else if (bPredict) GroundHit = PredHit;

	// Прижим по Z — ТОЛЬКО при реальном контакте
	if (bIsOnGround && DynamicSet.Velocity.Z < 0.f) DynamicSet.Velocity.Z = FMath::Max(DynamicSet.Velocity.Z, DynamicSet.StickVelocityZ);
}

bool UMovementSystemComponent::ShortGroundContact(FHitResult& OutHit) const
{
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FVector End   = Start + FVector(0, 0, -(DynamicSet.GroundContactTolerance + 0.5f));

	return SweepSingleByChannel(OutHit, Start, End) && IsWalkable(OutHit.Normal);
}

void UMovementSystemComponent::ApplyInputAcceleration(const float DeltaTime)
{
	if (LastInputDir = ConsumeInputVector(); !LastInputDir.IsNearlyZero())
	{
		const bool  bGround		= bIsOnGround;
		const float MaxSpeed	= bGround ? DynamicSet.MaxGroundSpeed	  : DynamicSet.MaxAirSpeed;
		const float TargetAccel	= bGround ? DynamicSet.GroundAcceleration : DynamicSet.AirAcceleration;

		FVector InputDir = LastInputDir.GetSafeNormal();
		if (bGround && GroundHit.IsValidBlockingHit()) InputDir = FVector::VectorPlaneProject(InputDir, GroundHit.Normal).GetSafeNormal();

		FVector HVel(DynamicSet.Velocity.X, DynamicSet.Velocity.Y, 0.f);
		const float SpeedFrac = FMath::Clamp(HVel.Size() / MaxSpeed, 0.f, 1.f);

		const float AccelScale = DynamicSet.GroundAccelerationCurve != nullptr ? DynamicSet.GroundAccelerationCurve->GetFloatValue(SpeedFrac) : 1.f;

		// 5) Применить ускорение
		const FVector Accel = InputDir * TargetAccel * AccelScale;
		DynamicSet.Velocity += Accel * DeltaTime;

		// 6) Кламп горизонтальной составляющей под MaxSpeed
		HVel = FVector(DynamicSet.Velocity.X, DynamicSet.Velocity.Y, 0.f);
		const float NewHSpeed = HVel.Size();
		if (NewHSpeed > MaxSpeed && NewHSpeed > KINDA_SMALL_NUMBER)
		{
			const FVector ClampedH = HVel * (MaxSpeed / NewHSpeed);
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
	float   Speed = H.Size();
	if (Speed <= KINDA_SMALL_NUMBER) return;

	if (!!LastInputDir.IsNearlyZero())
	{
		// Тормозим к нулю (без капа в присутствии ввода)
		const float Drop     = DynamicSet.BrakingDeceleration * DeltaTime;
		const float NewSpeed = FMath::Max(Speed - Drop, 0.f);

		if (NewSpeed <= DynamicSet.StopSpeedEpsilon) H = FVector::ZeroVector;
		else H *= NewSpeed / Speed;
	}
	else
	{
		// Разложим скорость на вдоль/поперёк желаемого направления
		const FVector Dir      = LastInputDir; // уже нормализован и спроецирован по склону
		const float   AlongMag = FVector::DotProduct(H, Dir);
		const FVector Along    = Dir * AlongMag;
		const FVector Side     = H - Along;

		// Дамп только боковой части — убирает «занос»
		const float SideSpeed = Side.Size();
		if (SideSpeed > KINDA_SMALL_NUMBER)
		{
			const float  SideDrop  = DynamicSet.LateralFriction * DeltaTime;
			const float  NewSide   = FMath::Max(SideSpeed - SideDrop, 0.f);
			const FVector NewSideV = Side * (NewSide / SideSpeed);
			H = Along + NewSideV;
		}

		// Если жмём, но горизонтальная скорость ПРОТИВ ввода — разрешим тормозить «встречную»
		if (AlongMag < 0.f)
		{
			const float DropOpp  = DynamicSet.BrakingDeceleration * DeltaTime;
			const float NewSpeed = FMath::Max(H.Size() - DropOpp, 0.f);
			if (NewSpeed > KINDA_SMALL_NUMBER) H *= NewSpeed / H.Size();
			else H = FVector::ZeroVector;
		}
	}

	// Анти-дрожь около нуля
	if (H.SizeSquared() <= FMath::Square(DynamicSet.StopSpeedEpsilon)) H = FVector::ZeroVector;

	DynamicSet.Velocity.X = H.X;
	DynamicSet.Velocity.Y = H.Y;
}

void UMovementSystemComponent::ApplyGravity(const float DeltaTime)
{
	// Прямая гравитация
	DynamicSet.Velocity.Z -= DynamicSet.GravityZ * DeltaTime;
}

void UMovementSystemComponent::MoveWithCollisions(const float DeltaTime)
{
	const FVector  Delta = DynamicSet.Velocity * DeltaTime;
	FHitResult     Hit;

	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentRotation(), true, Hit);

	if (!Hit.IsValidBlockingHit()) return;

	const bool bWall      = Hit.Normal.Z < DynamicSet.MaxWallCosZ;
	const bool bHasInput  = !LastInputDir.IsNearlyZero();
	const bool bCanTryStep =
		bIsOnGround &&
		Hit.Time < 1.f &&
		bWall &&
		DynamicSet.MaxStepHeight > 0.f &&
		DynamicSet.TimeSinceStepFail >= DynamicSet.StepAttemptCooldown &&
		bHasInput;

	bool bStepped = false;
	if (bCanTryStep)
	{
		const FVector HVel = FVector(DynamicSet.Velocity.X, DynamicSet.Velocity.Y, 0.f);
		const float HSpeed = HVel.Size();
		if (HSpeed > KINDA_SMALL_NUMBER)
		{
			const FVector MoveDir = HVel / HSpeed;
			const float IntoWall = FVector::DotProduct(MoveDir, -Hit.Normal);
			if (IntoWall < 0.4f) return;
		}

		
		// Если почти стоим — добавим минимальный «вперёд»
		FVector HDelta = FVector(DynamicSet.Velocity.X, DynamicSet.Velocity.Y, 0.f) * DeltaTime * (1.f - Hit.Time);
		if (HDelta.SizeSquared() < FMath::Square(DynamicSet.MinStepForward))
		{
			FVector Dir = LastInputDir.IsNearlyZero() ? FVector::ZeroVector : LastInputDir.GetSafeNormal();
			if (bIsOnGround && GroundHit.IsValidBlockingHit()) Dir = FVector::VectorPlaneProject(Dir, GroundHit.Normal).GetSafeNormal();
			if (!Dir.IsNearlyZero()) HDelta = Dir * DynamicSet.MinStepForward;
		}

		bStepped = TryStepUp(Hit, HDelta, DynamicSet.MaxStepHeight);
		if (!bStepped) DynamicSet.TimeSinceStepFail = 0.f;
	}

	if (!bStepped)
	{
		SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit);

		if (Hit.Normal.Z < DynamicSet.MaxWallCosZ)
		{
			const FVector HVel   = FVector(DynamicSet.Velocity.X, DynamicSet.Velocity.Y, 0.f);
			const FVector HSlide = FVector::VectorPlaneProject(HVel, Hit.Normal);
			DynamicSet.Velocity.X = HSlide.X;
			DynamicSet.Velocity.Y = HSlide.Y;
		}

		if (DynamicSet.Velocity.Z < 0.f && Hit.Normal.Z > 0.2f) DynamicSet.Velocity.Z = 0.f;
	}
}

bool UMovementSystemComponent::TryStepUp(const FHitResult& BlockingHit, const FVector& HDelta, const float MaxStepHeight)
{
	if (!UpdatedComponent || HDelta.IsNearlyZero()) return false;

	const FVector Up       = FVector::UpVector;
	const float   UpDist   = MaxStepHeight + 2.f;
	const float   DownDist = MaxStepHeight + DynamicSet.MinLedgeDown;

	FScopedMovementUpdate ScopedMove(UpdatedComponent.Get(), EScopedUpdate::DeferredUpdates);

	const FVector OwnerLoc   = UpdatedComponent->GetComponentLocation();
	const float   LedgeHeight= BlockingHit.ImpactPoint.Z - OwnerLoc.Z;

	// слишком низко или слишком высоко — не шаг
	if (LedgeHeight <= 1.f || LedgeHeight > DynamicSet.MaxStepHeight + 2.f) return false;

	// 0) Быстрая проверка «это ступень?»
	// На текущей высоте впереди — блок (мы уже знаем), а на высоте шага должно быть свободно.
	{
		FHitResult TestUpHit;
		SafeMoveUpdatedComponent(Up * UpDist, UpdatedComponent->GetComponentQuat(), true, TestUpHit);
		if (TestUpHit.bStartPenetrating)
		{
			ScopedMove.RevertMove();
			return false;
		}

		FHitResult TestFwdHigh;
		SafeMoveUpdatedComponent(HDelta, UpdatedComponent->GetComponentQuat(), true, TestFwdHigh);

		// Если на высоте шага впереди снова стена — это не ступень
		if (TestFwdHigh.IsValidBlockingHit() && TestFwdHigh.Normal.Z < DynamicSet.MaxWallCosZ)
		{
			ScopedMove.RevertMove();
			return false;
		}

		// Откатываем подготовительные тесты
		ScopedMove.RevertMove();
	}

	// 1) Подъём
	FHitResult UpHit;
	SafeMoveUpdatedComponent(Up * UpDist, UpdatedComponent->GetComponentQuat(), true, UpHit);
	if (UpHit.bStartPenetrating || (UpHit.IsValidBlockingHit() && UpHit.Normal.Z < DynamicSet.MaxWallCosZ))
	{
		ScopedMove.RevertMove();
		return false;
	}

	// 2) Горизонталь на вершине ступени
	FHitResult FwdHit;
	SafeMoveUpdatedComponent(HDelta, UpdatedComponent->GetComponentQuat(), true, FwdHit);
	if (FwdHit.IsValidBlockingHit() && FwdHit.Normal.Z < DynamicSet.MaxWallCosZ)
	{
		ScopedMove.RevertMove();
		return false;
	}

	// 3) Опускание вниз
	FHitResult DownHit;
	SafeMoveUpdatedComponent(-Up * DownDist, UpdatedComponent->GetComponentQuat(), true, DownHit);
	if (DownHit.IsValidBlockingHit() && IsWalkable(DownHit.Normal))
	{
		// Чуть подбросим, чтобы «перевалиться» через кромку без дрожи
		DynamicSet.Velocity.Z = FMath::Max(DynamicSet.Velocity.Z, DynamicSet.StepUpVerticalBoost);
		GroundHit = DownHit;
		bIsOnGround = true;
		return true;
	}

	ScopedMove.RevertMove();
	return false;
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

	const float Speed = DynamicSet.Velocity.Size();
	const float Ahead = FMath::Clamp(Speed * GetWorld()->GetDeltaSeconds() * 1.5f, 0.f, 120.f);

	constexpr float StepSlack = 6.f;
	const float ProbeDown = DynamicSet.GroundProbeLength + Ahead + StepSlack;

	const FVector End = Start + FVector(0, 0, -ProbeDown);

	if (!SweepSingleByChannel(OutHit, Start, End)) return false;

	const float SlopeDeg = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(OutHit.Normal, FVector::UpVector)));
	return SlopeDeg <= DynamicSet.MaxSlopeAngleDeg;
}
