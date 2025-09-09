
#pragma once

#include "CoreMinimal.h"
#include "MovementSystemComponent.h"
#include "AbilitySystem/DynamicAbility.h"
#include "MovementAbility.generated.h"

USTRUCT(BlueprintType)
struct FMovementAbilitySettings : public FTableRowBase
{
	GENERATED_BODY()
};

class UMovementSystemComponent;

UCLASS(Blueprintable, BlueprintType, ClassGroup=(MAS))
class MAS_API UMovementAbility : public UDynamicAbility
{
	GENERATED_BODY()

	friend class UMovementAbilitySystem;
	friend class FMASEditorModule;

	UPROPERTY()
	TObjectPtr<UMovementSystemComponent> MovementSystem;

	UPROPERTY()
	TObjectPtr<UMovementAbilitySystem> MovementAbilitySystem;

protected:
	FORCEINLINE const UMovementSystemComponent* GetMovementSystem() const { return MovementSystem.Get(); }
	FORCEINLINE const UMovementAbilitySystem* GetMovementAbilitySystem() const { return MovementAbilitySystem.Get(); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementSettings")
	FMovementAbilitySettings MovementAbilitySettings;

	FORCEINLINE UMovementSystemComponent* GetMutableMovementSystem() const;
	FORCEINLINE UMovementAbilitySystem* GetMutableMovementAbilitySystem() const;

	template<EMovementSettingsType Key, typename T>
	bool SetMovementSetting(const T& Value, const uint8 Priority = 0) const;

	template<EMovementSettingsType Key>
	bool SetMovementSettingToDefault(const uint8 Priority = 0) const;

	template <EMovementSettingsType Key, typename T>
	const T& GetMovementSetting(const bool bDefaultSetting) const;

	bool ApplyMovementImpulse(const FVector& Direction, const bool bOverride, const uint8 Priority) const;

	const FHitResult& GetGroundHitResult() const;
	bool GetIsOnGround() const;
};