#pragma once

#include "GPUTextureManager.h"

#include "BlendPCROrigin.h"

#include "SceneViewExtension.h"
#include "RenderResource.h"

#include "InputDevice/InputDeviceManager.h"

DECLARE_GPU_STAT_NAMED_EXTERN(BlendPCR, TEXT("BlendPCR"))
DECLARE_STATS_GROUP(TEXT("BlendPCR"), STATGROUP_BlendPCR, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("BlendPCR_CPU"), STAT_BlendPCR_PrePost, STATGROUP_BlendPCR);

class FBlendPCRViewExtension : public FSceneViewExtensionBase
{
public:
    FBlendPCRViewExtension(const FAutoRegister& AutoRegister);
	~FBlendPCRViewExtension();

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs) override;
	virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled);

	void RegisterBlendPCROrigin(ABlendPCROrigin* Actor);
	void UnregisterBlendPCROrigin(ABlendPCROrigin* Actor);

private:
	void RenderDebugCameraPasses(FRDGBuilder& GraphBuilder, const FSceneView& View, const FScreenPassTexture& SceneColor);
	void AddCompositeWithDepthPass(FRDGBuilder& GraphBuilder, const FSceneView& View, FRDGTextureRef SourceColor, FRDGTextureRef SourceDepth, FRDGTextureRef DestColor, FRDGTextureRef DestDepth, FIntRect ViewRect);

	ABlendPCROrigin* GetPrimaryOrigin() const;

	TArray<TWeakObjectPtr<ABlendPCROrigin>> RegisteredActors;

#if WITH_EDITOR
	FDelegateHandle DebugDrawHandle;
	void DrawDebugOverlay(UCanvas* Canvas, APlayerController* Controller);
#endif
};