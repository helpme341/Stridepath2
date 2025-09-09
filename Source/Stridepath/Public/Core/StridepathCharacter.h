
#pragma once
#include "GameFramework/Pawn.h"

#include "StridepathCharacter.generated.h"

class UCapsuleComponent;
class UParkourMovementAbilitySystem;
class UCameraComponent;
class UCharacterInteractSettings;
struct FInputActionInstance;
struct FInputActionValue;
class UCharacterInputSettings;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class STRIDEPATH_API AStridepathCharacter : public APawn
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UParkourMovementAbilitySystem> ParkourMovementAbilitySystem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UCharacterInputSettings> InputSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UCharacterInteractSettings> InteractSettings;
	
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Jump(const FInputActionInstance& Instance);
	void Interact(const FInputActionInstance& Instance);

	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
public:	
	AStridepathCharacter();
};