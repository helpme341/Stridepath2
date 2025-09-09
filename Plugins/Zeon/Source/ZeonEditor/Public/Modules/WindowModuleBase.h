
#pragma once

class ZEONEDITOR_API FWindowModuleBase : public IModuleInterface
{
	FTickerDelegate TickDelegate;
	FTSTicker::FDelegateHandle TickHandle;
	FDelegateHandle PostPIEStartedDelegateHandle;
	FDelegateHandle EndPIEDelegateHandle;
	
	bool bIsGameStarted = false;
protected:
	FORCEINLINE bool IsGameStarted() const { return bIsGameStarted; }

	FORCEINLINE virtual void StartTickerUpdate()
	{
		if (bUpdateWindow && !TickHandle.IsValid()) FTSTicker::GetCoreTicker().AddTicker(TickDelegate, UpdateWindowRate);
	}
	FORCEINLINE virtual void EndTickerUpdate()
	{
		if (bUpdateWindow && TickHandle.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	}

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	virtual void OnPostPIEStarted(bool bArg);
	virtual void OnEndPIE(bool bArg);

	virtual void RegisterButton();
	virtual TSharedRef<SDockTab> RegisterWindow(const FSpawnTabArgs& SpawnTabArgs);

	virtual bool UpdateWindowInformation(float DeltaTime) { return true; }

	/**
	 * Автоматически включает тикер только тогда когда открывается окно во время запушенного PIE.
	 * А также выключает при выходе из PIE или закрытии она
	*/
	bool bAutoManageTicker = true;
	
	// Window Settings:
	bool bUpdateWindow = true;
	float UpdateWindowRate = 0.f;
	
	FName RegisterWindowId;
	FText WindowDisplayName;
	FName WindowIcon;
	ETabSpawnerMenuType::Type WindowMenuType = ETabSpawnerMenuType::Hidden;

	// Button settings:
	bool bSetUpButton = true;
	FName ButtonMenu;
	FName ButtonSection;
	
	FName ButtonName;
	FText ButtonLabel;
	FText ButtonToolTip;
	FName ButtonIcon;
};
