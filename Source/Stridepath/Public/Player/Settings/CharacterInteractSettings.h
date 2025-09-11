
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CharacterInteractSettings.generated.h"

UCLASS(BlueprintType)
class STRIDEPATH_API UCharacterInteractSettings : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Interact, meta=(AllowPrivateAccess = "true"))
	float InteractDistance = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Interact, meta=(AllowPrivateAccess = "true"))
	float InteractSize = 200.f;
};