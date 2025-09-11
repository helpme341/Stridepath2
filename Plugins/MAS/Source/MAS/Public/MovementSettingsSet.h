
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Speed", meta=(ClampMin="0", Units="cm/s", ToolTip="Максимальная горизонтальная скорость на земле (по XY), см/с. Ограничивает скорость после применения ускорений и трения."))
	float MaxGroundSpeed = 2300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Speed", meta=(ClampMin="0", Units="cm/s", ToolTip="Максимальная горизонтальная скорость в воздухе, см/с. Ограничивает air-control и импульсы по XY."))
	float MaxAirSpeed = 600.f;

	// -------- ACCELERATION --------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Acceleration", meta=(ClampMin="0", Units="cm/s^2", ToolTip="Ускорение по вводу на земле (до клампа MaxGroundSpeed). Чем больше — тем резче разгон."))
	float GroundAcceleration = 4000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Acceleration", meta=(ClampMin="0", Units="cm/s^2", ToolTip="Ускорение по вводу в воздухе (air-control). Обычно меньше, чем на земле."))
	float AirAcceleration = 800.f;

	// -------- FRICTION & BRAKING --------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Friction & Braking", meta=(ClampMin="0", ToolTip="Коэффициент сухого трения на земле. Выше — сильнее замедление без ввода. Типично 3–8."))
	float GroundFriction = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Friction & Braking", meta=(ClampMin="0", Units="cm/s^2", ToolTip="Принудительное торможение (braking) на земле. Срабатывает когда нет активного ввода."))
	float BrakingDeceleration = 2048.f;

	// -------- GRAVITY --------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Gravity", meta=(ClampMin="0", ToolTip="Скейл гравитации: 1.0 ≈ стандартная земная. Увеличивайте для «тяжёлого» падения, уменьшайте для «лёгкого»."))
	float GravityZ = 1.f;

	// -------- GROUNDING / WALKABLE --------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Grounding", meta=(ClampMin="0", ClampMax="89", Units="deg", ToolTip="Максимальный угол уклона, по которому считаем поверхность ходибельной. Всё круче — считается стеной/скольжением."))
	float MaxSlopeAngleDeg = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Grounding", meta=(ClampMin="0", Units="cm", ToolTip="Длина зонда вниз для поиска пола. Больше — устойчивее на высокой скорости, но дороже и может «липнуть» к соседним полкам."))
	float GroundProbeLength = 60.f;

	// -------- SNAP / CONTACT --------
	// ⚠ ВАЖНО: это нижняя граница вертикальной скорости при контакте (значение ДОЛЖНО быть ≤ 0).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Grounding", meta=(ClampMax="0", Units="cm/s", ToolTip="Минимальная вертикальная скорость Vz при контакте с полом (отрицательная «прилипка»). Пример: -30 см/с не даёт капсуле дрожать и «подскакивать»."))
	float StickVelocityZ = -700.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Grounding", meta=(ClampMin="0", Units="cm", ToolTip="Короткий контактный допуск вниз для признания факта касания (2–4 см). Меньше — чаще «теряем» землю на скорости; больше — шанс ложных срабатываний."))
	float GroundContactTolerance = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Grounding", meta=(ClampMin="0", ClampMax="0.5", Units="s", ToolTip="Койот-тайм: как долго после потери контакта ещё считаем себя «на земле». Помогает не «проваливаться» на ребрах и краях."))
	float GroundGraceTime = 0.10f; // 100 мс

	// -------- SNAP / PREDICTIVE --------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Snap & Prediction", meta=(ClampMin="0", Units="cm", ToolTip="Базовая часть динамического снапа (добавляется к скорости*dt). Используется для «дотяжки» к поверхности на скорости."))
	float BaseSnapDistance = 10.f;

	// -------- PREDICTION AHEAD --------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Prediction", meta=(ClampMin="0", Units="cm", ToolTip="Максимальная дальность предиктивной проверки пола вперёд по ходу движения. Слишком большое значение может «липнуть» к несвязанным поверхностям."))
	float MaxPredictAhead = 80.f;

	// -------- RUNTIME (STATE) --------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Runtime", meta=(ClampMin="0", Units="s", ToolTip="Служебный счётчик (сколько времени мы вне контакта). Обычно обновляется кодом и не редактируется в дата-наборе."))
	float TimeSinceLostGround = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Movement|Runtime", meta=(Units="cm/s", ToolTip="Текущая скорость (состояние), обновляется кодом каждый тик."))
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