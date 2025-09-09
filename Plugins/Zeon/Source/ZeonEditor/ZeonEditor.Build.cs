using UnrealBuildTool;

public class ZeonEditor : ModuleRules
{
	public ZeonEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core", 
			"CoreUObject",
			"Engine",
			"Zeon"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"Slate",
			"SlateCore",
			"ToolMenus",
			"UnrealEd",
			"LevelEditor",
		});
	}
}