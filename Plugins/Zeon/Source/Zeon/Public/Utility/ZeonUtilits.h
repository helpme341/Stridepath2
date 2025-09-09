
#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Delegates/Delegate.h"

class ZEON_API FZeonUtil
{
	static TUniquePtr<FZeonUtil> Instance;
	static FDelegateHandle PostWorldInitDelegateHandle;
	
	static void OnPostWorldInitialization(UWorld* World, const UWorld::InitializationValues /*IVS*/)
	{
		World->OnWorldBeginPlay.AddLambda([WorldType = World->WorldType]{ OnWorldBeginPlay.Broadcast(WorldType); });
	}

public:

	static void Initialize()
	{
		if (!Instance) Instance = MakeUnique<FZeonUtil>();
		PostWorldInitDelegateHandle = FWorldDelegates::OnPostWorldInitialization.AddStatic(&OnPostWorldInitialization);
	}
	
	static void Shutdown()
	{
		Instance.Reset();
		OnWorldBeginPlay.Clear();
		FWorldDelegates::OnPostWorldInitialization.Remove(PostWorldInitDelegateHandle);
	}

	static FZeonUtil& Get()
	{
		check(Instance)
		return *Instance;
	}


public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnWorldBeginPlay, EWorldType::Type /*WorldType*/);
	static FOnWorldBeginPlay OnWorldBeginPlay;
	

	FORCEINLINE static const TSet<EWorldType::Type>& GetDefaultWorldTypes()
	{
		static const TSet Types = { EWorldType::PIE, EWorldType::Game };
		return Types;
	}
	static UWorld* FindWorld(const TSet<EWorldType::Type>& WorldTypes = GetDefaultWorldTypes())
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (WorldTypes.Contains(Context.WorldType)) return Context.World();
		}
		return nullptr;
	}
};