#include "NiagaraDataInterfaceRHITexture.h"
#include "NiagaraShaderParametersBuilder.h"
#include "NiagaraSystemInstance.h"
#include "ShaderParameterUtils.h"

#define LOCTEXT_NAMESPACE "NiagaraDataInterfaceRHITexture"

struct FNiagaraDataInterfaceProxyRHITexture : public FNiagaraDataInterfaceProxy
{
    FTexture2DRHIRef TextureRHI;

    virtual int32 PerInstanceDataPassedToRenderThreadSize() const override { return 0; }
};

static const FName SampleTextureName(TEXT("SampleTexture"));
static const FName GetTextureDimensionsName(TEXT("GetTextureDimensions"));

UNiagaraDataInterfaceRHITexture::UNiagaraDataInterfaceRHITexture()
{
    Proxy.Reset(new FNiagaraDataInterfaceProxyRHITexture());
}

void UNiagaraDataInterfaceRHITexture::SetTexture(FTexture2DRHIRef InTexture)
{
    FScopeLock Lock(&TextureLock);
    TextureRHI = InTexture;
    
    // Proxy updaten für GPU-Zugriff
    if (FNiagaraDataInterfaceProxyRHITexture* RTProxy = 
        static_cast<FNiagaraDataInterfaceProxyRHITexture*>(Proxy.Get()))
    {
        ENQUEUE_RENDER_COMMAND(UpdateNiagaraRHITexture)(
            [RTProxy, InTexture](FRHICommandListImmediate& RHICmdList)
            {
                RTProxy->TextureRHI = InTexture;
            });
    }
}

void UNiagaraDataInterfaceRHITexture::GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)
{
    // SampleTexture(UV) -> float4
    {
        FNiagaraFunctionSignature Sig;
        Sig.Name = SampleTextureName;
        Sig.bMemberFunction = true;
        Sig.bRequiresContext = false;
        Sig.bSupportsCPU = false;
        Sig.bSupportsGPU = true;
        Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec2Def(), TEXT("UV")));
        Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec4Def(), TEXT("Value")));
        OutFunctions.Add(Sig);
    }

    // GetTextureDimensions() -> int2
    {
        FNiagaraFunctionSignature Sig;
        Sig.Name = GetTextureDimensionsName;
        Sig.bMemberFunction = true;
        Sig.bRequiresContext = false;
        Sig.bSupportsCPU = false;
        Sig.bSupportsGPU = true;
        Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Width")));
        Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Height")));
        OutFunctions.Add(Sig);
    }
}

void UNiagaraDataInterfaceRHITexture::GetVMExternalFunction(
    const FVMExternalFunctionBindingInfo& BindingInfo,
    void* InstanceData,
    FVMExternalFunction& OutFunc)
{
    // GPU-only, keine CPU-Implementierung nötig
}

bool UNiagaraDataInterfaceRHITexture::Equals(const UNiagaraDataInterface* Other) const
{
    if (Other == this)
    {
        return true;
    }

    const UNiagaraDataInterfaceRHITexture* OtherTyped = Cast<UNiagaraDataInterfaceRHITexture>(Other);
    if (!OtherTyped)
    {
        return false;
    }

    // Deadlock-frei: erst jeweils unter Lock lokale Kopien ziehen, dann vergleichen
    FTexture2DRHIRef ThisTex;
    {
        FScopeLock Lock(&TextureLock);
        ThisTex = TextureRHI;
    }

    FTexture2DRHIRef OtherTex;
    {
        FScopeLock Lock(&OtherTyped->TextureLock);
        OtherTex = OtherTyped->TextureRHI;
    }

    // Vergleich der RHI-Refs
    return ThisTex == OtherTex;
}

bool UNiagaraDataInterfaceRHITexture::CopyToInternal(UNiagaraDataInterface* Destination) const
{
    UNiagaraDataInterfaceRHITexture* Dest = Cast<UNiagaraDataInterfaceRHITexture>(Destination);
    if (!Dest)
    {
        return false;
    }

    FTexture2DRHIRef CopyTex;
    {
        FScopeLock Lock(&TextureLock);
        CopyTex = TextureRHI;
    }

    // Wichtig: SetTexture kümmert sich darum, GameThread state + RenderThread Proxy zu syncen
    Dest->SetTexture(CopyTex);
    return true;
}

#if WITH_EDITORONLY_DATA
void UNiagaraDataInterfaceRHITexture::GetParameterDefinitionHLSL(
    const FNiagaraDataInterfaceGPUParamInfo& ParamInfo,
    FString& OutHLSL)
{
    OutHLSL += FString::Printf(TEXT("Texture2D %s_Texture;\n"), *ParamInfo.DataInterfaceHLSLSymbol);
    OutHLSL += FString::Printf(TEXT("SamplerState %s_Sampler;\n"), *ParamInfo.DataInterfaceHLSLSymbol);
    OutHLSL += FString::Printf(TEXT("int2 %s_Dimensions;\n"), *ParamInfo.DataInterfaceHLSLSymbol);
}

bool UNiagaraDataInterfaceRHITexture::GetFunctionHLSL(
    const FNiagaraDataInterfaceGPUParamInfo& ParamInfo,
    const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo,
    int FunctionInstanceIndex,
    FString& OutHLSL)
{
    if (FunctionInfo.DefinitionName == SampleTextureName)
    {
        OutHLSL += FString::Printf(TEXT(R"(
void %s(float2 UV, out float4 Value)
{
    Value = %s_Texture.SampleLevel(%s_Sampler, UV, 0);
}
)"),
            *FunctionInfo.InstanceName,
            *ParamInfo.DataInterfaceHLSLSymbol,
            *ParamInfo.DataInterfaceHLSLSymbol);
        return true;
    }
    
    if (FunctionInfo.DefinitionName == GetTextureDimensionsName)
    {
        OutHLSL += FString::Printf(TEXT(R"(
void %s(out int Width, out int Height)
{
    Width = %s_Dimensions.x;
    Height = %s_Dimensions.y;
}
)"),
            *FunctionInfo.InstanceName,
            *ParamInfo.DataInterfaceHLSLSymbol,
            *ParamInfo.DataInterfaceHLSLSymbol);
        return true;
    }
    
    return false;
}
#endif


#undef LOCTEXT_NAMESPACE