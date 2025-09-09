
#pragma once

#include "CoreMinimal.h"

#define GENERATED_TICKER_BODY(Name) \
	public: \
		static FName GetModuleName() { return Name; } \
	private:

/** Обработчик задачи, подключённый к FStaticTickerManager для выполнения во времени */
class TICKERSYSTEM_API FTickerModule
{
public:
	virtual ~FTickerModule() = default;

private:
	friend class FStaticTickerManager;

	bool bIsGamePaused = false;
	
	/** Владелец - менеджер модуля */
	FStaticTickerManager* OwnerManager;
protected:
	FORCEINLINE bool GetIsGamePaused() const { return bIsGamePaused; }

	/** Функция, которая вызывается каждый тик (или настроенное во владельце время),
	 * также важно отметить что тик в менеджере может быть выключен и вызов этой функции подкрутится. */
	virtual void Tick(float DeltaTime) {}

	/** Вызывается в начале игры */
	virtual void OnGameStarted() {}
	/** Вызывается в конце игры */
	virtual void OnGameEnded() {}
	/** Вызывается при начале паузы во время игры */
	virtual void OnGamePaused() {}
	/** Вызывается при окончании паузы во время игры */
	virtual void OnGameUnPaused() {}
	
	/** Вызывается из менеджера для проверки нужен ли модулю tick, если нужен то возвращаем true, иначе false */
	virtual bool NeedUpdate() const { return false; }

	/** Функция для попытки начать работу tich в менеджера */	
	void TryStartTicker() const;
	/** Функция для попытки закончить работу tich в менеджере */	
	void TryEndTicker() const;

	/** Функция для попытки закончить работу tich в менеджере, с проверкой нужен ли тикер этому модулю */	
	void TryEndTickerSave() const;

	/** Имя модуля нужное для его регистрации в системе */
	FName ModuleName = NAME_None;

	/** Останавливать ли обновление модуля во время паузы */
	bool bTickInPauseDisabled = true;
};