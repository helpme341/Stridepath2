
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/DynamicAbility.h"
#include "CrouchAbility.generated.h"


UCLASS(Blueprintable, ClassGroup = "DAS")
class DASTEST_API UCrouchAbility : public UDynamicAbility
{ 
	GENERATED_BODY()\


	virtual TOptional<FGameplayTag> UpdateAbility(float DeltaTime) override
	{
		UE_LOG(LogTemp, Display, TEXT("Updating Ability"));
		return TOptional<FGameplayTag>();
	}

public:
	UCrouchAbility()
	{
		AbilityName = "Crouch Ability";
		AbilitySettings.BaseSlideSettings.SlideTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Crouch"),true));
		AbilitySettings.BaseSlideSettings.ActivationDelay = 3.f;
		AbilitySettings.BaseSlideSettings.bTickEveryFrame = true;
		AbilitySettings.BaseSlideSettings.MaxActiveTime = 10.f;

		AbilitySettings.SlidesSettings.Add(FGameplayTag::RequestGameplayTag(TEXT("Ability.Crouch.Start"), true),
		FAbilitySlideSettings(
			3,
			2,
			false,
			10.f,
			FGameplayTagContainer(FGameplayTag::RequestGameplayTag(TEXT("Ability.Crouch.Start"),true))));

		AbilitySettings.SlidesSettings.Add(FGameplayTag::RequestGameplayTag(TEXT("Ability.Crouch.End"), true),
		FAbilitySlideSettings());
	}
};
