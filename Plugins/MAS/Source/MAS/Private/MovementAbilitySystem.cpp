
#include "MovementAbilitySystem/MovementAbilitySystem.h"
#include "MovementSystemComponent.h"
#include "MovementAbility/MovementAbility.h"

DEFINE_LOG_CATEGORY(LogMovementAbilitySystem);

UMovementAbilitySystem::UMovementAbilitySystem()
{
	MovementSystemComponent = CreateDefaultSubobject<UMovementSystemComponent>("MovementComponent");
	MovementSystemComponent->ApplyMovementEdits.Bind(this, &UMovementAbilitySystem::ApplyDelayedMovementEdits);
}

void UMovementAbilitySystem::OnAbilityAdded(const FName Key, UDynamicAbility* Ability, const UObject* Adder)
{
	if (UMovementAbility* MovementAbility = CastChecked<UMovementAbility>(Ability))
	{
		MovementAbility->MovementAbilitySystem = this;
		MovementAbility->MovementSystem = MovementSystemComponent;
		// Movement abilities setup...
	}
	Super::OnAbilityAdded(Key, Ability, Adder);
}

bool UMovementAbilitySystem::ValidateAbilityAddition(const FName Key, const TSubclassOf<UDynamicAbility>& AbilityClass, const UObject* Adder) const
{
	if (Super::ValidateAbilityAddition(Key, AbilityClass, Adder))
	{
		if (!AbilityClass->IsChildOf<UMovementAbility>())
		{
			UE_LOG(LogMovementAbilitySystem, Error, TEXT("Failed to add ability class '%s': UMovementAbilitySystem only supports abilities derived from UMovementAbility."), *AbilityClass->GetName());
			return false;
		}
		return true;
	}
	return false;
}

bool UMovementAbilitySystem::FindAndSetAbilitySettings(const FName& Key, UDynamicAbility* Ability) const
{
	if (Super::FindAndSetAbilitySettings(Key, Ability))
	{
		if (UMovementAbility* MovementAbility = CastChecked<UMovementAbility>(Ability))
		{
			MovementAbility->MovementAbilitySettings = *MovementAbilitiesSettings->FindRow<FMovementAbilitySettings>(Key, TEXT("Cannot find settings for movement ability"));
			return true;
		}
	}
	return false;
}

void UMovementAbilitySystem::ApplyDelayedMovementEdits()
{
	for (auto& EditType : DelayedMovementEdits)
	{
		uint8 MaxEditPriority = 0;
		TInvoker<void()>* Edit = nullptr;
		
		for (auto& MovementEdit : EditType.Value)
		{
			const auto Priority = MovementEdit.Key;
			if (Priority > MaxEditPriority)
			{
				MaxEditPriority = Priority;
				Edit = &MovementEdit.Value;
			}
		}
		if (Edit) (*Edit)();
	}
}