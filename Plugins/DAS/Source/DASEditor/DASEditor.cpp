
#include "AbilityInfoWindowModule.h"
#include "AbilitySystem/DynamicAbilitySystem.h"

class FDASEditorModule final : public FAbilityInfoWindowModule<UDynamicAbilitySystem, UDynamicAbility>
{
public:
	FDASEditorModule()
	{
		// Window set up:
		RegisterWindowId = "DASDebugWindow";
		WindowIcon = "Node.Debug";
		WindowDisplayName = NSLOCTEXT("DASDebugWindow", "DASDebugTabTitle", "DASDebug");
		WindowMenuType = ETabSpawnerMenuType::Hidden;
		// Button set up:
		ButtonMenu = "LevelEditor.MainMenu.Tools.Debug";
		ButtonSection = "CustomDebugSection";

		ButtonName = "DASDebugButton";
		ButtonLabel = FText::FromString("DAS Debug Tool");
		ButtonToolTip = FText::FromString("Open the DAS Debug Tool (Added By DAS Plugin)");
		ButtonIcon = "Node.Debug";
	}
};

IMPLEMENT_MODULE(FDASEditorModule, DASEditor);