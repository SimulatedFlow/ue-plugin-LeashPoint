// Copyright 2026 Silvan Teufel. All Rights Reserved.

using UnrealBuildTool;

public class LeashPoint : ModuleRules
{
	public LeashPoint(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",

			// AActor, UActorComponent and DrawDebugHelpers for the demo level.
			"Engine",

			// ULeashPointSettings is a UDeveloperSettings, so the radii and the timers sit under
			// Project Settings > Plugins > LeashPoint without an editor module.
			"DeveloperSettings",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
		});

		// Deliberately NOT here:
		//   AIModule / NavigationSystem - LeashPoint decides WHEN a chase is over and where home is.
		//                                 Walking back there is your behaviour tree's job, and tying
		//                                 the plugin to one navigation setup would shut out every
		//                                 project that moves its enemies another way.
	}
}
