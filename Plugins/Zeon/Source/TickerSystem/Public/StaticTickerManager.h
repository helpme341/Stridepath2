
#pragma once

#include "CoreMinimal.h"
#include "TickerModule.h"
#include "Containers/Ticker.h"
#include "Utility/Invoker.h"

/** Enum для выбора ивента для активации или де активации тикера */
UENUM(BlueprintType)
enum class ETickerStateType : uint8
{
	Init,
	BeginPlay,
	GamePaused,

	EndPlay,
	GameUnPaused,
};

DECLARE_LOG_CATEGORY_EXTERN(LogStaticTicker, Log, All);

/** Класс, обеспечивающая централизованное управление логикой, работающей во времени через систему модулей. */
class TICKERSYSTEM_API FStaticTickerManager
{
	friend FTickerModule;
	
	bool Tick(float DeltaTime);
	bool CleanupManager(float DeltaTime);
	
	bool DoesRequireTicker(const FTickerModule* IgnoreModule) const;
	
	void TryStartTicker();
	void TryEndTicker(const FTickerModule* Module) const;
	bool EndTicker() const;

	bool bLastPauseState = false;
	float CurrentCleanupTime = 0.f;
	float CurrentPauseUpdateTime = 0.f;
	FTSTicker::FDelegateHandle TickHandle;
	FDelegateHandle GameEndedDelegateHandle;
	FDelegateHandle GameStartedDelegateHandle;
	FDelegateHandle GamePauseDelegateHandle;
	TMap<FName, TUniquePtr<FTickerModule>> TickerModules;
protected:
	FStaticTickerManager();
	virtual ~FStaticTickerManager();

	FStaticTickerManager(const FStaticTickerManager&) = delete;
	FStaticTickerManager& operator=(const FStaticTickerManager&) = delete;

	virtual void OnGameStarted(EWorldType::Type WorldType);
	virtual void OnGameEnded(UWorld* World);
	virtual void OnGamePaused(bool bPaused);
	
	/** Использовать ли систему проверки занятости тикера */
	bool bUseCleanupSystem = true;

	/** Частота проверки занятость тикера */
	float CleanupRate = 30.f;

	/** Частота обновления главного тикера */
	float GlobalTickerUpdateRate = 0.001;

	/** Список триггеров, при активации одного из них, система попытается активировать тикер */
	TSet<ETickerStateType> AutoActivateTickerType;

	/** Список триггеров, при активации одного из них, система попытается отключить тикер */
	TSet<ETickerStateType> AutoDisableTickerType;

	// ---------------- Fun ----------------

	/** Функция для добавления модуля в систему */
	bool RegisterTickerModule(FTickerModule* Module);

	/** Функция для получения зарегистрированного в системе модуля */
	FTickerModule* GetTickerModuleMutable(const FName ModuleName);

	/** Функция для создания и регистрации модуля в системе через шаблон, важно что добавление должно происходить в конструкторе */
	template<typename T>
	T* AddTickerModule();

	/** Функция для получения зарегистрированного в системе модуля через шаблон  */
	template<typename T>
	T* GetTickerModuleMutable();

	/** Функция для получения константного указателя на зарегистрированного модуля через шаблон */
	template<typename T>
	const T* GetTickerModule() const;

	/** Вызывает состояние активации или де активации тикера */
	FORCEINLINE void TryAutoModifyTickerState(const ETickerStateType& TickerState)
	{
		if (AutoActivateTickerType.Contains(TickerState)) TryStartTicker();
		if (AutoDisableTickerType.Contains(TickerState)) EndTicker();
	}
private:

	/** Обёртка для логов LogStaticTicker из шаблонов для избежания LNK ошибок. */
	FORCEINLINE static void LogTickerWarning(const FString& Msg)
	{
		UE_LOG(LogStaticTicker, Warning, TEXT("%s"), *Msg);
	}

	/** Обёртка для логов LogStaticTicker из шаблонов для избежания LNK ошибок. */
	FORCEINLINE static void LogTickerError(const FString& Msg)
	{
		UE_LOG(LogStaticTicker, Error, TEXT("%s"), *Msg);
	}
};

template <typename T>
T* FStaticTickerManager::AddTickerModule()
{
	const auto& ModuleName = T::GetModuleName();
	if (TickerModules.Contains(ModuleName))
	{
		LogTickerWarning(FString::Printf(TEXT("Cannot add module '%s' because it is already added"), *ModuleName.ToString()));
		return nullptr;	
	}
	if (T* NewModule = new T())
	{
		NewModule->OwnerManager = this;
		TickerModules.Add(ModuleName, TUniquePtr<FTickerModule>(std::move(NewModule)));
		return NewModule;
	}
	LogTickerError(FString::Printf(TEXT("Cannot create module: %s"), *ModuleName.ToString()));
	return nullptr;
}

template <typename T>
T* FStaticTickerManager::GetTickerModuleMutable()
{
	const FName ModuleName = T::GetModuleName();
	const auto* Found = TickerModules.Find(ModuleName);
	if (!Found) return nullptr;
	return static_cast<T*>(Found->Get());
}

template <typename T>
const T* FStaticTickerManager::GetTickerModule() const
{
	const FName ModuleName = T::GetModuleName();
	const auto* Found = TickerModules.Find(ModuleName);
	if (!Found) return nullptr;
	return static_cast<T*>(Found->Get());
}
