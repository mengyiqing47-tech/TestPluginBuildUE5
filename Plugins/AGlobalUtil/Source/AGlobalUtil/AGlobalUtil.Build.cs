// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AGlobalUtil : ModuleRules
{
	public AGlobalUtil(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
       // bUsePrecompiled = true;
        PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
				"AGlobalUtil/Public",
				"AGlobalUtil/Public/ProcessLock",
				"AGlobalUtil/Public/ShareMemory"
            }
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
				"AGlobalUtil/Private",
				"AGlobalUtil/Private/ProcessLock",
				"AGlobalUtil/Private/ShareMemory"
            }
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
                "ImageWrapper",
                "Core",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
        if (Target.bBuildEditor == true)
        {
            PublicDependencyModuleNames.AddRange(
                new string[] {
                    "UnrealEd",
                }
                );
        }
    }
}
