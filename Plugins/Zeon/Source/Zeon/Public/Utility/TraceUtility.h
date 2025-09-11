
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

	static void DrawDebugSweep(const UWorld* World, const FVector& Start, const FVector& End, const FCollisionShape& Shape,
		const bool bHit, const float Duration = 2.f,const int32 Segments = 16)
	{
		check(World)
		if (!bShowDebug) return;

	    const FColor Color = bHit ? FColor::Green : FColor::Red;
	    const FVector Delta = End - Start;
	    const float Distance = Delta.Size();
	    const FVector Dir = Distance > KINDA_SMALL_NUMBER ? (Delta / Distance) : FVector::UpVector;
	    const FQuat Orient = FRotationMatrix::MakeFromZ(Dir).ToQuat();

	    DrawDebugLine(World, Start, End, Color, /*bPersistentLines*/ false, Duration, /*DepthPriority*/0, /*Thickness*/ 1.5f);

	    if (Shape.IsSphere())
	    {
	        const float R = Shape.GetSphereRadius();

	        // Сферы в начале и в конце
	        DrawDebugSphere(World, Start, R, Segments, Color, false, Duration);
	        DrawDebugSphere(World, End,   R, Segments, Color, false, Duration);

	        // «Растянутая» капсула эквивалентна сферическому свипу
	        const float Half = Distance * 0.5f;
	        const FVector Mid = (Start + End) * 0.5f;
	        DrawDebugCapsule(World, Mid, Half, R, Orient, Color, false, Duration);
	    }
	    else if (Shape.IsCapsule())
	    {
	        float HalfHeight = Shape.GetCapsuleHalfHeight();
	        const float Radius = Shape.GetCapsuleRadius();

	        // Капсулы в начале и конце
	        DrawDebugCapsule(World, Start, HalfHeight, Radius, Orient, Color, false, Duration);
	        DrawDebugCapsule(World, End,   HalfHeight, Radius, Orient, Color, false, Duration);

	        // «Растянутая» капсула: добавляем половину дистанции к halfHeight
	        const float SweptHalf = HalfHeight + Distance * 0.5f;
	        const FVector Mid = (Start + End) * 0.5f;
	        DrawDebugCapsule(World, Mid, SweptHalf, Radius, Orient, Color, false, Duration);
	    }
	    else if (Shape.IsBox())
	    {
	        const FVector Ext = Shape.GetBox();
	        const FQuat NoRot = FQuat::Identity;

	        // Боксы в начале и в конце
	        DrawDebugBox(World, Start, Ext, NoRot, Color, false, Duration);
	        DrawDebugBox(World, End,   Ext, NoRot, Color, false, Duration);

	        // Вершины локального бокса
	        const FVector V[8] = {
	            {+Ext.X, +Ext.Y, +Ext.Z}, {+Ext.X, +Ext.Y, -Ext.Z},
	            {+Ext.X, -Ext.Y, +Ext.Z}, {+Ext.X, -Ext.Y, -Ext.Z},
	            {-Ext.X, +Ext.Y, +Ext.Z}, {-Ext.X, +Ext.Y, -Ext.Z},
	            {-Ext.X, -Ext.Y, +Ext.Z}, {-Ext.X, -Ext.Y, -Ext.Z}
	        };
	        for (int i = 0; i < 8; ++i) DrawDebugLine(World, Start + V[i], End + V[i], Color.WithAlpha(150), false, Duration, 0, 0.5f);

	        // Стрелка направления для ясности
	        DrawDebugDirectionalArrow(World, Start, End, 20.f, Color, false, Duration, 0, 1.5f);
	    }
	}


	static bool bShowDebug; 
};
