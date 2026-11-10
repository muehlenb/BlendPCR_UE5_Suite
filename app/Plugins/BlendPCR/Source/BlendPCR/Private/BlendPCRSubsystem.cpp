#include "BlendPCRSubsystem.h"

#include "SceneViewExtension.h"

#include "BlendPCRViewExtension.h"

void UBlendPCRSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogTemp, Display, TEXT("[BlendPCR] Initialize Rendering Subsystem..."));

    BlendPCRViewExtension = FSceneViewExtensions::NewExtension<FBlendPCRViewExtension>();
}

void UBlendPCRSubsystem::Deinitialize()
{
    UE_LOG(LogTemp, Display, TEXT("[BlendPCR] Destruct Rendering Subsystem..."));

    // Prevent this SVE from being gathered, in case it is kept alive by a strong reference somewhere else.
    {
        BlendPCRViewExtension->IsActiveThisFrameFunctions.Empty();

        FSceneViewExtensionIsActiveFunctor IsActiveFunctor;

        IsActiveFunctor.IsActiveFunction = [](const ISceneViewExtension* SceneViewExtension, const FSceneViewExtensionContext& Context)
            {
                return TOptional<bool>(false);
            };

        BlendPCRViewExtension->IsActiveThisFrameFunctions.Add(IsActiveFunctor);
    }

    ENQUEUE_RENDER_COMMAND(ReleaseSVE)([this](FRHICommandListImmediate& RHICmdList)
        {
            // Prevent this SVE from being gathered, in case it is kept alive by a strong reference somewhere else.
            {
                BlendPCRViewExtension->IsActiveThisFrameFunctions.Empty();

                FSceneViewExtensionIsActiveFunctor IsActiveFunctor;

                IsActiveFunctor.IsActiveFunction = [](const ISceneViewExtension* SceneViewExtension, const FSceneViewExtensionContext& Context)
                    {
                        return TOptional<bool>(false);
                    };

                BlendPCRViewExtension->IsActiveThisFrameFunctions.Add(IsActiveFunctor);
            }

            BlendPCRViewExtension.Reset();
            BlendPCRViewExtension = nullptr;
        });

    // Finish all rendering commands before cleaning up actors.
    FlushRenderingCommands();

    UE_LOG(LogTemp, Display, TEXT("[BlendPCR] I'm destructed now :-("));
}


void UBlendPCRSubsystem::RegisterBlendPCROrigin(ABlendPCROrigin* Actor)
{
    BlendPCRViewExtension->RegisterBlendPCROrigin(Actor);
}

void UBlendPCRSubsystem::UnregisterBlendPCROrigin(ABlendPCROrigin* Actor)
{
    BlendPCRViewExtension->UnregisterBlendPCROrigin(Actor);
}