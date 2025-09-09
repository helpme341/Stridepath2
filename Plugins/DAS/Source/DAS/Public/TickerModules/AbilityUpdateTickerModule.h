
#pragma once

#include "CoreMinimal.h"
#include "TickerModule.h"
#include "Utility/Invoker.h"

struct FUpdateAbilityTickerData
{
	FUpdateAbilityTickerData(const float InUpdateRate, const float MaxActiveTime)
		: UpdateRate(InUpdateRate)
	   , MaxActiveTime(MaxActiveTime) {}
	
	float UpdateRate;
	float MaxActiveTime;
	float UpdateLoopRemainingTime = 0.0f;
	float RemainingTime = 0.0f;
};

class UDynamicAbility;

class DAS_API FAbilityUpdateTickerModule : public FTickerModule
{
	GENERATED_TICKER_BODY("AbilityUpdateTickerModule")
	
	using FAbilityUpdateInvoker = TInvoker<bool(const FName&, float)>;
	using FDisableAbilityInvoker = TInvoker<void(const FName&)>;

	virtual void Tick(float DeltaTime) override
	{
		TMap<FName, bool /* bCallDisableAbilityInvoker */> CompletedTasks;
		for (auto& TaskData : UpdateTasks)
		{
			const auto& Key = TaskData.Key;
			auto& Settings = TaskData.Value;

			Settings.RemainingTime += DeltaTime;
			Settings.UpdateLoopRemainingTime += DeltaTime;

			if (Settings.MaxActiveTime != 0.f && Settings.RemainingTime >= Settings.MaxActiveTime)
			{
				CompletedTasks.Add(Key, true);
				continue;
			}
			if (Settings.UpdateLoopRemainingTime >= Settings.UpdateRate)
			{
				Settings.UpdateLoopRemainingTime = 0.f;
 				if (!AbilityUpdateInvoker(Key, DeltaTime)) CompletedTasks.Add(Key, false);
			}
		}
		for (auto Key : CompletedTasks)
		{
			UpdateTasks.Remove(Key.Key);
			if (Key.Value) // вызываем DisableAbilityInvoker только тут, а не в for по UpdateTasks изо того что DisableAbilityInvoker может изменять UpdateTasks
			{
				DisableAbilityInvoker(Key.Key);
				TryEndTickerSave();
			}
		}
	}

	virtual bool NeedUpdate() const override
	{
		return !UpdateTasks.IsEmpty();
	}
	
	TMap<FName, FUpdateAbilityTickerData> UpdateTasks;
public:
	virtual ~FAbilityUpdateTickerModule() override
	{
		UpdateTasks.Empty();
	}
	
	FAbilityUpdateInvoker AbilityUpdateInvoker;	
	FDisableAbilityInvoker DisableAbilityInvoker;

	
	FORCEINLINE void StartAbilityUpdate(const FName& Key, const float UpdateRate, const float MaxActiveTime)
	{
		if (UpdateTasks.Contains(Key)) return;
		TryStartTicker();
		UpdateTasks.Add(Key, FUpdateAbilityTickerData(UpdateRate, MaxActiveTime));
	}

	FORCEINLINE void ReSetAbilityUpdate(const FName& Key, const float UpdateRate, const float MaxActiveTime)
	{
		if (UpdateTasks.Contains(Key)) EndUpdateAbility(Key);
		TryStartTicker();
		UpdateTasks.Add(Key, FUpdateAbilityTickerData(UpdateRate, MaxActiveTime));
	}
	
	FORCEINLINE void EndUpdateAbility(const FName& Key)
	{
		if (!UpdateTasks.Contains(Key)) return;
		UpdateTasks.Remove(Key);
		TryEndTickerSave();	
	}	
	
	FORCEINLINE const FUpdateAbilityTickerData* GetUpdateTask(const FName& Key) const
	{
		if (!UpdateTasks.Contains(Key)) return nullptr;
		return &UpdateTasks[Key];
	}
};