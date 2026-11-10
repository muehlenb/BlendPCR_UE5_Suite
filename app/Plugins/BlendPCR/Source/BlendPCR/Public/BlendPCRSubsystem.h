#pragma once

#include "CoreMinimal.h"
#include "SubSystems/EngineSubsystem.h"

#include "BlendPCROrigin.h"

#include "BlendPCRSubsystem.generated.h"

class FBlendPCRViewExtension;

UCLASS()
class UBlendPCRSubsystem : public UEngineSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    void RegisterBlendPCROrigin(ABlendPCROrigin* Actor);
    void UnregisterBlendPCROrigin(ABlendPCROrigin* Actor);

protected:
    TSharedPtr<FBlendPCRViewExtension, ESPMode::ThreadSafe> BlendPCRViewExtension;
};
