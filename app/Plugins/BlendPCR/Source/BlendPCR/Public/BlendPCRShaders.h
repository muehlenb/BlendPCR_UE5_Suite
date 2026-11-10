#pragma once

#include "GlobalShader.h"
#include "Runtime/Renderer/Public/ScreenPass.h"

class FHoleFillingVS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FHoleFillingVS);
    SHADER_USE_PARAMETER_STRUCT(FHoleFillingVS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    END_SHADER_PARAMETER_STRUCT()
};

class FHoleFillingPS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FHoleFillingPS);
    SHADER_USE_PARAMETER_STRUCT(FHoleFillingPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<uint>, DepthTex)
        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, ColorTex)
        SHADER_PARAMETER_SAMPLER(SamplerState, PointSampler)
        SHADER_PARAMETER(FVector2f, TexSize)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
};

class FPointCloudVS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FPointCloudVS);
    SHADER_USE_PARAMETER_STRUCT(FPointCloudVS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    END_SHADER_PARAMETER_STRUCT()
};

class FPointCloudPS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FPointCloudPS);
    SHADER_USE_PARAMETER_STRUCT(FPointCloudPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<uint>, DepthTex)
        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float2>, LookupTex)
        SHADER_PARAMETER_SAMPLER(SamplerState, PointSampler)
        SHADER_PARAMETER(FVector2f, TexSize)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
};

class FRejectionVS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FRejectionVS);
    SHADER_USE_PARAMETER_STRUCT(FRejectionVS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    END_SHADER_PARAMETER_STRUCT()
};

class FRejectionPS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FRejectionPS);
    SHADER_USE_PARAMETER_STRUCT(FRejectionPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, PointCloudTex)
        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, ColorTex)
        SHADER_PARAMETER_SAMPLER(SamplerState, PointSampler)
        SHADER_PARAMETER(uint32, ShouldClip)
        SHADER_PARAMETER(FMatrix44f, ClipTransform)
        SHADER_PARAMETER(FMatrix44f, ModelMatrix)
        SHADER_PARAMETER(FVector2f, TexSize)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
};

class FEdgeDistanceVS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FEdgeDistanceVS);
    SHADER_USE_PARAMETER_STRUCT(FEdgeDistanceVS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    END_SHADER_PARAMETER_STRUCT()
};

class FEdgeDistancePS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FEdgeDistancePS);
    SHADER_USE_PARAMETER_STRUCT(FEdgeDistancePS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, RejectedTex)
        SHADER_PARAMETER_SAMPLER(SamplerState, PointSampler)
        SHADER_PARAMETER(FVector2f, TexSize)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
};

class FMLSPointCloudVS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FMLSPointCloudVS);
    SHADER_USE_PARAMETER_STRUCT(FMLSPointCloudVS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    END_SHADER_PARAMETER_STRUCT()
};

class FMLSPointCloudPS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FMLSPointCloudPS);
    SHADER_USE_PARAMETER_STRUCT(FMLSPointCloudPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, PointCloudTex)
        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, EdgeProximityTex)
        SHADER_PARAMETER_SAMPLER(SamplerState, PointSampler)
        SHADER_PARAMETER(float, p_h)
        SHADER_PARAMETER(float, kernelSpread)
        SHADER_PARAMETER(int32, kernelRadius)
        SHADER_PARAMETER(FVector2f, TexSize)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
};

class FNormalVS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FNormalVS);
    SHADER_USE_PARAMETER_STRUCT(FNormalVS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    END_SHADER_PARAMETER_STRUCT()
};

class FNormalPS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FNormalPS);
    SHADER_USE_PARAMETER_STRUCT(FNormalPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, InputVerticesTex)
        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, MLSVerticesTex)
        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, EdgeProximityTex)
        SHADER_PARAMETER_SAMPLER(SamplerState, PointSampler)
        SHADER_PARAMETER(float, p_h)
        SHADER_PARAMETER(float, kernelSpread)
        SHADER_PARAMETER(int32, kernelRadius)
        SHADER_PARAMETER(FVector2f, TexSize)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
};

class FQualityVS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FQualityVS);
    SHADER_USE_PARAMETER_STRUCT(FQualityVS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    END_SHADER_PARAMETER_STRUCT()
};

class FQualityPS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FQualityPS);
    SHADER_USE_PARAMETER_STRUCT(FQualityPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, VerticesTex)
        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, NormalsTex)
        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, EdgeDistTex)
        SHADER_PARAMETER_SAMPLER(SamplerState, PointSampler)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
};

class FSeparateMeshVS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FSeparateMeshVS);
    SHADER_USE_PARAMETER_STRUCT(FSeparateMeshVS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER(FMatrix44f, ProjectionMatrix)
        SHADER_PARAMETER(FMatrix44f, ViewMatrix)
        SHADER_PARAMETER(FMatrix44f, ModelMatrix)
        SHADER_PARAMETER(int32, Stride)
        SHADER_PARAMETER(int32, TexWidth)
        SHADER_PARAMETER(int32, TexHeight)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, TextureVertices)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, TextureEdgeProximity)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, TextureColor)
        SHADER_PARAMETER_SAMPLER(SamplerState, LinearSampler)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
};

class FSeparateMeshPS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FSeparateMeshPS);
    SHADER_USE_PARAMETER_STRUCT(FSeparateMeshPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_SAMPLER(SamplerState, LinearSampler)
    END_SHADER_PARAMETER_STRUCT()
};


class FMajorCamVS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FMajorCamVS);
    SHADER_USE_PARAMETER_STRUCT(FMajorCamVS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    END_SHADER_PARAMETER_STRUCT()
};

class FMajorCamPS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FMajorCamPS);
    SHADER_USE_PARAMETER_STRUCT(FMajorCamPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenUV0)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenUV1)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenUV2)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenUV3)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenUV4)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenUV5)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenUV6)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenUV7)

        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, TexturePosition0)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, TexturePosition1)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, TexturePosition2)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, TexturePosition3)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, TexturePosition4)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, TexturePosition5)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, TexturePosition6)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, TexturePosition7)

        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Quality0)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Quality1)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Quality2)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Quality3)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Quality4)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Quality5)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Quality6)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Quality7)

        SHADER_PARAMETER(float, CameraActive0)
        SHADER_PARAMETER(float, CameraActive1)
        SHADER_PARAMETER(float, CameraActive2)
        SHADER_PARAMETER(float, CameraActive3)
        SHADER_PARAMETER(float, CameraActive4)
        SHADER_PARAMETER(float, CameraActive5)
        SHADER_PARAMETER(float, CameraActive6)
        SHADER_PARAMETER(float, CameraActive7)

        SHADER_PARAMETER(FMatrix44f, ModelMatrix0)
        SHADER_PARAMETER(FMatrix44f, ModelMatrix1)
        SHADER_PARAMETER(FMatrix44f, ModelMatrix2)
        SHADER_PARAMETER(FMatrix44f, ModelMatrix3)
        SHADER_PARAMETER(FMatrix44f, ModelMatrix4)
        SHADER_PARAMETER(FMatrix44f, ModelMatrix5)
        SHADER_PARAMETER(FMatrix44f, ModelMatrix6)
        SHADER_PARAMETER(FMatrix44f, ModelMatrix7)

        SHADER_PARAMETER(FMatrix44f, ViewMatrix)

        SHADER_PARAMETER_SAMPLER(SamplerState, LinearSampler)

        RENDER_TARGET_BINDING_SLOTS()

    END_SHADER_PARAMETER_STRUCT()
};



class FCameraWeightsVS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FCameraWeightsVS);
    SHADER_USE_PARAMETER_STRUCT(FCameraWeightsVS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    END_SHADER_PARAMETER_STRUCT()
};

class FCameraWeightsPS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FCameraWeightsPS);
    SHADER_USE_PARAMETER_STRUCT(FCameraWeightsPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, MajorCamTex)
        SHADER_PARAMETER_SAMPLER(SamplerState, LinearSampler)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
};


class FScreenBlendingVS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FScreenBlendingVS);
    SHADER_USE_PARAMETER_STRUCT(FScreenBlendingVS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    END_SHADER_PARAMETER_STRUCT()
};

class FScreenBlendingPS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FScreenBlendingPS);
    SHADER_USE_PARAMETER_STRUCT(FScreenBlendingPS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        // Screen UV Textures (für Indirektion)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenUV0)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenUV1)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenUV2)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenUV3)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenUV4)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenUV5)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenUV6)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenUV7)

        // Color Textures
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Color0)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Color1)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Color2)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Color3)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Color4)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Color5)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Color6)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Color7)

        // Vertex/Position Textures
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Vertices0)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Vertices1)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Vertices2)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Vertices3)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Vertices4)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Vertices5)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Vertices6)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Vertices7)

        // Quality Textures (statt Normals)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Quality0)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Quality1)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Quality2)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Quality3)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Quality4)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Quality5)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Quality6)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Quality7)

        // Depth Textures
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenDepth0)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenDepth1)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenDepth2)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenDepth3)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenDepth4)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenDepth5)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenDepth6)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenDepth7)

        // Weight Textures
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, MiniWeightsA)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, MiniWeightsB)

        // Camera Active Flags
        SHADER_PARAMETER(float, CameraActive0)
        SHADER_PARAMETER(float, CameraActive1)
        SHADER_PARAMETER(float, CameraActive2)
        SHADER_PARAMETER(float, CameraActive3)
        SHADER_PARAMETER(float, CameraActive4)
        SHADER_PARAMETER(float, CameraActive5)
        SHADER_PARAMETER(float, CameraActive6)
        SHADER_PARAMETER(float, CameraActive7)

        SHADER_PARAMETER(FMatrix44f, ModelMatrix0)
        SHADER_PARAMETER(FMatrix44f, ModelMatrix1)
        SHADER_PARAMETER(FMatrix44f, ModelMatrix2)
        SHADER_PARAMETER(FMatrix44f, ModelMatrix3)
        SHADER_PARAMETER(FMatrix44f, ModelMatrix4)
        SHADER_PARAMETER(FMatrix44f, ModelMatrix5)
        SHADER_PARAMETER(FMatrix44f, ModelMatrix6)
        SHADER_PARAMETER(FMatrix44f, ModelMatrix7)

        SHADER_PARAMETER(FMatrix44f, ViewMatrix)

        // Parameter
        SHADER_PARAMETER(float, PreColorSaturation)
        SHADER_PARAMETER(FLinearColor, PreColorMultiplier)
        SHADER_PARAMETER(float, ColorHueShift)
        SHADER_PARAMETER(float, ColorContrast)
        SHADER_PARAMETER(float, ColorBrightness)
        SHADER_PARAMETER(float, ColorSaturation)
        SHADER_PARAMETER(float, ColorEdgeAlpha)
        SHADER_PARAMETER(float, ColorAlpha)

        // Sampler
        SHADER_PARAMETER_SAMPLER(SamplerState, LinearSampler)

        // Render Targets
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
};

class FCompositeWithDepthVS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FCompositeWithDepthVS);
    SHADER_USE_PARAMETER_STRUCT(FCompositeWithDepthVS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    END_SHADER_PARAMETER_STRUCT()
};

class FCompositeWithDepthPS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FCompositeWithDepthPS);
    SHADER_USE_PARAMETER_STRUCT(FCompositeWithDepthPS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SourceColor)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SourceDepth)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepth)
        SHADER_PARAMETER_SAMPLER(SamplerState, PointSampler)
        SHADER_PARAMETER_SAMPLER(SamplerState, LinearSampler)

        SHADER_PARAMETER(FVector4f, ViewRect)
        SHADER_PARAMETER(FVector2f, SceneTexSize)

        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
};