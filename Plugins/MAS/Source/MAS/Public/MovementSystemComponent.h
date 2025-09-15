
#pragma once

#include "CoreMinimal.h"
#include "Utility/Invoker.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Settings/MovementSettingsSet.h"
#include "MovementSystemComponent.generated.h"


class UMovementSystemSettings;

UCLASS(Blueprintable, BlueprintType, ClassGroup=(MAS), meta=(BlueprintSpawnableComponent))
class MAS_API UMovementSystemComponent : public UPawnMovementComponent
{
	GENERATED_BODY()

	friend class UMovementAbilitySystem;
	friend class UMovementAbility;
	
	virtual bool ShouldSkipUpdate(float DeltaTime) const override
	{
		return Super::ShouldSkipUpdate(DeltaTime);
	}

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void BeginPlay() override;
	
	UPROPERTY()
	TObjectPtr<UCapsuleComponent> CapsuleComp;
public:

	UFUNCTION(BlueprintCallable, Category="Movement")
	FORCEINLINE void AddImpulse(const FVector& WorldVector, const bool bOverride = false)
	{
		if (bOverride) Velocity = WorldVector;
		else Velocity += WorldVector;
	}

protected:
	UMovementSystemComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Movement)
	TObjectPtr<UMovementSystemSettings> MovementSettings;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category=Movement)
	FMovementSettingsSet DynamicSet;

	void PrePhysics_UpdateGroundState();
	void ApplyInputAcceleration(const float DeltaTime);
	void ApplyFrictionAndBraking(const float DeltaTime);
	void ApplyGravity(const float DeltaTime);
	void MoveWithCollisions(const float DeltaTime);

	bool TryStepUp(const FHitResult& BlockingHit, const FVector& HDelta, const float MaxStepHeight);
	bool PredictiveGround(FHitResult& OutHit, const float DeltaTime) const;
	bool SweepSingleByChannel(FHitResult& OutHit, const FVector& Start, const FVector& End) const;
	bool ShortGroundContact(FHitResult& OutHit) const;
	bool SweepForGround(FHitResult& OutHit) const;
	
	bool IsWalkable(const FVector& Normal) const;
	
	FORCEINLINE static float GetCapsuleRadius(const UCapsuleComponent* CapsuleComponent)
	{
		if (CapsuleComponent) return CapsuleComponent->GetScaledCapsuleRadius();
		return 34.f;
	}
	FORCEINLINE static float GetCapsuleHalfHeight(const UCapsuleComponent* CapsuleComponent)
	{
		if (CapsuleComponent) return CapsuleComponent->GetScaledCapsuleHalfHeight();
		return 88.f;
	}

	FVector LastInputDir;
	FHitResult GroundHit;
	bool bIsOnGround = true;
	TInvoker<void()> ApplyMovementEdits;
};