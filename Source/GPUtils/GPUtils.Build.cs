// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GPUtils : ModuleRules
{
	public GPUtils(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Latest;
		PrivatePCHHeaderFile = "Private/GPUtilsPCH.h";

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "OnlineSubsystem", });

		PrivateDependencyModuleNames.AddRange(new string[] { "CoreUObject", "Engine", "RenderCore", });
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		DynamicallyLoadedModuleNames.AddRange(new string[] {  });
	}
}
