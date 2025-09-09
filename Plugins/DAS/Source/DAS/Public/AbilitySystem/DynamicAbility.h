
#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "GameplayTagContainer.h"

#if WITH_TOUCH
	#include "ManagerImpl/TouchManager.h"
#endif
#if WITH_ROTO
	#include "Core/RotoCameraManager.h"
#endif

#include "DynamicAbility.generated.h"

class UAttribute;

UENUM(BlueprintType)
enum class EAbilityState : uint8
{
	Inactive UMETA(DisplayName = "Inactive"),
	Activating UMETA(DisplayName = "Activating"),
	Active UMETA(DisplayName = "Active"),
};

enum class EAbilityFlag : uint8
{
	Updating = 0,
};
ENUM_CLASS_FLAGS(EAbilityFlag)


enum class EDisableType
{
	End,
	Removed,
	Forced,
	FromUpdate,
	Overridden
};

USTRUCT(BlueprintType)
struct FAbilitySlideSettings
{
	GENERATED_BODY()

	/**
	 * Задержка перед активацией слайда.
	 * Если слайд базовый, то работает как не посредственная задержка перед активацией самой способности.
	 * Если 0.f, то активация мгновенная.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeSettings", meta=(ClampMin="0.0"))
	float ActivationDelay = 0.f;

	/**
	 * Скорость обновления способности во время этого слайда и вызова функции UpdateAbility.
	 * Если 0.f, то способность во время работы этого слайда  не обновляется.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeSettings", meta=(ClampMin="0.0"))
	float UpdateAbilityRate = 0.f;

	/** Если включено, то обновление способности происходит каждый кард, и переминая UpdateAbilityRate не работает */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TimeSettings")
	bool bTickEveryFrame = false;

	/**
	 * Время при котором слайд остаётся активным.
	 * Если время вышло не на базовом слайде, то кастомный слайд сбросится на базовый,
	 * а если на базовом, то работа всей способности завершится.
	 * Если 0.f, то слайд работает бесконечно.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeSettings", meta=(ClampMin="0.0"))
	float MaxActiveTime = 0.f;


	/**
	 * Теги добавляющеюся владельцу способности во время работы этого слайда.
	 * Если слайд базовый, то теги не будут удаленны из владельца,
	 * не при каких условиях до завершения работы всей способности.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlideSettings")
	FGameplayTagContainer SlideTags;

	/**
   	 * Теги которые должны быть у владельца способности для активации этого слайда.
   	 * Если слайд базовый, то переменная используются для аналогичной проверки в начале работы всей способности,
   	 * при этом при последующем приключении на базовый слайд с кастомных сладов переменная и аналогичная проверка использоваться уже не будет.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlideSettings")
	FGameplayTagContainer ActivationNecessaryTags;

	/**
	 * Теги которые не должны быть у владельца способности для активации этого слайда.
	 * Если слайд базовый, то переменная используются для аналогичной проверки в начале работы всей способности,
	 * при этом при последующем приключении на базовый слайд с кастомных сладов переменная и аналогичная проверка использоваться уже не будет.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlideSettings")
	FGameplayTagContainer ActivationBlockedTags;
};

USTRUCT(BlueprintType)
struct FDynamicAbilitySettings : public FTableRowBase
{
	GENERATED_BODY()

	/** Если включено – то при выдаче этой способности владельцу будет произведена попытка ее активации */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Activation")
	bool ActivateAbilityOnGranted = false;

	/** Типы Inputs которые должны вызываться для этой способности */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TArray<FGameplayTag> InputsKeys;

	/**
	 * Настройки базового слайда способности,
	 * при котором внутренние настройки работают глобино на способность, а не на отдельно взятый слайд
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BaseSettings")
	FAbilitySlideSettings BaseSlideSettings;

	/**
	 * Набор тегов, которые эта способность может "перекрывать".
	 * При её активации все активные способности владельца, которые добавили эти теги 
	 * (в том числе через свои слайды), будут немедленно отключены с флагом Overridden.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BaseSettings")
	FGameplayTagContainer OverrideTags;

	/** Все слайды и их настройки которые способность может переключать во время ее работы */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BaseSettings")
	TMap<FGameplayTag, FAbilitySlideSettings> SlidesSettings;
};

UCLASS(Blueprintable, BlueprintType, ClassGroup=(DAS))
class DAS_API UDynamicAbility : public UObject
{
	GENERATED_BODY()

	template<typename T, typename AbilityT>
	friend class FAbilityInfoWindowModule;
	friend class UDynamicAbilitySystem;

	/** Не посредственно владелец способности и всей системы в которой она работает */
	UPROPERTY()
	TObjectPtr<AActor> Owner;

	/** Система, которая владеет способностью и управляет ей. Сама система принадлежит Owner */
	UPROPERTY()
	TObjectPtr<UDynamicAbilitySystem> AbilitySystem;

	/** Тег, обозначающий активный (текущий) слайд способности */
	FGameplayTag CurrentSlideType;

	/** Все активные флаги этой способности (битовая маска EAbilityFlag) */
	TSet<EAbilityFlag> AbilityFlags;

	/** Текущее состояние этой способности */
	EAbilityState AbilityState = EAbilityState::Inactive;
protected:
	FORCEINLINE const AActor* GetOwner() const { return Owner.Get(); }
	FORCEINLINE const FGameplayTag& GetCurrentSlideTag() const { return CurrentSlideType; }
	FORCEINLINE const EAbilityState& GetAbilityState() const { return AbilityState; }
	FORCEINLINE TSet<EAbilityFlag> GetAbilityFlags() const { return AbilityFlags; }
	FORCEINLINE const UDynamicAbilitySystem* GetAbilitySystem() const { return AbilitySystem.Get(); }

#if WITH_TOUCH
	const UTouchManager* GetTouchSystem() const;

	UTouchManager* GetTouchSystemMutable() const;
#endif
#if WITH_ROTO
	const ARotoCameraManager* GetRotoSystem() const;

	ARotoCameraManager* GetRotoSystemMutable() const;
#endif

	/**
	 *	Дает способности все разрешения и доступы.
	 *	Для работы этой переменной также нужно добавить класс способности в AttributesWithAllAllows в менеджере.
	 *	Эта функция нужна в редких случаях например для способностей которые являются базовыми,
	 *	в иных случаях и при работах с обычными классами способностей ее использование не рекомендуется.
	 */
	bool bGetAllAllows = false;

	/** Уникальное имя способности. Используется для хранения в системе и привязки её настроек. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FName AbilityName;

	/**
	 * Если включено, система автоматически ищет настройки AbilitySettings по имени AbilityName.
	 * Если выключено, настройки AbilitySettings можно задать вручную в конструкторе наследника.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bMustSearchSettings = false;

	/** Не посредственно сами настройки способности */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FDynamicAbilitySettings AbilitySettings;

	/** Функция для получения не const указателя на менеджер */ 
	UDynamicAbilitySystem* GetAbilitySystemMutable() const;

	/** Функция для получения const указателя на зарегистрированного ContextObject */ 
	const UObject* GetContextObject(const FName& Key) const;

 	/** Функция для получения указателя на зарегистрированного ContextObject */ 
	UObject* GetMutableContextObject(const FName& Key) const;
	
	/** Функция для смены активного слайда */
	bool ChangeSlide(const FGameplayTag& NewSlideType);

	/** Функция для получения атрибута  */
	UAttribute* GetAttribute(const TSubclassOf<UAttribute>& AttributeClass) const;
	
	/** Вызывается перед активацией способности для кастомный логики валидации */
	virtual bool ValidateAbilityActivation(const UObject* Activator) { return true; }

	/** Вызывается перед сменой слайда для кастомный логики валидации */
	virtual bool ValidateSlideChange(const FGameplayTag& SlideType) const { return true; }
	
	/**
	 * Вызывается для обновления способности во время ее работы.
	 * Для завершения работы способности нужно вернуть причину в виде FGameplayTag.
	 */
	virtual TOptional<FGameplayTag> UpdateAbility(float DeltaTime) { return TOptional<FGameplayTag>(); }
	
	/** Вызывается при выдаче игроку способности */
	virtual void OnAbilityAdded(const UObject* Adder) {} 
 
	/** Вызывается при удалении у игрока способности */
	virtual void OnAbilityRemoved(const UObject* Remover) {}

	/** Вызывается при активации способности */
	virtual void OnAbilityActivated(const UObject* Activator) {}

	/** Вызывается при завершении работы способности */
	virtual void OnAbilityDisabled(const EDisableType& DisableType, const FGameplayTag& Reason, const UObject* Disabler) {	}

	/** Вызывается при сменен слайда на новый */
	virtual void OnSlideChanged(const FGameplayTag& SlideType) {}
	
	/** Вызывается при вызове AddAbilityInput у менажера способности, если она активина */
	virtual void OnAddInput(const FGameplayTag& InputKey, const ETriggerEvent& Event) {}

	/** Вызывается при вызове AddAbilityInput у менажера способности, если она активина */
	virtual void OnAbilityInputVector(const FVector& WorldVector, const FGameplayTag& InputKey, const ETriggerEvent& Event) {}
public:
	FORCEINLINE const FName& GetAbilityName() const { return AbilityName; }
};