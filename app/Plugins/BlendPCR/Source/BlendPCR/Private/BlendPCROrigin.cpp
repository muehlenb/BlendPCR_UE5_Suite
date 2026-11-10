#include "BlendPCROrigin.h"

#include "Niagara/Public/NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "BlendPCRFunctionLibrary.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/BillboardComponent.h"

#include "BlendPCRSubSystem.h"

ABlendPCROrigin::ABlendPCROrigin()
{
    // Ursprungs-Komponente erstellen und als Root setzen
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(
        TEXT("/BlendPCR/Resources/BlendPCROriginIcon.BlendPCROriginIcon")
    );

    SpriteComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
    SpriteComponent->SetSprite(IconTexture.Object);
    SpriteComponent->bIsEditorOnly = true;
    SpriteComponent->SetupAttachment(RootComponent);
    SpriteComponent->SetRelativeScale3D(FVector(0.075f));

    // Box-Komponente erstellen (zur Definition, nicht rendernd)
    ClippingBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ClippingBox"));
    ClippingBox->SetupAttachment(RootComponent);
    ClippingBox->SetBoxExtent(FVector(50, 50, 50));

    // Box soll keine Kollision oder Rendering haben
    ClippingBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ClippingBox->SetHiddenInGame(true);
    ClippingBox->SetGenerateOverlapEvents(false);

    // Standardwerte
    bClippingActive = false;

    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
}


void ABlendPCROrigin::BeginPlay()
{
    Super::BeginPlay();

    // Beispiel: Ausgabe im Log, falls Clipping aktiv ist
    if (bClippingActive)
    {
        UE_LOG(LogTemp, Log, TEXT("Clipping is active for %s"), *GetName());
    }
}

void ABlendPCROrigin::UpdateNiagaraSystem(FString cameraId, UTextureRenderTarget2D* ColorNiagaraRenderTarget, UTextureRenderTarget2D* PointCloudNiagaraRenderTarget, int Width, int Height) {
    PendingUpdates.Enqueue({ cameraId,ColorNiagaraRenderTarget,PointCloudNiagaraRenderTarget, Width, Height });
}


void ABlendPCROrigin::ApplyNiagaraUpdate_GameThread(const FPendingNiagaraUpdate& Update){
    if (!NiagaraSystems.Contains(Update.CameraId)){
        UNiagaraComponent* NiagaraComp =
            UNiagaraFunctionLibrary::SpawnSystemAttached(
                NiagaraSystemClass,
                RootComponent,
                NAME_None,
                FVector::ZeroVector,
                FRotator::ZeroRotator,
                EAttachLocation::KeepRelativeOffset,
                false
            );

        if (NiagaraComp)
        {
            NiagaraComp->SetAutoActivate(false);
            NiagaraComp->SetFlags(RF_Transient);
            NiagaraComp->DeactivateImmediate();
        }

        if (NiagaraComp)
        {
            SpawnedNiagaraComponents.Add(NiagaraComp);
            NiagaraSystems.Add(Update.CameraId, NiagaraComp);
        }
        UE_LOG(LogTemp, Warning, TEXT("CREATE NIAGARA SYSTEM for Camera %s"), *Update.CameraId);
    }
    else 
    {
        if (NiagaraSystems.Contains(Update.CameraId)) {
            UNiagaraComponent* NiagaraComp = NiagaraSystems[Update.CameraId];
            NiagaraComp->SetVariableTexture(TEXT("User.ColorTexture"), Cast<UTexture>(Update.ColorRT));
            NiagaraComp->SetVariableTexture(TEXT("User.PositionTexture"), Cast<UTexture>(Update.PositionRT));
            NiagaraComp->SetVariableInt(TEXT("User.PointNum"), Update.Width * Update.Height);
            NiagaraComp->SetVariableInt(TEXT("User.Width"), Update.Width);
            NiagaraComp->SetVariableInt(TEXT("User.Height"), Update.Height);

            FTransform tf = FTransform(FMatrix(FMatrix(CameraRegistrator::getMatrix(TCHAR_TO_UTF8(*Update.CameraId)))).GetTransposed());
            NiagaraComp->SetRelativeTransform(tf);

            if (!NiagaraComp->IsActive())
            {
                NiagaraComp->ReinitializeSystem();
                NiagaraComp->Activate(true);
            }
            NiagaraComp->SetVariableMatrix(TEXT("User.ClippingTransform"), (ClippingBox->GetRelativeTransform().Inverse() * NiagaraComp->GetRelativeTransform()).ToMatrixWithScale());
            NiagaraComp->SetVariableBool(TEXT("User.IsClippingActive"), bClippingActive);
        }
    }
}

void ABlendPCROrigin::PostRegisterAllComponents()
{
    Super::PostRegisterAllComponents();

    if (GEngine)
    {
        if (UBlendPCRSubsystem* Subsystem = GEngine->GetEngineSubsystem<UBlendPCRSubsystem>())
        {
            Subsystem->RegisterBlendPCROrigin(this);
        }
    }

    CheckForErrors();
}

void ABlendPCROrigin::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GEngine)
    {
        if (UBlendPCRSubsystem* Subsystem = GEngine->GetEngineSubsystem<UBlendPCRSubsystem>())
        {
            Subsystem->UnregisterBlendPCROrigin(this);
        }
    }

    Super::EndPlay(EndPlayReason);
    CheckForErrors();
}

void ABlendPCROrigin::Tick(float DeltaTime){
    Super::Tick(DeltaTime);

    FPendingNiagaraUpdate Update;
    while (PendingUpdates.Dequeue(Update))
    {
        ApplyNiagaraUpdate_GameThread(Update);
    }

    UBlendPCRFunctionLibrary::TickFromOrigin(DeltaTime);
}

#if WITH_EDITOR
void ABlendPCROrigin::PostEditMove(bool bFinished)
{
    Super::PostEditMove(bFinished);

    if (bFinished && GEngine)
    {
        if (UBlendPCRSubsystem* Subsystem = GEngine->GetEngineSubsystem<UBlendPCRSubsystem>())
        {
            Subsystem->RegisterBlendPCROrigin(this);
        }
    }
    CheckForErrors();
}

bool ABlendPCROrigin::ShouldTickIfViewportsOnly() const
{
    return true;
}

void ABlendPCROrigin::PostEditUndo()
{
    Super::PostEditUndo();

    if (GEngine)
    {
        if (UBlendPCRSubsystem* Subsystem = GEngine->GetEngineSubsystem<UBlendPCRSubsystem>())
        {
            Subsystem->RegisterBlendPCROrigin(this);
        }
    }
    CheckForErrors();
}

void ABlendPCROrigin::Destroyed()
{
    if (GEngine)
    {
        if (UBlendPCRSubsystem* Subsystem = GEngine->GetEngineSubsystem<UBlendPCRSubsystem>())
        {
            Subsystem->UnregisterBlendPCROrigin(this);
        }
    }

    Super::Destroyed();
    CheckForErrors();
}
#endif
