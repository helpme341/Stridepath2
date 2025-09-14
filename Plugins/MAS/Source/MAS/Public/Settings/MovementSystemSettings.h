
#pragma once

#include "CoreMinimal.h"
#include "MovementSettingsSet.h"
#include "MovementSystemSettings.generated.h"

UCLASS(BlueprintType)
class MAS_API UMovementSystemSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Settings, meta=(AllowPrivateAccess = "true"))
	FMovementSettingsSet DefaultSettings;
};