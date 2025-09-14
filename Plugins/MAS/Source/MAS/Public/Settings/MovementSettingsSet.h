
#pragma once

#include "CoreMinimal.h"
#include "MovementSettingsSet.generated.h"

UENUM(BlueprintType)
enum EMovementSettingsType : uint8
{
	MaxGroundSpeed,
	MaxAirSpeed,
	GroundAcceleration,
	AirAcceleration,
	BrakingDeceleration,
	GravityZ,
	MaxSlopeAngleDeg,
	GroundProbeLength,
	StickVelocityZ,
	GroundContactTolerance,
	
	GroundGraceTime,
	BaseSnapDistance,
	MaxPredictAhead,
	TimeSinceLostGround,
	
	Velocity,
};

template<EMovementSettingsType Key>
struct TMovementSetting;

USTRUCT(BlueprintType)
struct FMovementSettingsSet
{
	GENERATED_BODY()
	// -------- SPEED --------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement",
		meta=(ClampMin="0", Units="cm/s", ToolTip="Максимальная скорость на земле (XY), см/с."))
	float MaxGroundSpeed = 2300.0f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Speed",
		meta=(ClampMin="0", Units="cm/s", ToolTip="Максимальная скорость в воздухе (XY), см/с."))
	float MaxAirSpeed = 600.0f;

	// -------- ACCELERATION --------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Acceleration",
		meta=(ClampMin="0", Units="cm/s^2", ToolTip="Ускорение на земле, см/с²."))
	float GroundAcceleration = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Acceleration", meta=(ToolTip="Кривая ускорения на земле"))
	TObjectPtr<UCurveFloat> GroundAccelerationCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Acceleration",
		meta=(ClampMin="0", Units="cm/s^2", ToolTip="Ускорение в воздухе, см/с²."))
	float AirAcceleration = 800.0f;

	// -------- FRICTION & BRAKING --------
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Friction & Braking",
		meta=(ClampMin="0", Units="cm/s^2", ToolTip="Принудительное торможение на земле, см/с²."))
	float BrakingDeceleration = 2048.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Friction & Braking",
		meta=(ClampMin="0", Units="cm/s^2", ToolTip=""))
	float LateralFriction = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Friction & Braking",
	meta=(ClampMin="0", Units="cm/s^2", ToolTip=""))
	float StopSpeedEpsilon = 1.0f;

	// -------- GRAVITY --------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Gravity",
		meta=(ClampMin="0", ToolTip="Множитель гравитации (1.0 = стандартная Земная)."))
	float GravityZ = 1.0f;

	// -------- GROUNDING / WALKABLE --------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Grounding",
		meta=(ClampMin="0", ClampMax="89", Units="deg", ToolTip="Максимальный угол уклона, градусы."))
	float MaxSlopeAngleDeg = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Grounding",
		meta=(ClampMin="0", Units="cm", ToolTip="Длина трассировки вниз для поиска пола, см."))
	float GroundProbeLength = 60.0f;

	// -------- SNAP / CONTACT --------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Grounding",
		meta=(ClampMax="0", Units="cm/s", ToolTip="Минимальная вертикальная скорость при контакте, см/с (≤0)."))
	float StickVelocityZ = -700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Grounding",
		meta=(ClampMin="0", Units="cm", ToolTip="Контактный допуск вниз, см."))
	float GroundContactTolerance = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Grounding",
		meta=(ClampMin="0", ClampMax="0.5", Units="s", ToolTip="Койот-тайм после потери контакта, с."))
	float GroundGraceTime = 0.10f;

	// -------- SNAP / PREDICTIVE --------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Snap & Prediction",
		meta=(ClampMin="0", Units="cm", ToolTip="Базовое расстояние динамического снапа, см."))
	float BaseSnapDistance = 10.0f;

	// -------- PREDICTION AHEAD --------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Prediction",
		meta=(ClampMin="0", Units="cm", ToolTip="Макс. дальность предиктивной проверки пола, см."))
	float MaxPredictAhead = 80.0f;

	// -------- RUNTIME (STATE) --------
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Movement|Runtime",
		meta=(ClampMin="0", Units="s", ToolTip="Счётчик времени вне контакта, с."))
	float TimeSinceLostGround = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Movement|Runtime",
		meta=(Units="cm/s", ToolTip="Текущая скорость, см/с."))
	FVector Velocity = FVector::ZeroVector;

	template<EMovementSettingsType Key>
	TMovementSetting<Key>::ValueType& GetValue()
	{
		return this->*TMovementSetting<Key>::Member;
	}
	
	template<EMovementSettingsType Key>
	const typename TMovementSetting<Key>::ValueType& GetValue() const
	{
		return this->*TMovementSetting<Key>::Member;
	}

	/**
	template<typename F>
	static void ForEachSetting(F&& Fn)
	{
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::MaxGroundSpeed>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::MaxAirSpeed>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::GroundAcceleration>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::AirAcceleration>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::GroundFriction>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::BrakingDeceleration>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::GravityZ>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::JumpZVelocity>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::MaxSlopeAngleDeg>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::GroundProbeLength>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::PlaneConstraintNormal>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::GroundHit>{});
	}
		
	FMovementSettingsSet& CopyFrom(const FMovementSettingsSet& Other, TInvoker<bool(const EMovementSettingsType&)> ValidationSettingChange)
	{
		auto TryCopy = [&](auto KeyC)
		{
			static constexpr EMovementSettingsType Key = KeyC.value;
			if (ValidationSettingChange(Key)) this->GetValue<Key>() = Other.GetValue<Key>();
		};
		ForEachSetting(TryCopy);
		return *this;
	}
	*/
};

template<>
struct TMovementSetting<MaxGroundSpeed>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::MaxGroundSpeed;
};
template<>
struct TMovementSetting<MaxAirSpeed>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::MaxAirSpeed;
};

template<>
struct TMovementSetting<GroundAcceleration>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::GroundAcceleration;
};
template<>
struct TMovementSetting<AirAcceleration>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::AirAcceleration;
};
template<>
struct TMovementSetting<BrakingDeceleration>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::BrakingDeceleration;
};

template<>
struct TMovementSetting<GravityZ>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::GravityZ;
};

template<>
struct TMovementSetting<MaxSlopeAngleDeg>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::MaxSlopeAngleDeg;
};

template<>
struct TMovementSetting<GroundProbeLength>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::GroundProbeLength;
};

template<>
struct TMovementSetting<StickVelocityZ>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::StickVelocityZ;
};

template<>
struct TMovementSetting<GroundContactTolerance>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::GroundContactTolerance;
};


template<>
struct TMovementSetting<GroundGraceTime>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::GroundGraceTime;
};

template<>
struct TMovementSetting<BaseSnapDistance>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::BaseSnapDistance;
};


template<>
struct TMovementSetting<MaxPredictAhead>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::MaxPredictAhead;
};


template<>
struct TMovementSetting<TimeSinceLostGround>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::TimeSinceLostGround;
};


template<>
struct TMovementSetting<Velocity>
{
	using ValueType = FVector;
	static constexpr auto Member = &FMovementSettingsSet::Velocity;
};