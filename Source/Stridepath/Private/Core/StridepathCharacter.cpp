
#include "Core/StridepathCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Core/CharacterInputSettings.h"
#include "Core/CharacterInteractSettings.h"
#include "Evora/Public/ParkourMovementAbilitySystem.h"
#include "Interfaces/Interact.h"
#include "Utility/TraceUtility.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AStridepathCharacter::AStridepathCharacter()
{
	// Create a CapsuleComponent	
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>("CapsuleComponent");
	RootComponent = CapsuleComponent;
	CapsuleComponent->InitCapsuleSize(55.f, 96.0f);
		
	// Create a CameraComponent	
	CameraComponent = CreateDefaultSubobject<UCameraComponent>("FirstPersonCamera");
	CameraComponent->SetupAttachment(RootComponent);
	CameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f));
	CameraComponent->bUsePawnControlRotation = true;

	ParkourMovementAbilitySystem = CreateDefaultSubobject<UParkourMovementAbilitySystem>("ParkourMovementAbilitySystem");
}

void AStridepathCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	check(InputSettings);
	
	if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputSettings->DefaultMappingContext, 0);
		}
	}
}

void AStridepathCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumped
		EnhancedInputComponent->BindAction(InputSettings->JumpAction, ETriggerEvent::Started, this, &AStridepathCharacter::Jump);

		// Moving
		EnhancedInputComponent->BindAction(InputSettings->MoveAction, ETriggerEvent::Triggered, this, &AStridepathCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(InputSettings->LookAction, ETriggerEvent::Triggered, this, &AStridepathCharacter::Look);

		// Interact
		EnhancedInputComponent->BindAction(InputSettings->InteractAction, ETriggerEvent::Started, this, &AStridepathCharacter::Interact);
		EnhancedInputComponent->BindAction(InputSettings->InteractAction, ETriggerEvent::Triggered, this, &AStridepathCharacter::Interact);
		EnhancedInputComponent->BindAction(InputSettings->InteractAction, ETriggerEvent::Completed, this, &AStridepathCharacter::Interact);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AStridepathCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), MovementVector.Y);
	AddMovementInput(GetActorRightVector(), MovementVector.X);
}

void AStridepathCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void AStridepathCharacter::Jump(const FInputActionInstance& Instance)
{
	ParkourMovementAbilitySystem->AddAbilityInput(FGameplayTag::RequestGameplayTag(TEXT("Input.Jump"),true), Instance.GetTriggerEvent());
}

void AStridepathCharacter::Interact(const FInputActionInstance& Instance)
{
	check(InteractSettings);
	if (FHitResult HitResult; FTraceUtility::ExecuteCameraSphereTrace(this, HitResult, InteractSettings->InteractDistance, InteractSettings->InteractSize))
	{
		if (auto* InteractableActor = Cast<IInteract>(HitResult.GetActor()))
		{
			InteractableActor->Interact(FGameplayTag::EmptyTag, Instance.GetTriggerEvent(), this);
		}
	}
}
