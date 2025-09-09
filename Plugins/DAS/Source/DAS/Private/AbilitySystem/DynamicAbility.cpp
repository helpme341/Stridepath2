
#include "AbilitySystem/DynamicAbility.h"
#include "AbilitySystem/DynamicAbilitySystem.h"

bool UDynamicAbility::ChangeSlide(const FGameplayTag& NewSlideType)
{
	return AbilitySystem->ChangeAbilitySlide(this, NewSlideType);
}

UAttribute* UDynamicAbility::GetAttribute(const TSubclassOf<UAttribute>& AttributeClass) const
{
	return AbilitySystem->GetAttribute(this, AttributeClass);
}

#if WITH_TOUCH
const UTouchManager* UDynamicAbility::GetTouchSystem() const
{
	if (AbilitySystem) return AbilitySystem->GetTouchSystem(this);
	return nullptr;
}

UTouchManager* UDynamicAbility::GetTouchSystemMutable() const
{
	if (AbilitySystem) return AbilitySystem->GetTouchSystem(this);
	return nullptr;
}
#endif

#if WITH_ROTO
const ARotoCameraManager* UDynamicAbility::GetRotoSystem() const
{
	if (AbilitySystem) return AbilitySystem->GetRotoSystem(this);
	return nullptr;
}

ARotoCameraManager* UDynamicAbility::GetRotoSystemMutable() const
{
	if (AbilitySystem) return AbilitySystem->GetRotoSystem(this);
	return nullptr;
}
#endif


UDynamicAbilitySystem* UDynamicAbility::GetAbilitySystemMutable() const
{
	if (AbilitySystem->IsAbilityGetSystem(this)) return AbilitySystem.Get();
	return nullptr;
}

const UObject* UDynamicAbility::GetContextObject(const FName& Key) const
{
	return AbilitySystem->GetContextObject(Key, this, true);
}

UObject* UDynamicAbility::GetMutableContextObject(const FName& Key) const
{
	return AbilitySystem->GetContextObject(Key, this, false);
}