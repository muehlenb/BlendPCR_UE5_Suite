#include "GPUTextureManager.h"

#include "Stats/Stats.h"
#include "RHI.h"
#include "PixelFormat.h"
#include "ScreenPass.h"
#include "PixelShaderUtils.h"
#include "PostProcess/PostProcessing.h"
#include "DynamicResolutionState.h"
#include "FXRenderingUtils.h"
#include "RenderGraphUtils.h"

#include "BlendPCRShaders.h"

void GPUTextureSlot::cameraPasses(std::string cameraId, FRDGBuilder& GraphBuilder, ABlendPCROrigin* Origin) {
    // Hole Filling Pass:
    {
        FRDGTextureRef RDGDepthRawTex = GraphBuilder.RegisterExternalTexture(DepthRawPooledTarget);
        FRDGTextureRef RDGColorRawTex = GraphBuilder.RegisterExternalTexture(ColorRawPooledTarget);
        FRDGTextureRef RDGDepthTex = GraphBuilder.RegisterExternalTexture(DepthPooledTarget);
        FRDGTextureRef RDGColorTex = GraphBuilder.RegisterExternalTexture(ColorPooledTarget);

        FHoleFillingPS::FParameters* PSParams = GraphBuilder.AllocParameters<FHoleFillingPS::FParameters>();
        PSParams->DepthTex = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(RDGDepthRawTex));
        PSParams->ColorTex = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(RDGColorRawTex));
        PSParams->PointSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
        PSParams->RenderTargets[0] = FRenderTargetBinding(RDGDepthTex, ERenderTargetLoadAction::EClear);
        PSParams->RenderTargets[1] = FRenderTargetBinding(RDGColorTex, ERenderTargetLoadAction::EClear);
        PSParams->TexSize = FVector2f(Width, Height);

        GraphBuilder.AddPass(
            RDG_EVENT_NAME("HoleFilling"),
            PSParams,
            ERDGPassFlags::Raster,
            [PSParams, this](FRHICommandList& RHICmdList)
            {
                RHICmdList.SetViewport(0, 0, 0.0f, Width, Height, 1.0f);

                TShaderMapRef<FHoleFillingVS> VertexShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
                TShaderMapRef<FHoleFillingPS> PixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

                FGraphicsPipelineStateInitializer PSOInit;
                RHICmdList.ApplyCachedRenderTargets(PSOInit);
                PSOInit.BlendState = TStaticBlendState<>::GetRHI();
                PSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
                PSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
                PSOInit.BoundShaderState.VertexDeclarationRHI = GScreenVertexDeclaration.VertexDeclarationRHI;
                PSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
                PSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
                PSOInit.PrimitiveType = PT_TriangleList;
                SetGraphicsPipelineState(RHICmdList, PSOInit);

                SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PSParams);

                // Vollbild-Quad zeichnen
                RHICmdList.DrawPrimitive(0, 2, 1);
            }
        );
    }


    // Generate Point Cloud Pass: 
    {
        FRDGTextureRef RDGDepthTex = GraphBuilder.RegisterExternalTexture(DepthPooledTarget);
        FRDGTextureRef RDGLookupTex = GraphBuilder.RegisterExternalTexture(LookupPooledTarget);
        FRDGTextureRef RDGPointCloudTex = GraphBuilder.RegisterExternalTexture(PointCloudPooledTarget);

        FPointCloudPS::FParameters* PSParams = GraphBuilder.AllocParameters<FPointCloudPS::FParameters>();
        PSParams->DepthTex = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(RDGDepthTex));
        PSParams->LookupTex = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(RDGLookupTex));
        PSParams->PointSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
        PSParams->RenderTargets[0] = FRenderTargetBinding(RDGPointCloudTex, ERenderTargetLoadAction::EClear);
        PSParams->TexSize = FVector2f(Width, Height);

        GraphBuilder.AddPass(
            RDG_EVENT_NAME("GeneratePointCloud"),
            PSParams,
            ERDGPassFlags::Raster,
            [PSParams, this](FRHICommandList& RHICmdList)
            {
                RHICmdList.SetViewport(0, 0, 0.0f, Width, Height, 1.0f);

                TShaderMapRef<FPointCloudVS> VertexShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
                TShaderMapRef<FPointCloudPS> PixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

                FGraphicsPipelineStateInitializer PSOInit;
                RHICmdList.ApplyCachedRenderTargets(PSOInit);
                PSOInit.BlendState = TStaticBlendState<>::GetRHI();
                PSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
                PSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
                PSOInit.BoundShaderState.VertexDeclarationRHI = GScreenVertexDeclaration.VertexDeclarationRHI;
                PSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
                PSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
                PSOInit.PrimitiveType = PT_TriangleList;
                SetGraphicsPipelineState(RHICmdList, PSOInit);

                SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PSParams);

                // Vollbild-Quad zeichnen
                RHICmdList.DrawPrimitive(0, 2, 1);
            }
        );
    }

    // Generate Rejection Texture:
    {
        FRDGTextureRef RDGPointCloudTex = GraphBuilder.RegisterExternalTexture(PointCloudPooledTarget);
        FRDGTextureRef RDGColorTex = GraphBuilder.RegisterExternalTexture(ColorPooledTarget);
        FRDGTextureRef RDGRejectionTex = GraphBuilder.RegisterExternalTexture(RejectionPooledTarget);

        FRejectionPS::FParameters* PSParams = GraphBuilder.AllocParameters<FRejectionPS::FParameters>();
        PSParams->PointCloudTex = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(RDGPointCloudTex));
        PSParams->ColorTex = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(RDGColorTex));
        PSParams->PointSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

        PSParams->ShouldClip = Origin->bClippingActive; // oder 1, falls gewünscht
        PSParams->ClipTransform = FMatrix44f(Origin->ClippingBox->GetComponentTransform().ToMatrixWithScale().Inverse());

        PSParams->ModelMatrix = ModelMatrix.GetTransposed();
        PSParams->TexSize = FVector2f(Width, Height);

        PSParams->RenderTargets[0] = FRenderTargetBinding(RDGRejectionTex, ERenderTargetLoadAction::EClear);

        GraphBuilder.AddPass(
            RDG_EVENT_NAME("GenerateRejectionTexture"),
            PSParams,
            ERDGPassFlags::Raster,
            [PSParams, this](FRHICommandList& RHICmdList)
            {
                RHICmdList.SetViewport(0, 0, 0.0f, Width, Height, 1.0f);

                TShaderMapRef<FRejectionVS> VertexShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
                TShaderMapRef<FRejectionPS> PixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

                FGraphicsPipelineStateInitializer PSOInit;
                RHICmdList.ApplyCachedRenderTargets(PSOInit);
                PSOInit.BlendState = TStaticBlendState<>::GetRHI();
                PSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
                PSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
                PSOInit.BoundShaderState.VertexDeclarationRHI = GEmptyVertexDeclaration.VertexDeclarationRHI;
                PSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
                PSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
                PSOInit.PrimitiveType = PT_TriangleList;

                SetGraphicsPipelineState(RHICmdList, PSOInit);
                SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PSParams);

                // Fullscreen-Triangle
                RHICmdList.DrawPrimitive(0, 3, 1);
            }
        );
    }


    // Generate Edge Distance Texture:
    {
        FRDGTextureRef RDGRejectedTex = GraphBuilder.RegisterExternalTexture(RejectionPooledTarget);
        FRDGTextureRef RDGEdgeDistanceTex = GraphBuilder.RegisterExternalTexture(EdgeDistancePooledTarget);

        FEdgeDistancePS::FParameters* PSParams = GraphBuilder.AllocParameters<FEdgeDistancePS::FParameters>();
        PSParams->RejectedTex = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(RDGRejectedTex));
        PSParams->PointSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
        PSParams->TexSize = FVector2f(Width, Height);
        PSParams->RenderTargets[0] = FRenderTargetBinding(RDGEdgeDistanceTex, ERenderTargetLoadAction::EClear);

        GraphBuilder.AddPass(
            RDG_EVENT_NAME("GenerateEdgeDistanceTexture"),
            PSParams,
            ERDGPassFlags::Raster,
            [PSParams, this](FRHICommandList& RHICmdList)
            {
                RHICmdList.SetViewport(0, 0, 0.0f, Width, Height, 1.0f);

                TShaderMapRef<FEdgeDistanceVS> VertexShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
                TShaderMapRef<FEdgeDistancePS> PixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

                FGraphicsPipelineStateInitializer PSOInit;
                RHICmdList.ApplyCachedRenderTargets(PSOInit);
                PSOInit.BlendState = TStaticBlendState<>::GetRHI();
                PSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
                PSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
                PSOInit.BoundShaderState.VertexDeclarationRHI = GEmptyVertexDeclaration.VertexDeclarationRHI;
                PSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
                PSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
                PSOInit.PrimitiveType = PT_TriangleList;

                SetGraphicsPipelineState(RHICmdList, PSOInit);
                SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PSParams);

                // Fullscreen-Triangle
                RHICmdList.DrawPrimitive(0, 3, 1);
            }
        );
    }

    // Generate Moving Least Squares (MLS) Pass Texture
    {
        FRDGTextureRef RDGPointCloudTex = GraphBuilder.RegisterExternalTexture(PointCloudPooledTarget);
        FRDGTextureRef RDGEdgeDistanceTex = GraphBuilder.RegisterExternalTexture(EdgeDistancePooledTarget);
        FRDGTextureRef RDGMLSTex = GraphBuilder.RegisterExternalTexture(MLSPooledTarget);

        FMLSPointCloudPS::FParameters* PSParams = GraphBuilder.AllocParameters<FMLSPointCloudPS::FParameters>();
        PSParams->PointCloudTex = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(RDGPointCloudTex));
        PSParams->EdgeProximityTex = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(RDGEdgeDistanceTex));
        PSParams->PointSampler = TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
        PSParams->p_h = 0.01f;
        PSParams->kernelSpread = 1.0f;
        PSParams->kernelRadius = 8;
        PSParams->TexSize = FVector2f(Width, Height);
        PSParams->RenderTargets[0] = FRenderTargetBinding(RDGMLSTex, ERenderTargetLoadAction::EClear);

        GraphBuilder.AddPass(
            RDG_EVENT_NAME("GenerateMLSTexture"),
            PSParams,
            ERDGPassFlags::Raster,
            [PSParams, this](FRHICommandList& RHICmdList)
            {
                RHICmdList.SetViewport(0, 0, 0.0f, Width, Height, 1.0f);

                TShaderMapRef<FMLSPointCloudVS> VertexShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
                TShaderMapRef<FMLSPointCloudPS> PixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

                FGraphicsPipelineStateInitializer PSOInit;
                RHICmdList.ApplyCachedRenderTargets(PSOInit);
                PSOInit.BlendState = TStaticBlendState<>::GetRHI();
                PSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
                PSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
                PSOInit.BoundShaderState.VertexDeclarationRHI = GEmptyVertexDeclaration.VertexDeclarationRHI;
                PSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
                PSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
                PSOInit.PrimitiveType = PT_TriangleList;

                SetGraphicsPipelineState(RHICmdList, PSOInit);
                SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PSParams);
                RHICmdList.DrawPrimitive(0, 3, 1);
            }
        );
    }

    // Generate Normal Texture Pass
    {
        FRDGTextureRef RDGInputVerts = GraphBuilder.RegisterExternalTexture(PointCloudPooledTarget);
        FRDGTextureRef RDGMLSVerts = GraphBuilder.RegisterExternalTexture(MLSPooledTarget);
        FRDGTextureRef RDGEdgeTex = GraphBuilder.RegisterExternalTexture(EdgeDistancePooledTarget);
        FRDGTextureRef RDGNormalTex = GraphBuilder.RegisterExternalTexture(NormalPooledTarget);

        FNormalPS::FParameters* PSParams = GraphBuilder.AllocParameters<FNormalPS::FParameters>();
        PSParams->InputVerticesTex = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(RDGInputVerts));
        PSParams->MLSVerticesTex = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(RDGMLSVerts));
        PSParams->EdgeProximityTex = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(RDGEdgeTex));
        PSParams->PointSampler = TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
        PSParams->p_h = 0.2f;
        PSParams->kernelSpread = 2.0f;
        PSParams->kernelRadius = 5;
        PSParams->TexSize = FVector2f(Width, Height);
        PSParams->RenderTargets[0] = FRenderTargetBinding(RDGNormalTex, ERenderTargetLoadAction::EClear);

        GraphBuilder.AddPass(
            RDG_EVENT_NAME("GenerateNormalTexture"),
            PSParams,
            ERDGPassFlags::Raster,
            [PSParams, this](FRHICommandList& RHICmdList)
            {
                RHICmdList.SetViewport(0, 0, 0.0f, Width, Height, 1.0f);

                TShaderMapRef<FNormalVS> VertexShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
                TShaderMapRef<FNormalPS> PixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

                FGraphicsPipelineStateInitializer PSOInit;
                RHICmdList.ApplyCachedRenderTargets(PSOInit);
                PSOInit.BlendState = TStaticBlendState<>::GetRHI();
                PSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
                PSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
                PSOInit.BoundShaderState.VertexDeclarationRHI = GEmptyVertexDeclaration.VertexDeclarationRHI;
                PSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
                PSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
                PSOInit.PrimitiveType = PT_TriangleList;

                SetGraphicsPipelineState(RHICmdList, PSOInit);
                SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PSParams);
                RHICmdList.DrawPrimitive(0, 3, 1);
            }
        );
    }


    // -----------------------------------------------------------
// Generate Quality Texture Pass
// -----------------------------------------------------------
    {
        FRDGTextureRef RDGVerticesTex = GraphBuilder.RegisterExternalTexture(MLSPooledTarget);
        FRDGTextureRef RDGNormalsTex = GraphBuilder.RegisterExternalTexture(NormalPooledTarget);
        FRDGTextureRef RDGEdgeDistTex = GraphBuilder.RegisterExternalTexture(EdgeDistancePooledTarget);
        FRDGTextureRef RDGQualityTex = GraphBuilder.RegisterExternalTexture(QualityPooledTarget);

        FQualityPS::FParameters* PSParams = GraphBuilder.AllocParameters<FQualityPS::FParameters>();
        PSParams->VerticesTex = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(RDGVerticesTex));
        PSParams->NormalsTex = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(RDGNormalsTex));
        PSParams->EdgeDistTex = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(RDGEdgeDistTex));
        PSParams->PointSampler = TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
        PSParams->RenderTargets[0] = FRenderTargetBinding(RDGQualityTex, ERenderTargetLoadAction::EClear);

        GraphBuilder.AddPass(
            RDG_EVENT_NAME("GenerateQuality"),
            PSParams,
            ERDGPassFlags::Raster,
            [PSParams, this](FRHICommandList& RHICmdList)
            {
                RHICmdList.SetViewport(0, 0, 0.0f, Width, Height, 1.0f);

                TShaderMapRef<FQualityVS> VertexShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
                TShaderMapRef<FQualityPS> PixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

                FGraphicsPipelineStateInitializer PSOInit;
                RHICmdList.ApplyCachedRenderTargets(PSOInit);
                PSOInit.BlendState = TStaticBlendState<>::GetRHI();
                PSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
                PSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
                PSOInit.BoundShaderState.VertexDeclarationRHI = GScreenVertexDeclaration.VertexDeclarationRHI;
                PSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
                PSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
                PSOInit.PrimitiveType = PT_TriangleList;
                SetGraphicsPipelineState(RHICmdList, PSOInit);

                SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PSParams);

                RHICmdList.DrawPrimitive(0, 2, 1);
            }
        );
    }

    if (niagaraTexturesInitialized) {
        FRDGTextureRef ColorRDGSource = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(ColorRawTexture, TEXT("ColorRawRT")));
        if (ColorNiagaraRenderTarget == nullptr || !ColorNiagaraRenderTarget->IsValidLowLevelFast())
            return;

        auto targetResource = ColorNiagaraRenderTarget->GetRenderTargetResource();

        if (targetResource != nullptr) {
            FRDGTextureRef ColorRDGDest = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(targetResource->GetRenderTargetTexture(), TEXT("ColorNiagaraRT")));

            FRHICopyTextureInfo CopyInfo;
            AddCopyTexturePass(GraphBuilder, ColorRDGSource, ColorRDGDest, CopyInfo);

            FRDGTextureRef PCRDGSource = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(PointCloudTexture, TEXT("PCRawRT")));
            FRDGTextureRef PCRDGDest = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(PointCloudNiagaraRenderTarget->GetRenderTargetResource()->GetRenderTargetTexture(), TEXT("PCNiagaraRT")));

            FRHICopyTextureInfo PCCopyInfo;
            AddCopyTexturePass(GraphBuilder, PCRDGSource, PCRDGDest, PCCopyInfo);

            FString cameraIdUE(cameraId.c_str());
            Origin->UpdateNiagaraSystem(cameraIdUE, ColorNiagaraRenderTarget, PointCloudNiagaraRenderTarget, Width, Height);
        }
    }
}