
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/DynamicAbility.h"
#include "JumpAbility.generated.h"

UCLASS(Blueprintable, ClassGroup = "DAS")
class DASTEST_API UJumpAbility : public UDynamicAbility
{ 
	GENERATED_BODY()

	
public:
	UJumpAbility()
	{
		AbilityName = "Jump Ability";
		AbilitySettings.BaseSlideSettings.SlideTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Jump"),true));
	}
};
