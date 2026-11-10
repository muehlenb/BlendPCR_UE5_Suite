#pragma once

#include <memory>
#include <mutex>
#include <map>
#include <string>

#include "RHI.h"
#include "RHIResources.h"
#include "RenderResource.h"
#include "RenderUtils.h"
#include "RenderingThread.h"

#include "CoreMinimal.h"
#include "BlendPCROrigin.h"

#include "Niagara/NiagaraDataInterfaceRHITexture.h"
#include "Engine/TextureRenderTarget2D.h"

#include "RGBDImage.h"

class GPUTextureSlot 
{
    std::shared_ptr<RGBDImage> currentPointCloud = nullptr;
    bool reUploadPointCloud = false;
    mutable std::mutex pcMutex;

    bool isInitialized = false;

    bool lookupUploaded = false;

    bool niagaraTexturesInitRequested = false;
    bool niagaraTexturesInitialized = false;

public:
    const int Width;
    const int Height;

    FMatrix44f ModelMatrix;

    /* Input Color Texture */
    FTexture2DRHIRef ColorRawTexture;
    TRefCountPtr<IPooledRenderTarget> ColorRawPooledTarget;
    UTextureRenderTarget2D* ColorNiagaraRenderTarget;

    /* Input Depth Texture */
    FTexture2DRHIRef DepthRawTexture;
    TRefCountPtr<IPooledRenderTarget> DepthRawPooledTarget;

    /* Input Color Texture */
    FTexture2DRHIRef ColorTexture;
    TRefCountPtr<IPooledRenderTarget> ColorPooledTarget;

    /* Input Depth Texture */
    FTexture2DRHIRef DepthTexture;
    TRefCountPtr<IPooledRenderTarget> DepthPooledTarget;

    /* Input Lookup Texture */
    FTexture2DRHIRef LookupTexture;
    TRefCountPtr<IPooledRenderTarget> LookupPooledTarget;

    /* Generated Point Cloud Texture */
    FTexture2DRHIRef PointCloudTexture;
    TRefCountPtr<IPooledRenderTarget> PointCloudPooledTarget;
    UTextureRenderTarget2D* PointCloudNiagaraRenderTarget;

    /* Generated Rejection Texture */
    FTexture2DRHIRef RejectionTexture;
    TRefCountPtr<IPooledRenderTarget> RejectionPooledTarget;

    /* Generated Edge Distance Texture */
    FTexture2DRHIRef EdgeDistanceTexture;
    TRefCountPtr<IPooledRenderTarget> EdgeDistancePooledTarget;

    /* Generated Moving Least Squares Texture */
    FTexture2DRHIRef MLSTexture;
    TRefCountPtr<IPooledRenderTarget> MLSPooledTarget;

    /* Generated Normal Texture */
    FTexture2DRHIRef NormalTexture;
    TRefCountPtr<IPooledRenderTarget> NormalPooledTarget;

    /* Generated Quality Estimate Texture */
    FTexture2DRHIRef QualityTexture;
    TRefCountPtr<IPooledRenderTarget> QualityPooledTarget;

    GPUTextureSlot(int32 pWidth, int32 pHeight)
        : Width(pWidth)
        , Height(pHeight)
    {
        ENQUEUE_RENDER_COMMAND(CreateTextures)(
            [this](FRHICommandListImmediate& RHICmdList)
            {
                // Color Texture (INPUT):
                ColorRawTexture = RHICreateTexture(
                    FRHITextureCreateDesc::Create2D(TEXT("ColorRawTex"))
                    .SetExtent(Width, Height)
                    .SetFormat(PF_B8G8R8A8)
                    .SetNumMips(1)
                    .SetNumSamples(1)
                    .SetFlags(ETextureCreateFlags::ShaderResource | ETextureCreateFlags::RenderTargetable)
                );
                ColorRawPooledTarget = CreateRenderTarget(ColorRawTexture, TEXT("BlendPCR.CameraColorRaw"));

                // Depth Texture (INPUT):
                DepthRawTexture = RHICreateTexture(
                    FRHITextureCreateDesc::Create2D(TEXT("DepthRawTex"))
                    .SetExtent(Width, Height)
                    .SetFormat(PF_R16_UINT)
                    .SetNumMips(1)
                    .SetNumSamples(1)
                    .SetFlags(ETextureCreateFlags::ShaderResource | ETextureCreateFlags::RenderTargetable)
                );
                DepthRawPooledTarget = CreateRenderTarget(DepthRawTexture, TEXT("BlendPCR.CameraDepthRaw"));

                // Color Texture:
                ColorTexture = RHICreateTexture(
                    FRHITextureCreateDesc::Create2D(TEXT("ColorTex"))
                    .SetExtent(Width, Height)
                    .SetFormat(PF_B8G8R8A8)
                    .SetNumMips(1)
                    .SetNumSamples(1)
                    .SetFlags(ETextureCreateFlags::ShaderResource | ETextureCreateFlags::RenderTargetable)
                );
                ColorPooledTarget = CreateRenderTarget(ColorTexture, TEXT("BlendPCR.CameraColor"));

                // Depth Texture:
                DepthTexture = RHICreateTexture(
                    FRHITextureCreateDesc::Create2D(TEXT("DepthHFTex"))
                    .SetExtent(Width, Height)
                    .SetFormat(PF_R16_UINT)
                    .SetNumMips(1)
                    .SetNumSamples(1)
                    .SetFlags(ETextureCreateFlags::ShaderResource | ETextureCreateFlags::RenderTargetable)
                );
                DepthPooledTarget = CreateRenderTarget(DepthTexture, TEXT("BlendPCR.CameraDepth"));

                // Lookup Texture (INPUT):
                LookupTexture = RHICreateTexture(
                    FRHITextureCreateDesc::Create2D(TEXT("LookupTex"))
                    .SetExtent(Width, Height)
                    .SetFormat(PF_G32R32F)
                    .SetNumMips(1)
                    .SetNumSamples(1)
                    .SetFlags(ETextureCreateFlags::ShaderResource | ETextureCreateFlags::RenderTargetable)
                );
                LookupPooledTarget = CreateRenderTarget(LookupTexture, TEXT("BlendPCR.CameraLookup"));

                // Point Cloud Texture:
                PointCloudTexture = RHICreateTexture(
                    FRHITextureCreateDesc::Create2D(TEXT("PointCloudTex"))
                    .SetExtent(Width, Height)
                    .SetFormat(PF_FloatRGBA)
                    .SetNumMips(1)
                    .SetNumSamples(1)
                    .SetFlags(ETextureCreateFlags::ShaderResource | ETextureCreateFlags::RenderTargetable)
                );
                PointCloudPooledTarget = CreateRenderTarget(PointCloudTexture, TEXT("BlendPCR.CameraPointCloud"));

                // Rejection Texture:
                RejectionTexture = RHICreateTexture(
                    FRHITextureCreateDesc::Create2D(TEXT("RejectionTex"))
                    .SetExtent(Width, Height)
                    .SetFormat(PF_R8)
                    .SetNumMips(1)
                    .SetNumSamples(1)
                    .SetFlags(ETextureCreateFlags::ShaderResource | ETextureCreateFlags::RenderTargetable | ETextureCreateFlags::UAV) 
                );
                RejectionPooledTarget = CreateRenderTarget(RejectionTexture, TEXT("BlendPCR.CameraRejection"));

                // EdgeDistance Texture:
                EdgeDistanceTexture = RHICreateTexture(
                    FRHITextureCreateDesc::Create2D(TEXT("EdgeDistanceTex"))
                    .SetExtent(Width, Height)
                    .SetFormat(PF_R8)
                    .SetNumMips(1)
                    .SetNumSamples(1)
                    .SetFlags(ETextureCreateFlags::ShaderResource | ETextureCreateFlags::RenderTargetable | ETextureCreateFlags::UAV)
                );
                EdgeDistancePooledTarget = CreateRenderTarget(EdgeDistanceTexture, TEXT("BlendPCR.CameraEdgeDistance"));

                // Point Cloud Texture:
                MLSTexture = RHICreateTexture(
                    FRHITextureCreateDesc::Create2D(TEXT("MLSTex"))
                    .SetExtent(Width, Height)
                    .SetFormat(PF_FloatRGBA)
                    .SetNumMips(1)
                    .SetNumSamples(1)
                    .SetFlags(ETextureCreateFlags::ShaderResource | ETextureCreateFlags::RenderTargetable)
                );
                MLSPooledTarget = CreateRenderTarget(MLSTexture, TEXT("BlendPCR.CameraMLS"));

                // Point Cloud Texture:
                NormalTexture = RHICreateTexture(
                    FRHITextureCreateDesc::Create2D(TEXT("NormalTex"))
                    .SetExtent(Width, Height)
                    .SetFormat(PF_FloatRGBA)
                    .SetNumMips(1)
                    .SetNumSamples(1)
                    .SetFlags(ETextureCreateFlags::ShaderResource | ETextureCreateFlags::RenderTargetable)
                );
                NormalPooledTarget = CreateRenderTarget(NormalTexture, TEXT("BlendPCR.CameraNormal"));

                // Point Cloud Texture:
                QualityTexture = RHICreateTexture(
                    FRHITextureCreateDesc::Create2D(TEXT("QualityTex"))
                    .SetExtent(Width, Height)
                    .SetFormat(PF_FloatRGB)
                    .SetNumMips(1)
                    .SetNumSamples(1)
                    .SetFlags(ETextureCreateFlags::ShaderResource | ETextureCreateFlags::RenderTargetable)
                );
                QualityPooledTarget = CreateRenderTarget(QualityTexture, TEXT("BlendPCR.CameraQuality"));

                isInitialized = true;
            }
        );
    }

    void updatePointCloud(std::shared_ptr<RGBDImage> pc) {
        std::lock_guard<std::mutex> lock(pcMutex);
        currentPointCloud = pc;
        //UE_LOG(LogTemp, Warning, TEXT("Update Point Cloud of \"%s\"."), *FString(pc->cameraId.c_str()));
        reUploadPointCloud = true;
    }

    void updateTexturesByRenderThread(std::string cameraId, FRDGBuilder& GraphBuilder, ABlendPCROrigin* Origin)
    {
        check(IsInRenderingThread());

        if (Origin != nullptr && !niagaraTexturesInitRequested) {
            niagaraTexturesInitRequested = true;

            AsyncTask(ENamedThreads::GameThread, [this]() {
                ColorNiagaraRenderTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
                ColorNiagaraRenderTarget->RenderTargetFormat = RTF_RGBA8;
                ColorNiagaraRenderTarget->InitAutoFormat(Width, Height);
                ColorNiagaraRenderTarget->UpdateResourceImmediate(true);

                PointCloudNiagaraRenderTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
                PointCloudNiagaraRenderTarget->RenderTargetFormat = RTF_RGBA16f;
                PointCloudNiagaraRenderTarget->InitAutoFormat(Width, Height);
                PointCloudNiagaraRenderTarget->UpdateResourceImmediate(true);

                niagaraTexturesInitialized = true;
            });
        }
        std::shared_ptr<RGBDImage> localPC;
        {
            std::lock_guard<std::mutex> lock(pcMutex);
            if (!reUploadPointCloud || !currentPointCloud)
            {
                return; // Nichts zu tun
            }

            localPC = currentPointCloud;


            const FMatrix44f LocalMatrix = localPC->modelMatrix;
            const FMatrix44f OriginMatrix = FMatrix44f(Origin->GetActorTransform().ToMatrixWithScale().GetTransposed());

            ModelMatrix = OriginMatrix * LocalMatrix;

            reUploadPointCloud = false;
        }

        if (!isInitialized || !localPC)
            return;

       // const int32 Width = localPC->width;
       // const int32 Height = localPC->height;
        const size_t Resolution = static_cast<size_t>(Width) * Height;

        if (!ColorRawTexture.IsValid() || !DepthRawTexture.IsValid() || !LookupTexture.IsValid())
            return;

        // Upload Color Texture:
        {
            uint32 Stride = 0;
            void* TextureData = RHILockTexture2D(ColorRawTexture, 0, RLM_WriteOnly, Stride, false);
            if (TextureData && localPC->color){
                FMemory::Memcpy(TextureData, localPC->color, Resolution * sizeof(uint32));
            }
            RHIUnlockTexture2D(ColorRawTexture, 0, false);
        }

        // Upload Depth Texture:
        {
            uint32 Stride = 0;
            void* TextureData = RHILockTexture2D(DepthRawTexture, 0, RLM_WriteOnly, Stride, false);
            if (TextureData && localPC->depth){
                FMemory::Memcpy(TextureData, localPC->depth, Resolution * sizeof(uint16));
            }
            RHIUnlockTexture2D(DepthRawTexture, 0, false);
        }

        // Upload Lookup Texture:
        if(!lookupUploaded){
            uint32 Stride = 0;
            void* TextureData = RHILockTexture2D(LookupTexture, 0, RLM_WriteOnly, Stride, false);
            if (TextureData && localPC->lookup2D){
                FMemory::Memcpy(TextureData, localPC->lookup2D, Resolution * sizeof(float) * 2);
            }
            RHIUnlockTexture2D(LookupTexture, 0, false);
            lookupUploaded = true;
        }

        cameraPasses(cameraId, GraphBuilder, Origin);
    }

private:
    void cameraPasses(std::string cameraId, FRDGBuilder& GraphBuilder, ABlendPCROrigin* Origin);
};

class BLENDPCR_API GPUTextureManager
{
public:
    std::map<std::string, std::shared_ptr<GPUTextureSlot>> cameraSlots;

    bool registerCamera(std::string cameraId, int width, int height) {
        if (cameraSlots.count(cameraId) == 0) {

            std::shared_ptr<GPUTextureSlot> slot = std::make_shared<GPUTextureSlot>(width, height);
            cameraSlots.emplace(cameraId, slot);

            UE_LOG(LogTemp, Warning, TEXT("[BlendPCR] Create Textures Slot for %s with Resolution %i x %i "), *FString(cameraId.c_str()), width, height);
            return true;
        }

        return false;
    }

    void updatePointCloud(std::shared_ptr<RGBDImage> pc) {
        std::string cameraId = pc->cameraId;
        if (cameraSlots.count(cameraId) == 0) {
            registerCamera(pc->cameraId, pc->width, pc->height);
        }

        cameraSlots[cameraId]->updatePointCloud(pc);
    }

    void updateTexturesByRenderThread(FRDGBuilder& GraphBuilder, ABlendPCROrigin* Origin) {
        for (auto entry : cameraSlots) {
            entry.second->updateTexturesByRenderThread(entry.first, GraphBuilder, Origin);
        }
    }
};