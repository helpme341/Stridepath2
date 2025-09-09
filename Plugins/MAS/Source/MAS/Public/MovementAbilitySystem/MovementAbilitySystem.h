
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/DynamicAbilitySystem.h"
#include "MovementAbilitySystem.generated.h"

struct FMovementSettingsSet;
enum EMovementSettingsType : uint8;
class UMovementAbility;
class UMovementSystemComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogMovementAbilitySystem, Log, All);

USTRUCT(BlueprintType)
struct FMovementTypeContainer : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TSet<TEnumAsByte<EMovementSettingsType>> Types;
};


UCLASS(Blueprintable, BlueprintType, ClassGroup=(MAS), meta=(BlueprintSpawnableComponent))
class MAS_API UMovementAbilitySystem : public UDynamicAbilitySystem
{
	GENERATED_BODY()

	friend class FMASEditorModule;
	friend class UMovementAbility;

protected:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	TObjectPtr<UDataTable> MovementAbilitiesSettings;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "MovemntComponent")
	TObjectPtr<UMovementSystemComponent> MovementSystemComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SecuritySettings")
	TSet<TSubclassOf<UMovementAbility>> GettingMovementSystemRefAllows;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SecuritySettings")
	TMap<TSubclassOf<UMovementAbility>, FMovementTypeContainer> AbilitiesMovementSettingsAllows;

	virtual bool ValidateAbilityAddition(const FName Key, const TSubclassOf<UDynamicAbility>& AbilityClass, const UObject* Adder) const override;
	virtual bool FindAndSetAbilitySettings(const FName& Key, UDynamicAbility* Ability) const override;

	virtual void OnAbilityAdded(const FName Key, UDynamicAbility* Ability, const UObject* Adder) override;

	bool IsAbilityHasAccessToSetting(const TSubclassOf<UMovementAbility>& AbilityClass, const EMovementSettingsType& SettingsType);
	bool CanAbilityGetMovementSystem(const UDynamicAbility* Ability) const;

	void ApplyDelayedMovementEdits();
	
	template<EMovementSettingsType Key, typename T>
	bool SetMovementSetting(const UMovementAbility* Ability, uint8 Priority, const T& Value);

	template<EMovementSettingsType Key>
	bool SetMovementSettingToDefault(const UMovementAbility* Ability, uint8 Priority);

	template<EMovementSettingsType Key, typename T>
	const T& GetMovementSetting(const bool bDefaultSetting = false) const;

	TMap<EMovementSettingsType, TMap<uint8, TInvoker<void()>>> DelayedMovementEdits;
public:
	UMovementAbilitySystem();
};