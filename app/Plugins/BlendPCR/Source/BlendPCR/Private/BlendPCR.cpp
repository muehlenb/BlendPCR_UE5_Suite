// Copyright Epic Games, Inc. All Rights Reserved.
#include "BlendPCR.h"

#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FBlendPCRModule"

void FBlendPCRModule::StartupModule()
{
    FString baseDir = IPluginManager::Get().FindPlugin(TEXT("BlendPCR"))->GetBaseDir();
    FString pluginShaderBaseDir = FPaths::Combine(baseDir, TEXT("Shaders"));
    AddShaderSourceDirectoryMapping(TEXT("/CustomShaders"), pluginShaderBaseDir);
}

void FBlendPCRModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBlendPCRModule, BlendPCR)