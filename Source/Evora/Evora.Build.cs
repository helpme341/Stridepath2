using UnrealBuildTool;

public class Evora : ModuleRules
{
	public Evora(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"Engine",
			"CoreUObject",
			"GameplayTags",
			"TickerSystem",
			"MAS",
			"DAS"
		});
	}
}
