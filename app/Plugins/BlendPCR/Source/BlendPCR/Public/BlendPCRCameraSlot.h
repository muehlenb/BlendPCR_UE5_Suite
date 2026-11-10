#pragma once

#include "CoreMinimal.h"
#include "NiagaraDataInterface.h"
#include "Niagara/NiagaraDataInterfaceRHITexture.h"
#include "BlendPCRCameraSlot.generated.h"

UCLASS()
class BLENDPCR_API UBlendPCRCameraSlot : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY()
    UNiagaraDataInterfaceRHITexture* NiagaraDI;
};