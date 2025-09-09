using UnrealBuildTool;

public class DASEditor : ModuleRules
{
	public DASEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
		});
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"ToolMenus",
				"Slate",
				"SlateCore",
				"GameplayTags",
				"AppFramework",
				"UnrealEd",
				"Zeon",
				"TickerSystem",
				"ZeonEditor",
				"DAS",
			}
		);
	}
}