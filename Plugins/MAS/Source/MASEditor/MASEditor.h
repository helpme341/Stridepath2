
#pragma once

#include "MovementAbility/MovementAbility.h"
#include "MovementAbilitySystem/MovementAbilitySystem.h"
#include "DASEditor/AbilityInfoWindowModule.h"

class FMASEditorModule : public FAbilityInfoWindowModule<UMovementAbilitySystem, UMovementAbility>
{
protected:

	virtual void UpdateAbilityInfo_Internal(const FName& Key, const UMovementAbility* Ability,
	TSharedPtr<SVerticalBox>& LeftAbilityBox, TSharedPtr<SVerticalBox>& RightAbilityBox) const override
	{
		/*
		LeftAbilityBox->AddSlot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("Movement Allows:"))))
			];

		if (const auto AllowsSettings = AbilitySystem->AbilitiesMovementSettingsAllows.Find(Ability->GetClass()))
		{
			for (const auto Allow : AllowsSettings->Types)
			{
				if (const auto Priority = Ability->MovementAbilitySettings.PrioritySettings.Find(Allow))
				{
					auto Text = FString::Printf(TEXT("%s, %d"),
						*StaticEnum<EMovementSettingsType>()->GetDisplayNameTextByValue(Allow).ToString(), *Priority);
					
					LeftAbilityBox->AddSlot()
					[
						SNew(STextBlock)
						.Text(FText::FromString(Text))
					];
				}
			}
		}
		*/
	}

public:
	FMASEditorModule()
	{
		// Window set up:
		RegisterWindowId = "MASDebugWindow";
		WindowIcon = "Node.Debug";
		WindowDisplayName = NSLOCTEXT("MASDebugWindow", "MASDebugTabTitle", "MASDebug");
		WindowMenuType = ETabSpawnerMenuType::Hidden;
		
		// Button set up:
		ButtonMenu = "LevelEditor.MainMenu.Tools.Debug";
		ButtonSection = "CustomDebugSection";
		ButtonName = "MASDebugButton";
		ButtonLabel = FText::FromString("MAS Debug Tool");
		ButtonToolTip = FText::FromString("Open the MAS Debug Tool (Added By MAS Plugin)");
		ButtonIcon = "Node.Debug";
	}
};
