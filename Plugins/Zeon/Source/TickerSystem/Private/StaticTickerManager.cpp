
#include "StaticTickerManager.h"
#include "UObject/UObjectGlobals.h"
#include "Utility/PauseManager.h"
#include "Utility/ZeonUtilits.h"
#include "TickerModule.h"

DEFINE_LOG_CATEGORY(LogStaticTicker);

FStaticTickerManager::FStaticTickerManager()
{
	GameStartedDelegateHandle = FZeonUtil::OnWorldBeginPlay.AddRaw(this, &FStaticTickerManager::OnGameStarted);
	GameEndedDelegateHandle = FWorldDelegates::OnWorldBeginTearDown.AddRaw(this, &FStaticTickerManager::OnGameEnded);
	GamePauseDelegateHandle = FPauseManager::OnGamePause.AddRaw(this, &FStaticTickerManager::OnGamePaused);
	TryAutoModifyTickerState(ETickerStateType::Init);
}

FStaticTickerManager::~FStaticTickerManager()
{
	EndTicker();
	FZeonUtil::OnWorldBeginPlay.Remove(GameStartedDelegateHandle);
	FWorldDelegates::OnWorldBeginTearDown.Remove(GameEndedDelegateHandle);
	FPauseManager::OnGamePause.Remove(GamePauseDelegateHandle);
	TickerModules.Empty();
}

bool FStaticTickerManager::Tick(float DeltaTime)
{
	for (auto& [_, Module] : TickerModules)
	{
		if (Module->bTickInPauseDisabled && bLastPauseState) continue;
		Module->Tick(DeltaTime);
	}
	return !CleanupManager(DeltaTime);
}

bool FStaticTickerManager::CleanupManager(float DeltaTime)
{
	if (!TickHandle.IsValid() || !bUseCleanupSystem) return false;
	
	CurrentCleanupTime += DeltaTime;
	if (CleanupRate <= CurrentCleanupTime)
	{
		CurrentCleanupTime = 0.f;
		if (!DoesRequireTicker(nullptr))
		{
			UE_LOG(LogStaticTicker, Error, TEXT("Ticker leak: running without active tasks, forcibly stopped."));
			return true;
		}
	}
	return false;
}

bool FStaticTickerManager::DoesRequireTicker(const FTickerModule* IgnoreModule) const
{
	if (TickerModules.IsEmpty()) return false;
	for (auto& ModuleData : TickerModules)
	{
		if (const auto Module = ModuleData.Value.Get(); Module->NeedUpdate())
		{
			if (!IgnoreModule) return true;
			if (ModuleData.Key != IgnoreModule->ModuleName) return true;
		}
	}
	return false;
}

void FStaticTickerManager::TryStartTicker()
{
	if (TickHandle.IsValid())
	{
		UE_LOG(LogStaticTicker, Warning, TEXT("Cannot start ticker because it is already active"));
		return;
	}
	CurrentCleanupTime = 0.f;
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FStaticTickerManager::Tick), GlobalTickerUpdateRate);
}

void FStaticTickerManager::TryEndTicker(const FTickerModule* Module) const
{
	check(Module)
	if (DoesRequireTicker(Module))
	{
		UE_LOG(LogStaticTicker, Warning, TEXT("Cannot disable ticker because module '%s' is using it"), *Module->ModuleName.ToString());
		return;
	}
	EndTicker();
}

bool FStaticTickerManager::EndTicker() const
{
	if (!TickHandle.IsValid())
	{
		UE_LOG(LogStaticTicker, Warning, TEXT("Cannot disable ticker because it is already disabled"));
		return false;
	}
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	return !TickHandle.IsValid();
}

void FStaticTickerManager::OnGameStarted(EWorldType::Type /*WorldType*/)
{
	TryAutoModifyTickerState(ETickerStateType::BeginPlay);
	for (const auto& ModuleData : TickerModules) ModuleData.Value->OnGameStarted();
}

void FStaticTickerManager::OnGameEnded(UWorld* /*World*/)
{
	TryAutoModifyTickerState(ETickerStateType::EndPlay);
	for (const auto& ModuleData : TickerModules) ModuleData.Value->OnGameEnded();
}

void FStaticTickerManager::OnGamePaused(bool bPaused)
{
	TryAutoModifyTickerState(bPaused ? ETickerStateType::GamePaused : ETickerStateType::GameUnPaused);
	for (const auto& ModuleData : TickerModules)
	{
		ModuleData.Value->bIsGamePaused = bPaused;
		bPaused ? ModuleData.Value->OnGamePaused() : ModuleData.Value->OnGameUnPaused();
	}
}

bool FStaticTickerManager::RegisterTickerModule(FTickerModule* Module)
{
	check(Module)
	if (TickerModules.Contains(Module->ModuleName))
	{
		UE_LOG(LogStaticTicker, Warning, TEXT("Cannot add module '%s' because it is already added"), *Module->ModuleName.ToString());
		return false;	
	}
	Module->OwnerManager = this;
	TickerModules.Add(Module->ModuleName, TUniquePtr<FTickerModule>(std::move(Module)));
	return true;
}

FTickerModule* FStaticTickerManager::GetTickerModuleMutable(const FName ModuleName)
{
	if (!TickerModules.Contains(ModuleName))
	{
		UE_LOG(LogStaticTicker, Warning, TEXT("Cannot find module: %s"), *ModuleName.ToString());
		return nullptr;
	}
	return TickerModules.Find(ModuleName)->Get();
}