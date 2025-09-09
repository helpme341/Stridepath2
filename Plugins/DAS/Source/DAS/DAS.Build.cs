
using UnrealBuildTool;

public class DAS : ModuleRules
{
	public DAS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", 
				"GameplayTags",
				"EnhancedInput",
				"TickerSystem",
				"Zeon",
			});
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
			});

		var WithTouch = ModuleExistsSafe("TouchCore");
		var WithRoto  = ModuleExistsSafe("RotoCore");
	
		if (WithTouch) PublicDependencyModuleNames.AddRange(new string[]
		{
			"TouchCore", 
			"TouchRuntime"
		});
		if (WithRoto) PublicDependencyModuleNames.AddRange(new string[]
		{
			"RotoCore", 
			"RotoRuntime"
		});

		PublicDefinitions.Add(WithTouch ? "WITH_TOUCH=1" : "WITH_TOUCH=0");
		PublicDefinitions.Add(WithRoto  ? "WITH_ROTO=1"  : "WITH_ROTO=0");
	}
	
	private bool ModuleExistsSafe(string ModuleName)
	{
		try
		{
			return !string.IsNullOrEmpty(GetModuleDirectory(ModuleName));
		}
		catch
		{
			return false;
		}
	}
}
