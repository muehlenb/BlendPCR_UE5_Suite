#pragma once

#include "NiagaraDataInterface.h"
#include "RenderGraphResources.h"
#include "NiagaraDataInterfaceRHITexture.generated.h"

UCLASS(EditInlineNew, Category = "Texture", meta = (DisplayName = "RHI Texture 2D"))
class UNiagaraDataInterfaceRHITexture : public UNiagaraDataInterface
{
    GENERATED_BODY()

public:
    UNiagaraDataInterfaceRHITexture();

    // Texture setzen (Thread-safe)
    void SetTexture(FTexture2DRHIRef InTexture);

    // UNiagaraDataInterface Interface
    virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions) override;
    virtual void GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, 
                                        void* InstanceData, 
                                        FVMExternalFunction& OutFunc) override;
    
#if WITH_EDITORONLY_DATA
    virtual bool GetFunctionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo,
                                  const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo,
                                  int FunctionInstanceIndex,
                                  FString& OutHLSL) override;
    virtual void GetParameterDefinitionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo,
                                             FString& OutHLSL) override;
#endif

    virtual bool CanExecuteOnTarget(ENiagaraSimTarget Target) const override 
    { 
        return Target == ENiagaraSimTarget::GPUComputeSim; 
    }

    virtual bool Equals(const UNiagaraDataInterface* Other) const override;
    virtual bool CopyToInternal(UNiagaraDataInterface* Destination) const override;


private:
    FTexture2DRHIRef TextureRHI;
    mutable FCriticalSection TextureLock;
};