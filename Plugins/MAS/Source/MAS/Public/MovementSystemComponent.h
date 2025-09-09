
#pragma once

#include "CoreMinimal.h"
#include "MovementSettingsSet.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Utility/Invoker.h"
#include "MovementSystemComponent.generated.h"


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

public:

	UFUNCTION(BlueprintCallable, Category="Movement")
	FORCEINLINE void AddImpulse(const FVector& WorldVector, const bool bOverride = false)
	{
		if (bOverride) Velocity = WorldVector;
		else Velocity += WorldVector;
	}

protected:
	UMovementSystemComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FMovementSettingsSet DefaultSet;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FMovementSettingsSet DynamicSet;

	void SetOwnerCompVelocity();
	void SetIsOnGround(const bool bInIsOnGround);

	void PrePhysics_UpdateGroundState();
	void ApplyInputAcceleration(float DeltaTime);
	void ApplyFrictionAndBraking(float DeltaTime);
	void ApplyGravity(float DeltaTime);
	void MoveWithCollisions(float DeltaTime);
	
	bool SweepForGround(FHitResult& OutHit) const;
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

	FHitResult GroundHit;
	bool bIsOnGround = true;
	TInvoker<void()> ApplyMovementEdits;
};