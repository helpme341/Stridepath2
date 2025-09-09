// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TickerSystem : ModuleRules
{
	public TickerSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Zeon",
			}
		);
	}
}