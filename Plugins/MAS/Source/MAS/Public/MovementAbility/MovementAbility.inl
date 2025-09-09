
#pragma once

#include "MovementAbility.h"
#include "MovementAbilitySystem/MovementAbilitySystem.h"

template <EMovementSettingsType Key, typename T>
bool UMovementAbility::SetMovementSetting(const T& Value, const uint8 Priority) const
{
	return MovementAbilitySystem->SetMovementSetting<Key>(this, Priority, Value);
}

template <EMovementSettingsType Key>
bool UMovementAbility::SetMovementSettingToDefault(const uint8 Priority) const
{
	return MovementAbilitySystem->SetMovementSettingToDefault<Key>(this, Priority);
}

template <EMovementSettingsType Key, typename T>
const T& UMovementAbility::GetMovementSetting(const bool bDefaultSetting) const
{
	return MovementAbilitySystem->GetMovementSetting<Key>(bDefaultSetting);
}

FORCEINLINE UMovementSystemComponent* UMovementAbility::GetMutableMovementSystem() const
{
	if (MovementAbilitySystem->CanAbilityGetMovementSystem(this)) return MovementSystem.Get();
	return nullptr;
}

FORCEINLINE UMovementAbilitySystem* UMovementAbility::GetMutableMovementAbilitySystem() const
{
	if (MovementAbilitySystem->CanAbilityGetSystem(this)) return MovementAbilitySystem.Get();
	return nullptr;
}

FORCEINLINE const FHitResult& UMovementAbility::GetGroundHitResult() const
{
	return MovementAbilitySystem->MovementSystemComponent->GroundHit;
}

FORCEINLINE bool UMovementAbility::GetIsOnGround() const
{
	return MovementAbilitySystem->MovementSystemComponent->bIsOnGround;
}

FORCEINLINE bool UMovementAbility::ApplyMovementImpulse(const FVector& Direction, const bool bOverride, const uint8 Priority) const
{
	if (bOverride) return MovementAbilitySystem->SetMovementSetting<Velocity>(this, Priority, Direction);

	const auto CurrentVelocity = MovementAbilitySystem->GetMovementSetting<Velocity, FVector>(false);
	return MovementAbilitySystem->SetMovementSetting<Velocity>(this, Priority, CurrentVelocity + Direction);
}
