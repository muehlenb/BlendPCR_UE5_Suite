#pragma once

#include "CoreMinimal.h"
#include "OrbbecConfig.generated.h"

UENUM(BlueprintType)
enum class EOrbbecFramerate : uint8
{
    FPS5  UMETA(DisplayName = "5 FPS"),
    FPS15 UMETA(DisplayName = "15 FPS"),
    FPS30 UMETA(DisplayName = "30 FPS"),
};

USTRUCT(BlueprintType)
struct FOrbbecConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbbec|Camera")
    EOrbbecFramerate FPS = EOrbbecFramerate::FPS30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbbec|Camera")
    FMatrix44f modelMatrix;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbbec|Depth")
    bool bEnableFrameSync = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbbec|Color")
    bool AutoExposure = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbbec|Color", meta = (EditCondition = "!AutoExposure"))
    int32 Exposure = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbbec|Color")
    bool AutoWhiteBalance = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbbec|Color", meta = (EditCondition = "!AutoWhiteBalance"))
    int32 WhiteBalance = 4500;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbbec|Color")
    int32 Gain = 64;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbbec|Color")
    int32 Brightness = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbbec|Color")
    int32 Contrast = 32;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbbec|Color")
    int32 Saturation = 64;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbbec|Color")
    int32 Sharpness = 50;
};