// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BlendPCVR : ModuleRules
{
	public BlendPCVR(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
