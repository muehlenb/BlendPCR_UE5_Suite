// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class BlendPCR : ModuleRules
{
    public BlendPCR(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        bUseRTTI = true;
        bEnableExceptions = true;

        PublicDefinitions.Add("_CRT_SECURE_NO_WARNINGS");

        PublicIncludePaths.AddRange(
            new string[] {
				// ... add public include paths required here ...
				EngineDirectory + "/Source/Runtime/Renderer/Private"
            }
        );

        PrivateIncludePaths.AddRange(
            new string[] {
				// ... add other private include paths required here ...
			}
        );

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "ApplicationCore",
                "Core",
                "CoreUObject",
                "Engine",
                "Renderer",
                "RenderCore",
                "RHI",
                "Projects",
                "Json",           
                "JsonUtilities",
				"OpenCVHelper",
                "OpenCV",
                "Sockets",
                "Networking"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Niagara",
                "Slate",
                "SlateCore"
			}
        );

        DynamicallyLoadedModuleNames.AddRange(new string[]{});

        LoadAzureKinectDependencies();
        LoadOrbbecDependencies();
    }

    void LoadAzureKinectDependencies()
    {
        string PluginPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../"));

        // Include Azure Kinect Files:
        PublicIncludePaths.Add(Path.Combine(PluginPath, "ThirdParty", "k4a_142", "include"));
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            string WinReleaseFolder = Path.Combine(PluginPath, "ThirdParty", "k4a_142", "windows-desktop", "amd64", "release");

            string[] k4aLibs = { "k4a.lib", "k4arecord.lib" };

            foreach (string lib in k4aLibs)
            {
                PublicAdditionalLibraries.Add(Path.Combine(WinReleaseFolder, "lib", lib));
            }

            string[] k4aDlls = { "k4a.dll", "k4arecord.dll", "depthengine_2_0.dll" };

            foreach (string dll in k4aDlls)
            {
                PublicDelayLoadDLLs.Add(dll);

                string SourceDll = Path.Combine(WinReleaseFolder, "bin", dll);

                RuntimeDependencies.Add(Path.Combine("$(ProjectDir)", "Binaries", "Win64", dll), SourceDll, StagedFileType.NonUFS);
                RuntimeDependencies.Add(Path.Combine("$(BinaryOutputDir)", dll), SourceDll, StagedFileType.NonUFS);
                RuntimeDependencies.Add(Path.Combine("$(TargetOutputDir)", dll), SourceDll, StagedFileType.NonUFS);
            }
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            string LinuxReleaseFolder = Path.Combine(PluginPath, "ThirdParty", "k4a_142", "linux_gnu", "amd64", "release");
            string LinuxLibPath = Path.Combine(LinuxReleaseFolder, "lib");

            PublicAdditionalLibraries.Add(Path.Combine(LinuxLibPath, "libk4a.so"));
            PublicAdditionalLibraries.Add(Path.Combine(LinuxLibPath, "libk4arecord.so"));
            PublicAdditionalLibraries.Add(Path.Combine(LinuxLibPath, "libdepthengine.so"));

            RuntimeDependencies.Add("$(BinaryOutputDir)/libk4a.so", Path.Combine(LinuxLibPath, "libk4a.so"), StagedFileType.NonUFS);
            RuntimeDependencies.Add("$(BinaryOutputDir)/libk4arecord.so", Path.Combine(LinuxLibPath, "libk4arecord.so"), StagedFileType.NonUFS);
            RuntimeDependencies.Add("$(BinaryOutputDir)/libdepthengine.so", Path.Combine(LinuxLibPath, "libdepthengine.so"), StagedFileType.NonUFS);

            RuntimeDependencies.Add("$(TargetOutputDir)/libk4a.so", Path.Combine(LinuxLibPath, "libk4a.so"), StagedFileType.NonUFS);
            RuntimeDependencies.Add("$(TargetOutputDir)/libk4arecord.so", Path.Combine(LinuxLibPath, "libk4arecord.so"), StagedFileType.NonUFS);
            RuntimeDependencies.Add("$(TargetOutputDir)/libdepthengine.so", Path.Combine(LinuxLibPath, "libdepthengine.so"), StagedFileType.NonUFS);
        }
    }

    void LoadOrbbecDependencies()
    {
        string PluginPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../"));

        // Include Orbbec Femto Files:
        PublicIncludePaths.Add(Path.Combine(PluginPath, "ThirdParty", "orbbec_sdk_276", "include"));
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            string WinReleaseFolder = Path.Combine(PluginPath, "ThirdParty", "orbbec_sdk_276");

            PublicAdditionalLibraries.Add(Path.Combine(WinReleaseFolder, "lib", "OrbbecSDK.lib"));
            //PublicAdditionalLibraries.Add(Path.Combine(WinReleaseFolder, "lib", "extensions", "depthengine", "depthengine.lib"));

            string[] allDLLs = {
                "concrt140.dll",
                "msvcp140.dll",
                "msvcp140_1.dll",
                "msvcp140_2.dll",
                "OrbbecSDK.dll",
                "vcruntime140.dll",
                Target.Configuration == UnrealTargetConfiguration.Debug ? "opencv_world340d.dll" : "opencv_world340.dll",
                Path.Combine("extensions", "depthengine", "depthengine.dll"),
                Path.Combine("extensions", "filters", "FilterProcessor.dll"),
                Path.Combine("extensions", "filters", "ob_priv_filter.dll"),
                Path.Combine("extensions", "firmwareupdater", "firmwareupdater.dll"),
                Path.Combine("extensions", "frameprocessor", "ob_frame_processor.dll")
            };

            foreach (string dll in allDLLs)
            {
                bool bIsVCRuntime = dll.StartsWith("msvcp140") || dll.StartsWith("vcruntime140") || dll == "concrt140.dll";

                if (!bIsVCRuntime)
                {
                    PublicDelayLoadDLLs.Add(Path.GetFileName(dll));
                }

                string SourceDll = Path.Combine(WinReleaseFolder, "bin", dll);

                RuntimeDependencies.Add(Path.Combine("$(ProjectDir)", "Binaries", "Win64", dll), SourceDll, StagedFileType.NonUFS);
                RuntimeDependencies.Add(Path.Combine("$(BinaryOutputDir)", dll), SourceDll, StagedFileType.NonUFS);
                RuntimeDependencies.Add(Path.Combine("$(TargetOutputDir)", dll), SourceDll, StagedFileType.NonUFS);
            }
        }
    }
}
