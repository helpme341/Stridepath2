using UnrealBuildTool;

public class MASEditor : ModuleRules
{
	public MASEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"DASEditor", 
				"Engine",
				"Slate",
				"SlateCore",
				
				"GameplayTags",
				"TickerSystem",
				"ZeonEditor",
				"Zeon",
				"MAS",
				"DAS",
			}
		);
	}
}