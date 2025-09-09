
#pragma once

class ZEON_API FTraceUtility
{
public:
	
	static bool ExecuteCameraSphereTrace(const APawn* Character, FHitResult& HitResult, const float SphereTraceDistance = 150.f, const float SphereTraceRadius = 5.f)
	{
		check(Character);
		if (const APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
		{
			int32 ScreenSizeX, ScreenSizeY;
			FVector StartLocation, WorldDirection;

			PlayerController->GetViewportSize(ScreenSizeX, ScreenSizeY);
			const FVector2D ScreenCenter(ScreenSizeX / 2.0f, ScreenSizeY / 2.0f);
			PlayerController->DeprojectScreenPositionToWorld(ScreenCenter.X, ScreenCenter.Y, StartLocation, WorldDirection);
			
			const FVector EndLocation = StartLocation + WorldDirection * SphereTraceDistance;
			const FCollisionQueryParams TraceParams(FName(TEXT("SphereTrace")), false, Character);
			const auto World = Character->GetWorld();
			
			if (bShowDebug)
			{
				DrawDebugLine(World, StartLocation, EndLocation, FColor::Green, false, 2.0f, 0, 1.0f);
				DrawDebugSphere(World, StartLocation, SphereTraceRadius, 12, FColor::Blue, false, 2.0f);
				DrawDebugSphere(World, EndLocation, SphereTraceRadius, 12, FColor::Red, false, 2.0f);
			}

			const auto bHit = World->SweepSingleByChannel(HitResult, StartLocation, EndLocation, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(SphereTraceRadius), TraceParams);
			if (bHit && bShowDebug && HitResult.GetActor())
			{
				DrawDebugSphere(World, HitResult.Location, SphereTraceRadius, 12, FColor::Yellow, false, 2.0f);
			}
			return bHit;
		}
		return false;
	}


	static bool bShowDebug; 
};
