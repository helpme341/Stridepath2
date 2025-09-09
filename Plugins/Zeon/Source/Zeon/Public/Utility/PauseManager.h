
#pragma once

#include "CoreMinimal.h"
#include "ZeonUtilits.h"
#include "Kismet/GameplayStatics.h"

class ZEON_API FPauseManager
{
	static TUniquePtr<FPauseManager> Instance;

public:

	static void Initialize()
	{
		if (!Instance) Instance = MakeUnique<FPauseManager>();
	}

	
	static void Shutdown()
	{
		Instance.Reset();
		OnGamePause.Clear();
	}

	static FPauseManager& Get()
	{
		check(Instance)
		return *Instance;
	}
public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnGamePause, bool /*bPaused*/);
	static FOnGamePause OnGamePause;

	
	FORCEINLINE static bool PauseGame(const bool bPaused)
	{
		const auto World = FZeonUtil::FindWorld();
		return PauseGame(World, bPaused);
	}
	FORCEINLINE static bool PauseGame(const bool bPaused, const TSet<EWorldType::Type>& WorldTypes = FZeonUtil::GetDefaultWorldTypes())
	{
		const auto World = FZeonUtil::FindWorld(WorldTypes);
		return PauseGame(World, bPaused);
	}
	FORCEINLINE static bool PauseGame(const UObject* ObjectContext, const bool bPaused)
	{
		const auto World = GEngine->GetWorldFromContextObjectChecked(ObjectContext);
		return PauseGame(World, bPaused);
	}
	static bool PauseGame(const UWorld* World, const bool bPaused)
	{
		if (UGameplayStatics::IsGamePaused(World) != bPaused)
		{
			OnGamePause.Broadcast(bPaused);
			return UGameplayStatics::SetGamePaused(World, bPaused);
		}
		return false;
	}

	FORCEINLINE static bool IsGamePaused()
	{
		const auto World = FZeonUtil::FindWorld();
		return UGameplayStatics::IsGamePaused(World);
	}
	FORCEINLINE static bool IsGamePaused(const TSet<EWorldType::Type>& WorldTypes = FZeonUtil::GetDefaultWorldTypes())
	{
		const auto World = FZeonUtil::FindWorld(WorldTypes);
		return UGameplayStatics::IsGamePaused(World);
	}
	FORCEINLINE static bool IsGamePaused(const UObject* ObjectContext)
	{
		const auto World = GEngine->GetWorldFromContextObjectChecked(ObjectContext);
		return UGameplayStatics::IsGamePaused(World);
	}
	FORCEINLINE static bool IsGamePaused(const UWorld* World)
	{
		return UGameplayStatics::IsGamePaused(World);
	}
};