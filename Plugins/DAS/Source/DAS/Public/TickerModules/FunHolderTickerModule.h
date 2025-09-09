
#pragma once

#include "CoreMinimal.h"
#include "TickerModule.h"
#include "Utility/Invoker.h"

struct FDelayedTickerFunTask
{
	float RemainingTime;
	TInvoker<void()> Task;
};

class UDynamicAbility;

class DAS_API FFunHolderTickerModule : public FTickerModule
{
	GENERATED_TICKER_BODY("FunHolderTickerModule")
	virtual void Tick(float DeltaTime) override
	{
		TArray<FName> CompletedFunctions;
		for (auto& FunData : DelayedFunctions)
		{
			FunData.Value.RemainingTime -= DeltaTime;
			if (FunData.Value.RemainingTime <= 0.f)
			{
				FunData.Value.Task();
				CompletedFunctions.Add(FunData.Key);
			}
		}
		for (auto Key : CompletedFunctions) DelayedFunctions.Remove(Key);
	}
	virtual bool NeedUpdate() const override
	{
		return !DelayedFunctions.IsEmpty();
	}

	TMap<FName, FDelayedTickerFunTask> DelayedFunctions;
public:
	FFunHolderTickerModule() = default;
	
	FFunHolderTickerModule(const FFunHolderTickerModule&) = delete;
	FFunHolderTickerModule& operator=(const FFunHolderTickerModule&) = delete;
	
	FFunHolderTickerModule(FFunHolderTickerModule&&) noexcept = default;
	FFunHolderTickerModule& operator=(FFunHolderTickerModule&&) noexcept = default;
	
	virtual ~FFunHolderTickerModule() override
	{
		DelayedFunctions.Empty();
	}
	
	TInvoker<void()>* AddDelayedFun(const FName& Key, const float DelaySeconds)
	{	
		if (DelayedFunctions.Contains(Key)) return nullptr;
		TryStartTicker();
		return &DelayedFunctions.Add(Key, FDelayedTickerFunTask(DelaySeconds)).Task;
	}
	FORCEINLINE void RemoveDelayedFun(const FName& Key)
	{
		if (!DelayedFunctions.Contains(Key)) return;
		DelayedFunctions.Remove(Key);
		TryEndTickerSave();
	}
};