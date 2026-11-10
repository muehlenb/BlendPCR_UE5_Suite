#include "BlendPCRViewExtension.h"

#include "BlendPCRShaders.h"

#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "Debug/DebugDrawService.h"
#include "Stats/Stats.h"
#include "RHI.h"
#include "PixelFormat.h"
#include "ScreenPass.h"
#include "PixelShaderUtils.h"
#include "PostProcess/PostProcessing.h"
#include "DynamicResolutionState.h"
#include "FXRenderingUtils.h"
#include "RenderGraphUtils.h"

#include "BlendPCRFunctionLibrary.h"

#include "UObject/ConstructorHelpers.h"

DEFINE_GPU_STAT(BlendPCR)

FBlendPCRViewExtension::FBlendPCRViewExtension(const FAutoRegister& AutoRegister)
    : FSceneViewExtensionBase(AutoRegister)
{ 
#if WITH_EDITOR
    // Registriere den DebugDraw-Callback für den Editor
    DebugDrawHandle = UDebugDrawService::Register(TEXT("Editor"), FDebugDrawDelegate::CreateRaw(this, &FBlendPCRViewExtension::DrawDebugOverlay));
#endif
}


FBlendPCRViewExtension::~FBlendPCRViewExtension()
{
#if WITH_EDITOR
    if (DebugDrawHandle.IsValid())
    {
        UDebugDrawService::Unregister(DebugDrawHandle);
        DebugDrawHandle.Reset();
    }
#endif
}


void FBlendPCRViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs)
{
    SCOPE_CYCLE_COUNTER(STAT_BlendPCR_PrePost);
    RDG_EVENT_SCOPE(GraphBuilder, "BlendPCR");
    RDG_GPU_STAT_SCOPE(GraphBuilder, BlendPCR);

    checkSlow(View.bIsViewInfo);
    const FViewInfo& ViewInfo = static_cast<const FViewInfo&>(View);
    const FIntRect PrimaryViewRect = UE::FXRenderingUtils::GetRawViewRectUnsafe(View);


    Inputs.Validate();
    FScreenPassTexture SceneColor((*Inputs.SceneTextures)->SceneColorTexture, PrimaryViewRect);
    FScreenPassTexture SceneDepth((*Inputs.SceneTextures)->SceneDepthTexture, PrimaryViewRect);
    if (!SceneColor.IsValid())
        return;
    
    const FIntPoint ScreenSize = SceneColor.ViewRect.Size();

    ABlendPCROrigin* Origin = GetPrimaryOrigin();
    if (Origin == nullptr || Origin->GetWorld() == nullptr)
        return;

    // Ensure that textures are updated by the current point clouds:
    GPUTextureManager& gpuTextureManager = UBlendPCRFunctionLibrary::GetGPUTextureManager();
    gpuTextureManager.updateTexturesByRenderThread(GraphBuilder, Origin);

    if(gpuTextureManager.cameraSlots.size() > 0){
        RDG_EVENT_SCOPE(GraphBuilder, "PCRMesh_UVPass");

        //const FIntPoint ScreenSize = PrimaryViewRect.Size();

        std::vector<std::shared_ptr<GPUTextureSlot>> textureSlots;
        for (auto& pair : gpuTextureManager.cameraSlots) {
            std::shared_ptr<GPUTextureSlot> Slot = pair.second;
            textureSlots.push_back(Slot);
        }

        std::vector<FRDGTextureRef> separateMeshTextures;
        std::vector<FRDGTextureRef> separateMeshTexturesDepth;

        for(std::shared_ptr<GPUTextureSlot> Slot : textureSlots){
            FRDGTextureDesc UVDesc = FRDGTextureDesc::Create2D(ScreenSize, PF_G16R16F, FClearValueBinding::Transparent, TexCreate_RenderTargetable | TexCreate_ShaderResource);
            FRDGTextureRef UVOutTexture = GraphBuilder.CreateTexture(UVDesc, TEXT("PCR.UVOutput"));
            separateMeshTextures.push_back(UVOutTexture);

            FRDGTextureDesc DepthDesc = FRDGTextureDesc::Create2D(ScreenSize, PF_DepthStencil, FClearValueBinding::DepthFar, TexCreate_DepthStencilTargetable | TexCreate_ShaderResource);
            FRDGTextureRef DepthTexture = GraphBuilder.CreateTexture(DepthDesc, TEXT("PCR.Depth"));
            separateMeshTexturesDepth.push_back(DepthTexture);

            FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
            TShaderMapRef<FSeparateMeshVS> VertexShader(ShaderMap);
            TShaderMapRef<FSeparateMeshPS> PixelShader(ShaderMap);

            FSeparateMeshVS::FParameters* VSParams = GraphBuilder.AllocParameters<FSeparateMeshVS::FParameters>();

            VSParams->ModelMatrix = Slot->ModelMatrix.GetTransposed();
            VSParams->ViewMatrix = FMatrix44f(View.ViewMatrices.GetViewMatrix());
            VSParams->ProjectionMatrix = FMatrix44f(View.ViewMatrices.GetProjectionMatrix());

            FRDGTextureRef MLSTex = GraphBuilder.RegisterExternalTexture(Slot->MLSPooledTarget, TEXT("PCR.Vertices"));
            FRDGTextureRef EdgeProximity = GraphBuilder.RegisterExternalTexture(Slot->EdgeDistancePooledTarget, TEXT("PCR.EdgeProximity"));
            FRDGTextureRef Color = GraphBuilder.RegisterExternalTexture(Slot->ColorPooledTarget, TEXT("PCR.Color"));

            VSParams->Stride = 2;
            VSParams->TexWidth = Slot->Width;
            VSParams->TexHeight = Slot->Height;
            VSParams->TextureVertices = MLSTex;
            VSParams->TextureEdgeProximity = EdgeProximity;
            VSParams->TextureColor = Color;
            VSParams->LinearSampler = TStaticSamplerState<SF_Bilinear>::GetRHI();

            FSeparateMeshVS::FParameters* PassParameters = GraphBuilder.AllocParameters<FSeparateMeshVS::FParameters>();
            PassParameters->RenderTargets[0] = FRenderTargetBinding(UVOutTexture, ERenderTargetLoadAction::EClear);
            PassParameters->RenderTargets.DepthStencil = FDepthStencilBinding(DepthTexture, ERenderTargetLoadAction::EClear, ERenderTargetLoadAction::ENoAction, FExclusiveDepthStencil::DepthWrite_StencilNop);

            PassParameters->TextureVertices = MLSTex;
            PassParameters->TextureEdgeProximity = EdgeProximity;
            PassParameters->TextureColor = Color;

            GraphBuilder.AddPass(
                RDG_EVENT_NAME("PCRMeshUVPass"),
                PassParameters,
                ERDGPassFlags::Raster,
                [VertexShader, PixelShader, VSParams, ScreenSize](FRHICommandList& RHICmdList)
                {
                    FGraphicsPipelineStateInitializer PSOInit;
                    RHICmdList.ApplyCachedRenderTargets(PSOInit);
                    PSOInit.BlendState = TStaticBlendState<>::GetRHI();
                    PSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
                    PSOInit.DepthStencilState = TStaticDepthStencilState<true, CF_GreaterEqual>::GetRHI();
                    PSOInit.PrimitiveType = PT_TriangleList;
                    PSOInit.BoundShaderState.VertexDeclarationRHI = GEmptyVertexDeclaration.VertexDeclarationRHI;
                    PSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
                    PSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();

                    RHICmdList.SetViewport(0, 0, 0.0f, ScreenSize.X, ScreenSize.Y, 1.0f);

                    SetGraphicsPipelineState(RHICmdList, PSOInit, 0);
                    SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), *VSParams);

                    const uint32 NumInstances = ((VSParams->TexHeight - 1) / VSParams->Stride) * ((VSParams->TexWidth - 1) / VSParams->Stride);
                    RHICmdList.DrawPrimitive(0, 2, NumInstances);
            });
        }

        FIntPoint ReducedScreenSize(ScreenSize.X / 2, ScreenSize.Y / 2);

        // Prepare Textures:
        std::vector<FRDGTextureRef> ScreenUVTextures;
        std::vector<FRDGTextureRef> ScreenDepthTextures;
        std::vector<FRDGTextureRef> PositionTextures;
        std::vector<FRDGTextureRef> QualityTextures;
        std::vector<bool> CameraActives;
        std::vector<FMatrix44f> ModelMatrices;

        for (int i = 0; i < 8; ++i) {
            bool validSlot = separateMeshTextures.size() > i;
            CameraActives.push_back(validSlot);
            if (separateMeshTextures.size() > i) {
                ScreenUVTextures.push_back(separateMeshTextures[i]);
                ScreenDepthTextures.push_back(separateMeshTexturesDepth[i]);
                FRDGTextureRef PositionTexture = GraphBuilder.RegisterExternalTexture(textureSlots[i]->MLSPooledTarget);
                PositionTextures.push_back(PositionTexture);
                FRDGTextureRef QualityTexture = GraphBuilder.RegisterExternalTexture(textureSlots[i]->QualityPooledTarget);
                QualityTextures.push_back(QualityTexture);
                ModelMatrices.push_back(textureSlots[i]->ModelMatrix);
            }
            else
            {
                ScreenUVTextures.push_back(separateMeshTextures[0]);
                ScreenDepthTextures.push_back(separateMeshTexturesDepth[0]);
                FRDGTextureRef PositionTexture = GraphBuilder.RegisterExternalTexture(textureSlots[0]->MLSPooledTarget);
                PositionTextures.push_back(PositionTexture);
                FRDGTextureRef QualityTexture = GraphBuilder.RegisterExternalTexture(textureSlots[0]->QualityPooledTarget);
                QualityTextures.push_back(QualityTexture);
                ModelMatrices.push_back(FMatrix44f::Identity);
            }
        }

        // Major Camera:
        FRDGTextureDesc UVDesc = FRDGTextureDesc::Create2D(ReducedScreenSize, PF_R8, FClearValueBinding::Transparent, TexCreate_RenderTargetable | TexCreate_ShaderResource);
        FRDGTextureRef MajorCamOut = GraphBuilder.CreateTexture(UVDesc, TEXT("PCR.MajorCam"));
        {
            FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
            TShaderMapRef<FMajorCamVS> VertexShader(ShaderMap);
            TShaderMapRef<FMajorCamPS> PixelShader(ShaderMap);

            FMajorCamVS::FParameters* VSParams = GraphBuilder.AllocParameters<FMajorCamVS::FParameters>();
            FMajorCamPS::FParameters* PSParams = GraphBuilder.AllocParameters<FMajorCamPS::FParameters>();

            PSParams->ScreenUV0 = ScreenUVTextures[0];
            PSParams->ScreenUV1 = ScreenUVTextures[1];
            PSParams->ScreenUV2 = ScreenUVTextures[2];
            PSParams->ScreenUV3 = ScreenUVTextures[3];
            PSParams->ScreenUV4 = ScreenUVTextures[4];
            PSParams->ScreenUV5 = ScreenUVTextures[5];
            PSParams->ScreenUV6 = ScreenUVTextures[6];
            PSParams->ScreenUV7 = ScreenUVTextures[7];

            PSParams->TexturePosition0 = PositionTextures[0];
            PSParams->TexturePosition1 = PositionTextures[1];
            PSParams->TexturePosition2 = PositionTextures[2];
            PSParams->TexturePosition3 = PositionTextures[3];
            PSParams->TexturePosition4 = PositionTextures[4];
            PSParams->TexturePosition5 = PositionTextures[5];
            PSParams->TexturePosition6 = PositionTextures[6];
            PSParams->TexturePosition7 = PositionTextures[7];

            PSParams->Quality0 = QualityTextures[0];
            PSParams->Quality1 = QualityTextures[1];
            PSParams->Quality2 = QualityTextures[2];
            PSParams->Quality3 = QualityTextures[3];
            PSParams->Quality4 = QualityTextures[4];
            PSParams->Quality5 = QualityTextures[5];
            PSParams->Quality6 = QualityTextures[6];
            PSParams->Quality7 = QualityTextures[7];

            PSParams->CameraActive0 = CameraActives[0] ? 1.0f : 0.0f;
            PSParams->CameraActive1 = CameraActives[1] ? 1.0f : 0.0f;
            PSParams->CameraActive2 = CameraActives[2] ? 1.0f : 0.0f;
            PSParams->CameraActive3 = CameraActives[3] ? 1.0f : 0.0f;
            PSParams->CameraActive4 = CameraActives[4] ? 1.0f : 0.0f;
            PSParams->CameraActive5 = CameraActives[5] ? 1.0f : 0.0f;
            PSParams->CameraActive6 = CameraActives[6] ? 1.0f : 0.0f;
            PSParams->CameraActive7 = CameraActives[7] ? 1.0f : 0.0f;

            PSParams->ModelMatrix0 = ModelMatrices[0];
            PSParams->ModelMatrix1 = ModelMatrices[1];
            PSParams->ModelMatrix2 = ModelMatrices[2];
            PSParams->ModelMatrix3 = ModelMatrices[3];
            PSParams->ModelMatrix4 = ModelMatrices[4];
            PSParams->ModelMatrix5 = ModelMatrices[5];
            PSParams->ModelMatrix6 = ModelMatrices[6];
            PSParams->ModelMatrix7 = ModelMatrices[7];

            PSParams->ViewMatrix = FMatrix44f(View.ViewMatrices.GetViewMatrix().GetTransposed());

            PSParams->LinearSampler = TStaticSamplerState<SF_Bilinear>::GetRHI();
            PSParams->RenderTargets[0] = FRenderTargetBinding(MajorCamOut, ERenderTargetLoadAction::EClear);

            GraphBuilder.AddPass(
                RDG_EVENT_NAME("PCRMajorCam"),
                PSParams,
                ERDGPassFlags::Raster,
                [VertexShader, PixelShader, VSParams, PSParams, ReducedScreenSize](FRHICommandList& RHICmdList)
                {
                    FGraphicsPipelineStateInitializer PSOInit;
                    RHICmdList.ApplyCachedRenderTargets(PSOInit);

                    PSOInit.BlendState = TStaticBlendState<>::GetRHI();
                    PSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
                    PSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
                    PSOInit.PrimitiveType = PT_TriangleList;
                    PSOInit.BoundShaderState.VertexDeclarationRHI = GScreenVertexDeclaration.VertexDeclarationRHI;
                    PSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
                    PSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();

                    RHICmdList.SetViewport(0, 0, 0.0f, ReducedScreenSize.X, ReducedScreenSize.Y, 1.0f);

                    SetGraphicsPipelineState(RHICmdList, PSOInit, 0);

                    // Shader-Parameter setzen
                    SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), *VSParams);
                    SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PSParams);

                    // Fullscreen-Triangle
                    RHICmdList.DrawPrimitive(0, 3, 1);
                });

            const FIntPoint SrcPos(0, 0);
            const FIntPoint SrcSize(ReducedScreenSize.X, ReducedScreenSize.Y);
            const FIntPoint DstPos(0, 0);
            const FIntPoint DstSize(ScreenSize.X, ScreenSize.Y);
        }

        FRDGTextureDesc CamWeightDesc1 = FRDGTextureDesc::Create2D(ReducedScreenSize, PF_R8G8B8A8, FClearValueBinding::Transparent, TexCreate_RenderTargetable | TexCreate_ShaderResource);
        FRDGTextureRef CamWeightOut1 = GraphBuilder.CreateTexture(CamWeightDesc1, TEXT("PCR.CamWeight1"));

        FRDGTextureDesc CamWeightDesc2 = FRDGTextureDesc::Create2D(ReducedScreenSize, PF_R8G8B8A8, FClearValueBinding::Transparent, TexCreate_RenderTargetable | TexCreate_ShaderResource);
        FRDGTextureRef CamWeightOut2 = GraphBuilder.CreateTexture(CamWeightDesc2, TEXT("PCR.CamWeight2"));
        // Camera Weights:
        {

            FCameraWeightsPS::FParameters* PSParams = GraphBuilder.AllocParameters<FCameraWeightsPS::FParameters>();
            PSParams->MajorCamTex = MajorCamOut;
            PSParams->LinearSampler = TStaticSamplerState<SF_Bilinear>::GetRHI();

            PSParams->RenderTargets[0] = FRenderTargetBinding(CamWeightOut1, ERenderTargetLoadAction::EClear);
            PSParams->RenderTargets[1] = FRenderTargetBinding(CamWeightOut2, ERenderTargetLoadAction::EClear);


            GraphBuilder.AddPass(
                RDG_EVENT_NAME("PCRCameraWeights"),
                PSParams,
                ERDGPassFlags::Raster,
                [PSParams, ReducedScreenSize](FRHICommandList& RHICmdList)
                {
                    RHICmdList.SetViewport(0, 0, 0.0f, ReducedScreenSize.X, ReducedScreenSize.Y, 1.0f);

                    TShaderMapRef<FCameraWeightsVS> VertexShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
                    TShaderMapRef<FCameraWeightsPS> PixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

                    FGraphicsPipelineStateInitializer PSOInit;
                    RHICmdList.ApplyCachedRenderTargets(PSOInit);

                    PSOInit.BlendState = TStaticBlendState<>::GetRHI();
                    PSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
                    PSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
                    PSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
                    PSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
                    PSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
                    PSOInit.PrimitiveType = PT_TriangleList;

                    SetGraphicsPipelineState(RHICmdList, PSOInit, 0);
                    SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PSParams);

                    RHICmdList.SetStreamSource(0, nullptr, 0);
                    RHICmdList.DrawPrimitive(0, 1, 1);
                }
            );

            const FIntPoint SrcPos(0, 0);
            const FIntPoint SrcSize(ReducedScreenSize.X, ReducedScreenSize.Y);
            const FIntPoint DstPos(0, 0);
            const FIntPoint DstSize(ScreenSize.X/2, ScreenSize.Y/2);
        }


        // Screen Blending Pass:
        // Screen Blending Pass - Mit Down/Upsampling
        {
            float resolutionScale = 1.0f;
            FIntPoint RenderSize(ScreenSize.X * resolutionScale, ScreenSize.Y * resolutionScale);

            // Output Textures erstellen
            FRDGTextureDesc BlendedColorDesc = FRDGTextureDesc::Create2D(
                RenderSize,
                PF_FloatRGBA,
                FClearValueBinding::Transparent,
                TexCreate_RenderTargetable | TexCreate_ShaderResource
            );
            FRDGTextureRef BlendedColorOut = GraphBuilder.CreateTexture(BlendedColorDesc, TEXT("PCR.BlendedColor"));

            FRDGTextureDesc BlendedDepthDesc = FRDGTextureDesc::Create2D(
                RenderSize,
                PF_R32_FLOAT,
                FClearValueBinding::White,
                TexCreate_RenderTargetable | TexCreate_ShaderResource
            );
            FRDGTextureRef BlendedDepthOut = GraphBuilder.CreateTexture(BlendedDepthDesc, TEXT("PCR.BlendedDepth"));

            FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
            TShaderMapRef<FScreenBlendingVS> VertexShader(ShaderMap);
            TShaderMapRef<FScreenBlendingPS> PixelShader(ShaderMap);

            FScreenBlendingVS::FParameters* VSParams = GraphBuilder.AllocParameters<FScreenBlendingVS::FParameters>();
            FScreenBlendingPS::FParameters* PSParams = GraphBuilder.AllocParameters<FScreenBlendingPS::FParameters>();

            // Screen UV Textures
            PSParams->ScreenUV0 = ScreenUVTextures[0];
            PSParams->ScreenUV1 = ScreenUVTextures[1];
            PSParams->ScreenUV2 = ScreenUVTextures[2];
            PSParams->ScreenUV3 = ScreenUVTextures[3];
            PSParams->ScreenUV4 = ScreenUVTextures[4];
            PSParams->ScreenUV5 = ScreenUVTextures[5];
            PSParams->ScreenUV6 = ScreenUVTextures[6];
            PSParams->ScreenUV7 = ScreenUVTextures[7];

            // Color Textures
            std::vector<FRDGTextureRef> ColorTextures;
            for (int i = 0; i < 8; ++i) {
                if (i < textureSlots.size()) {
                    ColorTextures.push_back(GraphBuilder.RegisterExternalTexture(textureSlots[i]->ColorPooledTarget, TEXT("PCR.Color")));
                }
                else {
                    ColorTextures.push_back(GraphBuilder.RegisterExternalTexture(textureSlots[0]->ColorPooledTarget, TEXT("PCR.Color")));
                }
            }

            PSParams->Color0 = ColorTextures[0];
            PSParams->Color1 = ColorTextures[1];
            PSParams->Color2 = ColorTextures[2];
            PSParams->Color3 = ColorTextures[3];
            PSParams->Color4 = ColorTextures[4];
            PSParams->Color5 = ColorTextures[5];
            PSParams->Color6 = ColorTextures[6];
            PSParams->Color7 = ColorTextures[7];

            // Vertices Textures
            PSParams->Vertices0 = PositionTextures[0];
            PSParams->Vertices1 = PositionTextures[1];
            PSParams->Vertices2 = PositionTextures[2];
            PSParams->Vertices3 = PositionTextures[3];
            PSParams->Vertices4 = PositionTextures[4];
            PSParams->Vertices5 = PositionTextures[5];
            PSParams->Vertices6 = PositionTextures[6];
            PSParams->Vertices7 = PositionTextures[7];

            // Quality Textures
            PSParams->Quality0 = QualityTextures[0];
            PSParams->Quality1 = QualityTextures[1];
            PSParams->Quality2 = QualityTextures[2];
            PSParams->Quality3 = QualityTextures[3];
            PSParams->Quality4 = QualityTextures[4];
            PSParams->Quality5 = QualityTextures[5];
            PSParams->Quality6 = QualityTextures[6];
            PSParams->Quality7 = QualityTextures[7];

            // Screen Depth Textures
            PSParams->ScreenDepth0 = ScreenDepthTextures[0];
            PSParams->ScreenDepth1 = ScreenDepthTextures[1];
            PSParams->ScreenDepth2 = ScreenDepthTextures[2];
            PSParams->ScreenDepth3 = ScreenDepthTextures[3];
            PSParams->ScreenDepth4 = ScreenDepthTextures[4];
            PSParams->ScreenDepth5 = ScreenDepthTextures[5];
            PSParams->ScreenDepth6 = ScreenDepthTextures[6];
            PSParams->ScreenDepth7 = ScreenDepthTextures[7];

            // Weight Textures
            PSParams->MiniWeightsA = CamWeightOut1;
            PSParams->MiniWeightsB = CamWeightOut2;

            // Camera Active Flags
            PSParams->CameraActive0 = CameraActives[0] ? 1.0f : 0.0f;
            PSParams->CameraActive1 = CameraActives[1] ? 1.0f : 0.0f;
            PSParams->CameraActive2 = CameraActives[2] ? 1.0f : 0.0f;
            PSParams->CameraActive3 = CameraActives[3] ? 1.0f : 0.0f;
            PSParams->CameraActive4 = CameraActives[4] ? 1.0f : 0.0f;
            PSParams->CameraActive5 = CameraActives[5] ? 1.0f : 0.0f;
            PSParams->CameraActive6 = CameraActives[6] ? 1.0f : 0.0f;
            PSParams->CameraActive7 = CameraActives[7] ? 1.0f : 0.0f;

            PSParams->ModelMatrix0 = ModelMatrices[0];
            PSParams->ModelMatrix1 = ModelMatrices[1];
            PSParams->ModelMatrix2 = ModelMatrices[2];
            PSParams->ModelMatrix3 = ModelMatrices[3];
            PSParams->ModelMatrix4 = ModelMatrices[4];
            PSParams->ModelMatrix5 = ModelMatrices[5];
            PSParams->ModelMatrix6 = ModelMatrices[6];
            PSParams->ModelMatrix7 = ModelMatrices[7];

            PSParams->ViewMatrix = FMatrix44f(View.ViewMatrices.GetViewMatrix().GetTransposed());

            PSParams->PreColorSaturation = Origin->PreSaturation;
            PSParams->PreColorMultiplier = Origin->PreMultiplier;
            PSParams->ColorHueShift = Origin->HueShift;
            PSParams->ColorSaturation = Origin->Saturation;
            PSParams->ColorBrightness = Origin->Brightness;
            PSParams->ColorContrast = Origin->Contrast;
            PSParams->ColorEdgeAlpha = Origin->EdgeAlpha;
            PSParams->ColorAlpha = Origin->Alpha;

            // Sampler
            PSParams->LinearSampler = TStaticSamplerState<SF_Bilinear>::GetRHI();

            // Render Targets - getrennt Color und Depth
            PSParams->RenderTargets[0] = FRenderTargetBinding(BlendedColorOut, ERenderTargetLoadAction::EClear);
            PSParams->RenderTargets[1] = FRenderTargetBinding(BlendedDepthOut, ERenderTargetLoadAction::EClear);

            GraphBuilder.AddPass(
                RDG_EVENT_NAME("PCRScreenBlending"),
                PSParams,
                ERDGPassFlags::Raster,
                [VertexShader, PixelShader, VSParams, PSParams, RenderSize](FRHICommandList& RHICmdList)
                {
                    RHICmdList.SetViewport(0, 0, 0.0f, RenderSize.X, RenderSize.Y, 1.0f);

                    FGraphicsPipelineStateInitializer PSOInit;
                    RHICmdList.ApplyCachedRenderTargets(PSOInit);

                    PSOInit.BlendState = TStaticBlendState<>::GetRHI();
                    PSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
                    PSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
                    PSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
                    PSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
                    PSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
                    PSOInit.PrimitiveType = PT_TriangleList;

                    SetGraphicsPipelineState(RHICmdList, PSOInit, 0);
                    SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), *VSParams);
                    SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PSParams);

                    RHICmdList.SetStreamSource(0, nullptr, 0);
                    RHICmdList.DrawPrimitive(0, 1, 1);
                }
            );

            // Composite Pass: Blende das Ergebnis mit Depth Test in die Scene
            AddCompositeWithDepthPass(GraphBuilder, View, BlendedColorOut, BlendedDepthOut,
                SceneColor.Texture, SceneDepth.Texture, SceneColor.ViewRect);
        }
    }
}

void FBlendPCRViewExtension::AddCompositeWithDepthPass(
    FRDGBuilder& GraphBuilder,
    const FSceneView& View,
    FRDGTextureRef SourceColor,
    FRDGTextureRef SourceDepth,
    FRDGTextureRef DestColor,
    FRDGTextureRef SceneDepth, 
    FIntRect ViewRect)
{
    TShaderMapRef<FCompositeWithDepthVS> VertexShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
    TShaderMapRef<FCompositeWithDepthPS> PixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    FCompositeWithDepthPS::FParameters* PSParams = GraphBuilder.AllocParameters<FCompositeWithDepthPS::FParameters>();
    PSParams->SourceColor = SourceColor;
    PSParams->SourceDepth = SourceDepth;
    PSParams->SceneDepth = SceneDepth;  // Nur zum Lesen!
    PSParams->PointSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
    PSParams->LinearSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

    FVector4f ViewRectParams;
    ViewRectParams.X = ViewRect.Min.X;
    ViewRectParams.Y = ViewRect.Min.Y;
    ViewRectParams.Z = ViewRect.Width();
    ViewRectParams.W = ViewRect.Height();

    FVector2f SceneTextureSize(SceneDepth->Desc.Extent.X, SceneDepth->Desc.Extent.Y);

    PSParams->ViewRect = ViewRectParams;
    PSParams->SceneTexSize = SceneTextureSize;

    PSParams->RenderTargets[0] = FRenderTargetBinding(DestColor, ERenderTargetLoadAction::ELoad);

    GraphBuilder.AddPass(
        RDG_EVENT_NAME("PCRCompositeWithDepth"),
        PSParams,
        ERDGPassFlags::Raster,
        [VertexShader, PixelShader, PSParams, ViewRect](FRHICommandList& RHICmdList)
        {
            RHICmdList.SetViewport(ViewRect.Min.X, ViewRect.Min.Y, 0.0f, ViewRect.Max.X, ViewRect.Max.Y, 1.0f);

            FGraphicsPipelineStateInitializer PSOInit;
            RHICmdList.ApplyCachedRenderTargets(PSOInit);

            // Alpha Blending
            using FAlphaBlend = TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha, BO_Add, BF_One, BF_InverseSourceAlpha>;
            PSOInit.BlendState = FAlphaBlend::GetRHI();
            PSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();

            // KEIN Hardware Depth Test - wird im Shader manuell gemacht!
            PSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

            PSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
            PSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
            PSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
            PSOInit.PrimitiveType = PT_TriangleList;

            SetGraphicsPipelineState(RHICmdList, PSOInit, 0);
            SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PSParams);

            RHICmdList.SetStreamSource(0, nullptr, 0);
            RHICmdList.DrawPrimitive(0, 1, 1);
        }
    );
}




void FBlendPCRViewExtension::RenderDebugCameraPasses(FRDGBuilder& GraphBuilder, const FSceneView& View, const FScreenPassTexture& SceneColor) {
    GPUTextureManager& gpuTextureManager = UBlendPCRFunctionLibrary::GetGPUTextureManager();
    if (gpuTextureManager.cameraSlots.size() > 0)
    {
        for (int i = 0; i < 7; ++i) {
            std::shared_ptr<GPUTextureSlot> Slot = gpuTextureManager.cameraSlots["Virtual Camera " + std::to_string(i)];
            TArray<TRefCountPtr<IPooledRenderTarget>> PooledTargets = {
                Slot->ColorPooledTarget,
                Slot->PointCloudPooledTarget,
                Slot->RejectionPooledTarget,
                Slot->EdgeDistancePooledTarget,
                Slot->MLSPooledTarget,
                Slot->NormalPooledTarget,
                Slot->QualityPooledTarget
            };

            float scaling = 1;

            if (Slot && Slot->ColorTexture.IsValid())
            {
                for (int h = 0; h < PooledTargets.Num(); ++h) {
                    FRDGTextureRef RDGSourceTexture = GraphBuilder.RegisterExternalTexture(PooledTargets[h], TEXT("BlendPCR.Debug"), ERDGTextureFlags::SkipTracking);

                    const FViewInfo& ViewInfo = static_cast<const FViewInfo&>(View);

                    const FIntPoint SrcPos(0, 0);
                    const FIntPoint SrcSize(Slot->Width, Slot->Height);
                    const FIntPoint DstPos(155 * h * scaling, 140 * i * scaling);
                    const FIntPoint DstSize(150 * scaling, 135 * scaling);

                    AddDrawTexturePass(GraphBuilder, View, RDGSourceTexture, SceneColor.Texture, SrcPos, SrcSize, DstPos, DstSize);
                }
            }
        }
    }
}

void FBlendPCRViewExtension::RegisterBlendPCROrigin(ABlendPCROrigin* Actor)
{
    if (Actor && !RegisteredActors.Contains(Actor))
    {
        RegisteredActors.Add(Actor);
        UE_LOG(LogTemp, Log, TEXT("[BlendPCR] Origin added to world: %s (%i)"), *Actor->GetName(), RegisteredActors.Num());

        for (TWeakObjectPtr<ABlendPCROrigin> a : RegisteredActors) {

            UE_LOG(LogTemp, Log, TEXT("[BlendPCR] --- %s (%i)"), *a->GetName(), RegisteredActors.Num());
        }

    }
}

void FBlendPCRViewExtension::UnregisterBlendPCROrigin(ABlendPCROrigin* Actor)
{
    if (Actor && RegisteredActors.Contains(Actor))
    {
        RegisteredActors.Remove(Actor);
        UE_LOG(LogTemp, Log, TEXT("[BlendPCR] Origin removed from world: %s (%i)"), *Actor->GetName(), RegisteredActors.Num());
    }
}

void FBlendPCRViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
{
}

ABlendPCROrigin* FBlendPCRViewExtension::GetPrimaryOrigin() const
{
    if (RegisteredActors.Num() == 0)
        return nullptr;

    for (TWeakObjectPtr<ABlendPCROrigin> OriginPtr : RegisteredActors)
    {
        if (!OriginPtr.IsValid())
            continue;

        if (ABlendPCROrigin* Origin = OriginPtr.Get())
        {
            if (UWorld* World = Origin->GetWorld())
            {
                if (World->IsGameWorld())
                {
                    return Origin;
                }
            }
        }
    }

    for (TWeakObjectPtr<ABlendPCROrigin> OriginPtr : RegisteredActors)
    {
        if (!OriginPtr.IsValid())
            continue;

        if (ABlendPCROrigin* Origin = OriginPtr.Get())
        {
            return Origin;
        }
    }

    return nullptr;
}


#if WITH_EDITOR
void FBlendPCRViewExtension::DrawDebugOverlay(UCanvas* Canvas, APlayerController* Controller)
{
    if (!Canvas)
        return;

    int32 NumValidOrigins = 0;
    for (const TWeakObjectPtr<ABlendPCROrigin>& Ptr : RegisteredActors)
    {
        if (Ptr.IsValid())
            ++NumValidOrigins;
    }

    if (NumValidOrigins > 1)
    {
        const FString Msg = FString::Printf(
            TEXT("⚠ [BlendPCR] Multiple BlendPCROrigins detected (%d) — only one should exist! Only the first one is used."),
            NumValidOrigins);

        FCanvasTextItem TextItem(
            FVector2D(50.f, 50.f),
            FText::FromString(Msg),
            GEngine->GetMediumFont(),
            FLinearColor::Red
        );
        TextItem.EnableShadow(FLinearColor::Black);

        Canvas->DrawItem(TextItem);
    }
}
#endif