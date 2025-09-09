
#pragma once

#include "CoreMinimal.h"
#include "Attribute.h"
#include "DynamicAbility.h"
#include "StaticTickerManager.h"

#if WITH_TOUCH
	#include "ManagerImpl/TouchManager.h"
#endif
#if WITH_ROTO
	#include "Core/RotoCameraManager.h"
#endif

#include "DynamicAbilitySystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDynamicAbilitySystem, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAddedAbility, FName, Key);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRemovedAbility, FName, Key);

enum class ESlideSettingsType
{
	Auto,
	Base,
	Custom,
	Current
};

USTRUCT(BlueprintType)
struct FContextObjectSettings
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TSet<TSubclassOf<UDynamicAbility>> ConstAllows;
};

USTRUCT(BlueprintType)
struct FAttributeClassesContainer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TArray<TSubclassOf<UAttribute>> Attributes;
};

UCLASS(Blueprintable, BlueprintType, ClassGroup=(DAS), meta=(BlueprintSpawnableComponent))
class DAS_API UDynamicAbilitySystem : public UActorComponent, public FStaticTickerManager
{
	GENERATED_BODY()

	template<typename T, typename AbilityT>
	friend class FAbilityInfoWindowModule;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	TObjectPtr<UDataTable> AbilitiesSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	TSet<TSubclassOf<UAttribute>> RegisteredAttributes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SecuritySettings")
	TSet<TSubclassOf<UDynamicAbility>> AttributesWithAllAllows;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SecuritySettings")
	TMap<TSubclassOf<UDynamicAbility>, FAttributeClassesContainer> AttributesSecuritySettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SecuritySettings")
	TSet<TSubclassOf<UDynamicAbility>> GettingSystemRefAllows;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SecuritySettings")
	TMap<FName, FContextObjectSettings> ContextObjectsConstAllows;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SecuritySettings")
	TMap<TSubclassOf<UDynamicAbility>, bool> GettingTouchSystemRefAllows;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SecuritySettings")
	TMap<TSubclassOf<UDynamicAbility>, bool> RotoTouchSystemRefAllows;
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	FORCEINLINE void CreateRegisteredAttributes()
	{
		if (RegisteredAttributes.IsEmpty())
		{
			UE_LOG(LogDynamicAbilitySystem, Warning, TEXT("No registered attributes found — nothing will be created."));
			return;
		}
		for (auto& AttributeClass : RegisteredAttributes)
		{
			Attributes.Add(AttributeClass, TStrongObjectPtr(NewObject<UAttribute>(this, AttributeClass)));
		}
	}
	void SetUpTickerManager();

	virtual void OnAbilityAdded(const FName Key, UDynamicAbility* Ability, const UObject* Adder);
	virtual bool ValidateAbilityAddition(const FName Key, const TSubclassOf<UDynamicAbility>& AbilityClass, const UObject* Adder) const;

	virtual void OnAbilityActivated(UDynamicAbility* Ability, const UObject* Activator);
	
	virtual bool UpdateAbility(const FName& Key, const float DeltaTime);

	virtual bool ValidateSlideChange(const FAbilitySlideSettings& SlideSettings) const;
	virtual bool OnAbilitySlideChanged(UDynamicAbility* Ability, const FGameplayTag& SlideName);
	
	virtual bool DisableAbility(UDynamicAbility* Ability, const EDisableType& DisableType, const UObject* Disabler, const FGameplayTag& DisableReason);
	virtual void OnAbilityDisabled(UDynamicAbility* Ability, const EDisableType& DisableType, const UObject* Disabler, const FGameplayTag& DisableReason);

	FORCEINLINE virtual bool FindAndSetAbilitySettings(const FName& Key, UDynamicAbility* Ability) const
	{
		if (!Ability)
		{
			UE_LOG(LogDynamicAbilitySystem, Fatal, TEXT("Attempted to find and set ability settings, but ability was invalid"));
			return false;
		}
		if (Ability->bMustSearchSettings)
		{
			Ability->AbilitySettings = *AbilitiesSettings->FindRow<FDynamicAbilitySettings>(Key, TEXT("Cannot find settings for ability"));
			return true;
		}
		return false;
	}

#if WITH_TOUCH
	TWeakObjectPtr<UTouchManager> TouchManager;
#endif
#if WITH_ROTO
	TWeakObjectPtr<ARotoCameraManager> RotoManager;
#endif
	FGameplayTagContainer OwnedTags;
	TMap<FName, TWeakObjectPtr<UObject>> ContextObjects;
	TMap<TSubclassOf<UAttribute>, TStrongObjectPtr<UAttribute>> Attributes;
	TMap<FName, TStrongObjectPtr<UDynamicAbility>> CurrentAbilities;
public:
	FORCEINLINE const TMap<FName, TStrongObjectPtr<UDynamicAbility>>& GetAbilities() const { return CurrentAbilities; }
	FORCEINLINE const FGameplayTagContainer& GetOwnedTags() { return OwnedTags; }
	
	UPROPERTY(BlueprintAssignable)
	FOnAddedAbility OnAddedAbility;
	UPROPERTY(BlueprintAssignable)
	FOnRemovedAbility OnRemovedAbility;

	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool AddAbility(const TSubclassOf<UDynamicAbility> AbilityClass, const UObject* Adder)
	{
		return AddAbility(FindAbilityCDO(AbilityClass)->AbilityName, AbilityClass, Adder);
	}
	virtual bool AddAbility(const FName Key, const TSubclassOf<UDynamicAbility>& AbilityClass, const UObject* Adder);

	UFUNCTION(BlueprintCallable)
	bool RemoveAbility(const FName Key, const UObject* Remover);
	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool RemoveAbilityByClass(const TSubclassOf<UDynamicAbility>& AbilityClass, const UObject* Remover)
	{
		return RemoveAbility(FindAbilityCDO(AbilityClass)->AbilityName, Remover);
	}

	UFUNCTION(BlueprintCallable)
	bool ActivateAbility(const FName Key, const UObject* Activator);
	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool ActivateAbilityByClass(const TSubclassOf<UDynamicAbility>& AbilityClass, const UObject* Activator)
	{
		return ActivateAbility(FindAbilityCDO(AbilityClass)->AbilityName, Activator);
	}

	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool ForcedAbilityDisable(const FName Key, const UObject* Disabler, const FGameplayTag& DisableReason)
	{
		if (const auto AbilityStorage = CurrentAbilities.Find(Key))
		{
			return DisableAbility(AbilityStorage->Get(), EDisableType::Forced, Disabler, DisableReason);
		}
		return false;
	}
	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool ForcedAbilityDisableByClass(const TSubclassOf<UDynamicAbility>& AbilityClass, const UObject* Disabler, const FGameplayTag& DisableReason)
	{
		return ForcedAbilityDisable(FindAbilityCDO(AbilityClass)->AbilityName, Disabler, DisableReason);
	}

	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool ChangeAbilitySlide(const FName Key, const FGameplayTag& SlideName)
	{
		if (const auto AbilityStorage = CurrentAbilities.Find(Key))
		{
			return ChangeAbilitySlide(AbilityStorage->Get(), SlideName);
		}
		return false;
	}
	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool ChangeAbilitySlideByClass(const TSubclassOf<UDynamicAbility>& AbilityClass, const FGameplayTag& SlideName)
	{
		for (const auto& AbilityData : CurrentAbilities)
		{
			if (AbilityData.Value->GetClass() != AbilityClass) return false;
			return ChangeAbilitySlide(AbilityData.Value.Get(), SlideName);
		}
		return false;
	}
	
	bool ChangeAbilitySlide(UDynamicAbility* Ability, const FGameplayTag& SlideName);

	UFUNCTION(Blueprintable)
	void AddAbilityInput(const FGameplayTag& InputKey, const ETriggerEvent& Event);

	UFUNCTION(Blueprintable)
	void AddAbilityInputVector(const FVector& WorldVector, const FGameplayTag& InputKey, const ETriggerEvent& Event);
	
	
	UFUNCTION(Blueprintable)
	FORCEINLINE bool AddContextObject(const FName Key, UObject* ContextObject)
	{
		if (ContextObjects.Contains(Key))
		{
			UE_LOG(LogDynamicAbilitySystem, Error, TEXT("Cannot add context object for key '%s' because this key is already in use"), *Key.ToString());
			return false;
		}
		ContextObjects.Add(Key, TWeakObjectPtr(ContextObject));
		return true;
	}
	UFUNCTION(Blueprintable)
	FORCEINLINE bool RemoveContextObject(const FName Key)
	{
		if (!ContextObjects.Contains(Key))
		{
			UE_LOG(LogDynamicAbilitySystem, Error, TEXT("Cannot remove context object for key '%s' because it does not exist"), *Key.ToString());
			return false;
		}
		ContextObjects.Remove(Key);
		return true;
	}

	UFUNCTION(Blueprintable)
	UAttribute* GetAttribute(const UDynamicAbility* Ability, const TSubclassOf<UAttribute>& AttributeClass);

	UFUNCTION(Blueprintable)
	UObject* GetContextObject(const FName Key, const UDynamicAbility* Ability, const bool bConst);

	UFUNCTION(Blueprintable)	
	FORCEINLINE bool IsAbilityGetSystem(const UDynamicAbility* Ability) const
	{
		if (IsAbilityHasAllAllows(Ability)) return true;
		return GettingSystemRefAllows.Contains(Ability->GetClass());
	}

#if WITH_TOUCH
	FORCEINLINE UTouchManager* GetTouchSystem(const UDynamicAbility* Ability) const
	{
		if (IsAbilityHasAllAllows(Ability) || GettingTouchSystemRefAllows.Contains(Ability->GetClass())) return TouchManager.Get();
		return nullptr;
	}
#endif
#if WITH_ROTO
	FORCEINLINE ARotoCameraManager* GetRotoSystem(const UDynamicAbility* Ability) const
	{
		if (IsAbilityHasAllAllows(Ability) || RotoTouchSystemRefAllows.Contains(Ability->GetClass())) return RotoManager.Get();
		return nullptr;
	}
#endif

	UFUNCTION(Blueprintable)	
	FORCEINLINE bool IsAbilityHasAllAllows(const UDynamicAbility* Ability) const
	{
		return AttributesWithAllAllows.Contains(Ability->GetClass()) && Ability->bGetAllAllows;
	}
protected:
	void OverrideAbilities(const UDynamicAbility* Overrider);	
	
	FORCEINLINE static bool CheckAbilitySlide(const UDynamicAbility* Ability, const FGameplayTag& SlideName)
	{
		return Ability ? Ability->CurrentSlideType == SlideName : false;
	}
	FORCEINLINE static const UDynamicAbility* FindAbilityCDO(const TSubclassOf<UDynamicAbility>& AbilityClass)
	{ 
		if (const auto* AbilityCDO = AbilityClass->GetDefaultObject<UDynamicAbility>()) return AbilityCDO;
		UE_LOG(LogDynamicAbilitySystem, Error, TEXT("Failed to find CDO for ability '%s'."), *AbilityClass->GetName());
		return nullptr;
	}
	
	static const FAbilitySlideSettings* FindSlideData(const UDynamicAbility* Ability, const ESlideSettingsType& SettingsType = ESlideSettingsType::Auto,
		const bool bEnsureResult = false, const FGameplayTag& SlideName = FGameplayTag::EmptyTag);
};