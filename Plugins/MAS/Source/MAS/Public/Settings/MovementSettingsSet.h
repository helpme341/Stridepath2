
#pragma once

#include "CoreMinimal.h"
#include "MovementSettingsSet.generated.h"

UENUM(BlueprintType)
enum EMovementSettingsType : uint8
{
	// 1) Speed
	MaxGroundSpeed,
	MaxAirSpeed,

	// 2) Acceleration
	GroundAcceleration,
	GroundAccelerationCurve,
	AirAcceleration,

	// 3) Friction & Braking
	BrakingDeceleration,
	LateralFriction,
	StopSpeedEpsilon,

	// 4) Gravity
	GravityZ,

	// 5) Grounding
	MaxSlopeAngleDeg,
	GroundProbeLength,
	GroundContactTolerance,
	GroundGraceTime,

	// 6) Snap & Prediction (grounding-related)
	StickVelocityZ,
	BaseSnapDistance,

	// 7) Predict forward
	MaxPredictAhead,

	// 8) Steps
	MaxStepHeight,
	MinLedgeDown,
	MinStepForward,
	StepUpVerticalBoost,
	StepAttemptCooldown,
	MaxWallCosZ,
	PredictBlockAfterStepFail,

	// 9) Runtime
	TimeSinceLostGround,
	Velocity,
	TimeSinceStepFail,
};

template<EMovementSettingsType Key>
struct TMovementSetting;

USTRUCT(BlueprintType)
struct FMovementSettingsSet
{
	GENERATED_BODY()
	
// ---- SPEED ---------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Speed",
	    meta=(ClampMin="0", Units="cm/s",
	         ToolTip="Макс. скорость на земле (XY). Влияет на кламп после разгона. Крути ПОСЛЕ GroundAcceleration/Curve. Диапазон: 1600–3200 см/с."))
	float MaxGroundSpeed = 2300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Speed",
	    meta=(ClampMin="0", Units="cm/s",
	         ToolTip="Макс. скорость в воздухе (XY). Ограничивает контроль в полёте. Диапазон: 450–900 см/с."))
	float MaxAirSpeed = 600.0f;


// ---- ACCELERATION --------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Acceleration",
	    meta=(ClampMin="0", Units="cm/s^2",
	         ToolTip="Ускорение на земле. Определяет, как быстро набираем скорость. Сначала крутится оно, потом кривая. Диапазон: 3000–7000 см/с²."))
	float GroundAcceleration = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Acceleration",
	    meta=(ToolTip="Кривая ускорения по доле от MaxGroundSpeed (0..1). Формирует профиль разгона: бодрый старт или мягкий хвост."))
	TObjectPtr<UCurveFloat> GroundAccelerationCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Acceleration",
	    meta=(ClampMin="0", Units="cm/s^2",
	         ToolTip="Ускорение в воздухе. Контроль в полёте, ниже чем на земле. Диапазон: 500–1500 см/с²."))
	float AirAcceleration = 800.0f;


// ---- FRICTION & BRAKING --------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Friction & Braking",
	    meta=(ClampMin="0", Units="cm/s^2",
	         ToolTip="Торможение без ввода. Чем выше — тем короче пробег до нуля. Диапазон: 1500–3000 см/с²."))
	float BrakingDeceleration = 2048.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Friction & Braking",
	    meta=(ClampMin="0", Units="cm/s^2",
	         ToolTip="Гашение поперечной (боковой) скорости при нажатом вводе. Убирает занос у стен. Диапазон: 400–900 см/с²."))
	float LateralFriction = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Friction & Braking",
	    meta=(ClampMin="0", Units="cm/s",
	         ToolTip="Порог «почти ноль» для горизонтальной скорости (анти-дрожь). Всё ниже прибивается к 0. Диапазон: 1–3 см/с."))
	float StopSpeedEpsilon = 1.5f;


// ---- GRAVITY -------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Gravity",
	    meta=(ClampMin="0", Units="cm/s^2",
	         ToolTip="Гравитация (см/с²). 980 ≈ земная. Выше — жестче падение/приземления, активнее спуск по ступеням."))
	float GravityZ = 980.0f;


// ---- GROUNDING (slope, probes, grace) ------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Grounding",
	    meta=(ClampMin="0", ClampMax="89", Units="deg",
	         ToolTip="Макс. угол ходьбы. Больше — разрешаем круче склоны. Типично: 35–50°."))
	float MaxSlopeAngleDeg = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Grounding",
	    meta=(ClampMin="0", Units="cm",
	         ToolTip="Длина зонда вниз. Бери HalfHeight+10..20 см. Слишком большая — тянет вниз и порождает дрожь."))
	float GroundProbeLength = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Grounding",
	    meta=(ClampMin="0", Units="cm",
	         ToolTip="Контактный допуск вниз. Малый люфт для стабильной опоры. Типично: 6–8 см."))
	float GroundContactTolerance = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Grounding",
	    meta=(ClampMin="0", ClampMax="0.5", Units="s",
	         ToolTip="Койот-тайм после потери опоры. Держит «на земле» короткое время. Типично: 0.08–0.12 с."))
	float GroundGraceTime = 0.10f;


// ---- SNAP / STICK --------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Grounding",
	    meta=(ClampMax="0", Units="cm/s",
	         ToolTip="Минимальная отрицательная Vz при контакте (прижим). Более отрицательно — активнее сползает вниз по ступеням. Типично: −80…−300 см/с."))
	float StickVelocityZ = -150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Snap & Prediction",
	    meta=(ClampMin="0", Units="cm",
	         ToolTip="Базовый снап к полу. Меньше — меньше залипания/конфликтов со ступенями. Типично: 8–12 см."))
	float BaseSnapDistance = 10.0f;


// ---- PREDICTION AHEAD ----------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Prediction",
	    meta=(ClampMin="0", Units="cm",
	         ToolTip="Дальность предиктивного луча вперёд. Стабилизирует разбег, но на кромках может мешать. Типично: 60–120 см (рост с H-скоростью)."))
	float MaxPredictAhead = 100.0f;


// ---- STEPS / LEDGES ------------------------------------------------------

	UPROPERTY(EditAnywhere, Category="Movement|Steps",
	    meta=(ClampMin="0", Units="cm",
	         ToolTip="Макс. высота шага вверх. Определяет, через какой порог «перешагиваем». Типично: 40–48 см."))
	float MaxStepHeight = 45.f;

	UPROPERTY(EditAnywhere, Category="Movement|Steps",
	    meta=(ClampMin="0", Units="cm",
	         ToolTip="Запас вниз при завершении шага (опускание). Помогает «встать» на верх. Типично: 8–12 см."))
	float MinLedgeDown = 10.f;

	UPROPERTY(EditAnywhere, Category="Movement|Steps",
	    meta=(ClampMin="0", Units="cm",
	         ToolTip="Мин. горизонтальный сдвиг для шага с места. Убирает «пиление» кромки. Типично: 16–24 см."))
	float MinStepForward = 24.f;

	UPROPERTY(EditAnywhere, Category="Movement|Steps",
	    meta=(ClampMin="0", Units="cm/s",
	         ToolTip="Вертикальный буст при успешном шаге (см/с). Помогает перевалиться через грань. Типично: 120–200."))
	float StepUpVerticalBoost = 160.f;

	UPROPERTY(EditAnywhere, Category="Movement|Steps",
	    meta=(ClampMin="0", Units="s",
	         ToolTip="Кулдаун после неудачной попытки шага (анти-флаппер). Исключает дрожь на ребре. Типично: 0.10–0.20 с."))
	float StepAttemptCooldown = 0.10f;

	UPROPERTY(EditAnywhere, Category="Movement|Steps",
	    meta=(ClampMin="0", ClampMax="1",
	         ToolTip="Порог Z нормали для стен. Ниже — считаем препятствием для шага. Типично: ~0.2."))
	float MaxWallCosZ = 0.2f;

	UPROPERTY(EditAnywhere, Category="Movement|Steps",
	    meta=(ClampMin="0", Units="s",
	         ToolTip="Время блокировки предиктива после провала шага (анти-дрожь на кромке). Типично: 0.06–0.12 с."))
	float PredictBlockAfterStepFail = 0.12f;


// ---- RUNTIME (только просмотр) -------------------------------------------

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Movement|Runtime",
	    meta=(ClampMin="0", Units="s",
	         ToolTip="Секунд с последнего контакта с землёй (runtime, для отладки)."))
	float TimeSinceLostGround = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Movement|Runtime",
	    meta=(Units="cm/s",
	         ToolTip="Текущая скорость (runtime, только просмотр)."))
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Movement|Runtime",
	    meta=(Units="s",
	         ToolTip="Секунд с последней неудачной попытки шага (runtime, для отладки)."))
	float TimeSinceStepFail = 0.2f;


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
		// 1) Speed
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::MaxGroundSpeed>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::MaxAirSpeed>{});

		// 2) Acceleration
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::GroundAcceleration>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::GroundAccelerationCurve>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::AirAcceleration>{});

		// 3) Friction & Braking
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::BrakingDeceleration>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::LateralFriction>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::StopSpeedEpsilon>{});

		// 4) Gravity
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::GravityZ>{});

		// 5) Grounding
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::MaxSlopeAngleDeg>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::GroundProbeLength>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::GroundContactTolerance>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::GroundGraceTime>{});

		// 6) Snap & Prediction
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::StickVelocityZ>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::BaseSnapDistance>{});

		// 7) Predict forward
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::MaxPredictAhead>{});

		// 8) Steps
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::MaxStepHeight>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::MinLedgeDown>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::MinStepForward>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::StepUpVerticalBoost>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::StepAttemptCooldown>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::MaxWallCosZ>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::PredictBlockAfterStepFail>{});

		// 9) Runtime
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::TimeSinceLostGround>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::Velocity>{});
		Fn(std::integral_constant<EMovementSettingsType, EMovementSettingsType::TimeSinceStepFail>{});
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

// ---------------------- 1) Speed ----------------------
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

// ------------------- 2) Acceleration -------------------
template<>
struct TMovementSetting<GroundAcceleration>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::GroundAcceleration;
};

template<>
struct TMovementSetting<GroundAccelerationCurve>
{
	using ValueType = TObjectPtr<UCurveFloat>;
	static constexpr auto Member = &FMovementSettingsSet::GroundAccelerationCurve;
};

template<>
struct TMovementSetting<AirAcceleration>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::AirAcceleration;
};

// --------------- 3) Friction & Braking -----------------
template<>
struct TMovementSetting<BrakingDeceleration>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::BrakingDeceleration;
};

template<>
struct TMovementSetting<LateralFriction>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::LateralFriction;
};

template<>
struct TMovementSetting<StopSpeedEpsilon>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::StopSpeedEpsilon;
};

// --------------------- 4) Gravity ----------------------
template<>
struct TMovementSetting<GravityZ>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::GravityZ;
};

// -------------------- 5) Grounding ---------------------
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

// -------- 6) Snap & Prediction (grounding-related) -----
template<>
struct TMovementSetting<StickVelocityZ>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::StickVelocityZ;
};

template<>
struct TMovementSetting<BaseSnapDistance>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::BaseSnapDistance;
};

// ---------------- 7) Predict forward -------------------
template<>
struct TMovementSetting<MaxPredictAhead>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::MaxPredictAhead;
};

// --------------------- 8) Steps ------------------------
template<>
struct TMovementSetting<MaxStepHeight>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::MaxStepHeight;
};

template<>
struct TMovementSetting<MinLedgeDown>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::MinLedgeDown;
};

template<>
struct TMovementSetting<MinStepForward>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::MinStepForward;
};

template<>
struct TMovementSetting<StepUpVerticalBoost>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::StepUpVerticalBoost;
};

template<>
struct TMovementSetting<StepAttemptCooldown>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::StepAttemptCooldown;
};

template<>
struct TMovementSetting<MaxWallCosZ>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::MaxWallCosZ;
};

template<>
struct TMovementSetting<PredictBlockAfterStepFail>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::PredictBlockAfterStepFail;
};

// --------------------- 9) Runtime ----------------------
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

template<>
struct TMovementSetting<TimeSinceStepFail>
{
	using ValueType = float;
	static constexpr auto Member = &FMovementSettingsSet::TimeSinceStepFail;
};