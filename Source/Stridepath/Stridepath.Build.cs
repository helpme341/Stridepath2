using UnrealBuildTool;

public class Stridepath : ModuleRules
{
	public Stridepath(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"GameplayTags",
			"Engine",
			"TickerSystem",
			"Evora",
			"MAS",
			"DAS",
		});
		
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"EnhancedInput",
			"Zeon",
		});
	}
}
	