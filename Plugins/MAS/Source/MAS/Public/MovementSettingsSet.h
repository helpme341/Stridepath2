
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
	GroundFriction,
	BrakingDeceleration,
	GravityZ,
	JumpZVelocity,
	MaxSlopeAngleDeg,
	GroundProbeLength,

	PlaneConstraintNormal,
	Velocity,
};

template<EMovementSettingsType Key>
struct TMovementSetting;

USTRUCT(BlueprintType)
struct FMovementSettingsSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxGroundSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxAirSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GroundAcceleration = 4000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AirAcceleration = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GroundFriction = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BrakingDeceleration = 2048.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GravityZ = -980.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float JumpZVelocity = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxSlopeAngleDeg = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GroundProbeLength = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector PlaneConstraintNormal = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
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
struct TMovementSetting<GroundFriction>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::GroundFriction;
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
struct TMovementSetting<JumpZVelocity>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::JumpZVelocity;
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
struct TMovementSetting<PlaneConstraintNormal>
{
	using ValueType = FVector;
	static constexpr auto Member = &FMovementSettingsSet::PlaneConstraintNormal;
};

template<>
struct TMovementSetting<Velocity>
{
	using ValueType = FVector;
	static constexpr auto Member = &FMovementSettingsSet::Velocity;
};