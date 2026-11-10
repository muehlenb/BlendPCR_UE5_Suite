#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"

#include "InputDevice/InputDeviceManager.h"

#include "Niagara/NiagaraDataInterfaceRHITexture.h"

#include "BlendPCROrigin.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

struct FPendingNiagaraUpdate
{
    FString CameraId;
    UTextureRenderTarget2D* ColorRT;
    UTextureRenderTarget2D* PositionRT;
    int32 Width;
    int32 Height;
};

UCLASS()
class BLENDPCR_API ABlendPCROrigin : public AActor
{
    GENERATED_BODY()

    TMap<FString,UNiagaraComponent*> NiagaraSystems;
    TQueue<FPendingNiagaraUpdate, EQueueMode::Mpsc> PendingUpdates;

public:
    // Konstruktor
    ABlendPCROrigin();

    void UpdateNiagaraSystem(FString cameraId, UTextureRenderTarget2D* ColorNiagaraRenderTarget, UTextureRenderTarget2D* PointCloudNiagaraRenderTarget, int Width, int Height);

    void ApplyNiagaraUpdate_GameThread(const FPendingNiagaraUpdate& Update);

protected:
    // Wird beim Start aufgerufen
    virtual void BeginPlay() override;

    // Wird im Editor und im Spiel aufgerufen, sobald der Actor "aktiv" ist
    virtual void PostRegisterAllComponents() override;

    // Wird beim Entfernen aus der Welt (oder Level-Unload) aufgerufen
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual void Tick(float DeltaTime) override;



#if WITH_EDITOR
    // Editor-spezifisch: Undo, Move, Delete, etc.
    virtual void PostEditMove(bool bFinished) override;
    virtual void PostEditUndo() override;
    virtual void Destroyed() override;
    virtual bool ShouldTickIfViewportsOnly() const override;
#endif

public:
    // Ursprung der Actor-Hierarchie
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transformation")
    USceneComponent* Root;

    UBillboardComponent* SpriteComponent;

    // Box zur Definition des Clipping-Bereichs (nicht rendernd)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Clipping")
    UBoxComponent* ClippingBox;

    // Aktiviert/Deaktiviert Clipping
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlendPCR")
    bool bClippingActive;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlendPCR", meta = (ClampMin = "0", UIMin = "0", UIMax = "5", Delta = "0.01"))
    float PreSaturation = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlendPCR", meta = (HideAlphaChannel, ClampMin = "0.0", Delta = "0.01"))
    FLinearColor PreMultiplier = FLinearColor(1.0, 1.0, 1.0, 1.0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlendPCR", meta = (ClampMin = "-180.0", ClampMax = "180.0", UIMin = "-180.0", UIMax = "180.0", Units = "deg", Delta = "1"))
    float HueShift = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlendPCR", meta = (ClampMin = "0", UIMin = "0", UIMax = "5", Delta = "0.01"))
    float Contrast = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlendPCR", meta = (ClampMin = "0", UIMin = "0", UIMax = "5", Delta = "0.01"))
    float Brightness = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlendPCR", meta = (ClampMin = "0", UIMin = "0", UIMax = "5", Delta = "0.01"))
    float Saturation = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlendPCR", meta = (ClampMin = "0", ClampMax = "1.0", UIMin = "0", UIMax = "1", Delta = "0.01"))
    float EdgeAlpha = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlendPCR", meta = (ClampMin = "0", ClampMax = "1.0", UIMin = "0", UIMax = "1", Delta = "0.01"))
    float Alpha = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BlendPCR|Niagara")
    UNiagaraSystem* NiagaraSystemClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BlendPCR|Niagara")
    TArray<UNiagaraComponent*> SpawnedNiagaraComponents;
};