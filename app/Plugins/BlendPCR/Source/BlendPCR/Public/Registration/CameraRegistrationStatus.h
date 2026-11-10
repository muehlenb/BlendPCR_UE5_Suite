#pragma once

#include "CoreMinimal.h"
#include "CameraRegistrationStatus.generated.h"

USTRUCT(BlueprintType)
struct FCameraRegistrationStatus
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString CameraId;

    UPROPERTY(BlueprintReadOnly)
    float AverageError = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bCheckerboardDetected = false;

    UPROPERTY(BlueprintReadOnly)
    int32 NumSamples = 0;

    UPROPERTY(BlueprintReadOnly)
    FMatrix44f Transform = FMatrix44f::Identity;
};
