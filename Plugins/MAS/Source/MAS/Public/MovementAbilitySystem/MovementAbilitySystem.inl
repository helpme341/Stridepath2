
#pragma once

#include "MovementAbilitySystem.h"
#include "MovementAbility/MovementAbility.h"
#include "Settings/MovementSystemSettings.h"

template <EMovementSettingsType Key, typename T>
bool UMovementAbilitySystem::SetMovementSetting(const UMovementAbility* Ability, uint8 Priority, const T& Value)
{
	if (!IsAbilityHasAccessToSetting(Ability->GetClass(), Key)) return false;
	DelayedMovementEdits.Add(Key, TPair(Priority, [this, Value]
	{
		MovementSystemComponent->DynamicSet.GetValue<Key>() = Value;
	}));
	return true;
}

template <EMovementSettingsType Key>
bool UMovementAbilitySystem::SetMovementSettingToDefault(const UMovementAbility* Ability, uint8 Priority)
{
	if (!IsAbilityHasAccessToSetting(Ability->GetClass(), Key)) return false;
	DelayedMovementEdits.Add(Key, TPair(Priority, [this]
	{
		MovementSystemComponent->DynamicSet.GetValue<Key>() = MovementSystemComponent->MovementSettings->DefaultSettings.GetValue<Key>();
	}));
	return true;
}

template <EMovementSettingsType Key, typename T>
const T& UMovementAbilitySystem::GetMovementSetting(const bool bDefaultSetting) const
{
	if (bDefaultSetting) return MovementSystemComponent->MovementSettings->DefaultSettings.GetValue<Key>();
	return MovementSystemComponent->DynamicSet.GetValue<Key>();
}

FORCEINLINE bool UMovementAbilitySystem::IsAbilityHasAccessToSetting(const TSubclassOf<UMovementAbility>& AbilityClass,
														 const EMovementSettingsType& SettingsType)
{
	if (const auto AbilityAllows = AbilitiesMovementSettingsAllows.Find(AbilityClass))
	{
		if (!AbilityAllows->Types.Contains(SettingsType))
		{
			UE_LOG(LogMovementAbilitySystem, Warning, TEXT("Ability class '%s' does not have access to movement setting '%s'."), *AbilityClass->GetName(), *UEnum::GetValueAsString(SettingsType));
		}
		return true;
	}
	UE_LOG(LogMovementAbilitySystem, Error, TEXT("Access settings for movement setting '%s' were not found for ability class '%s'."), *UEnum::GetValueAsString(SettingsType), *AbilityClass->GetName());
	return false;
}

FORCEINLINE bool UMovementAbilitySystem::CanAbilityGetMovementSystem(const UDynamicAbility* Ability) const
{
	if (IsAbilityHasAllAllows(Ability)) return true;
	return GettingMovementSystemRefAllows.Contains(Ability->GetClass());
}