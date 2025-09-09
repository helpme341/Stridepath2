
#include "WindowModuleBase.h"
#include "Styling/SlateIconFinder.h"

void FWindowModuleBase::StartupModule()
{
	PostPIEStartedDelegateHandle = FEditorDelegates::PostPIEStarted.AddRaw(this, &FWindowModuleBase::OnPostPIEStarted);
	EndPIEDelegateHandle = FEditorDelegates::EndPIE.AddRaw(this, &FWindowModuleBase::OnEndPIE);
	if (bSetUpButton) UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FWindowModuleBase::RegisterButton));
	if (bUpdateWindow) TickDelegate = FTickerDelegate::CreateRaw(this, &FWindowModuleBase::UpdateWindowInformation);
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(RegisterWindowId, FOnSpawnTab::CreateRaw(this, &FWindowModuleBase::RegisterWindow))
		.SetDisplayName(WindowDisplayName)
		.SetMenuType(WindowMenuType);
}

void FWindowModuleBase::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(RegisterWindowId);
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	
	EndTickerUpdate();
	FEditorDelegates::PostPIEStarted.Remove(PostPIEStartedDelegateHandle);
	FEditorDelegates::EndPIE.Remove(EndPIEDelegateHandle);
}

void FWindowModuleBase::OnPostPIEStarted(bool bArg)
{
	bIsGameStarted = true;
	if (bAutoManageTicker) StartTickerUpdate();
}

void FWindowModuleBase::OnEndPIE(bool bArg)
{
	bIsGameStarted = false;
	if (bAutoManageTicker) EndTickerUpdate();
}	

void FWindowModuleBase::RegisterButton()
{
	if (UToolMenu* DebugMenu = UToolMenus::Get()->ExtendMenu(ButtonMenu))
	{
		FToolMenuSection& Section = DebugMenu->FindOrAddSection(ButtonSection);

		Section.AddMenuEntry(ButtonName, ButtonLabel, ButtonToolTip, FSlateIconFinder::FindIcon(ButtonIcon),
			FUIAction(FExecuteAction::CreateLambda([this]
			{
				if (bAutoManageTicker && bIsGameStarted) StartTickerUpdate();
				FGlobalTabmanager::Get()->TryInvokeTab(RegisterWindowId);
			}))
		);
	}
}

TSharedRef<SDockTab> FWindowModuleBase::RegisterWindow(const FSpawnTabArgs& SpawnTabArgs)
{
	TSharedRef<SDockTab> Window = SNew(SDockTab).TabRole(NomadTab)
		.OnTabClosed_Lambda([this](TSharedRef<SDockTab> /* ActivatedTab */)
		{
			if (bAutoManageTicker) EndTickerUpdate();
		});
		
	const FSlateIcon Icon = FSlateIconFinder::FindIcon(WindowIcon);
	Window->SetTabIcon(Icon.GetIcon());
	return Window;
}
