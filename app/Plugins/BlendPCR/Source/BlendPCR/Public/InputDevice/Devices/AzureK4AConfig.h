#pragma once

#include "CoreMinimal.h"
#include "AzureK4AConfig.generated.h"

UENUM(BlueprintType)
enum class EAzureK4ASyncMode : uint8
{
    Standalone UMETA(DisplayName = "Standalone"),
    Master     UMETA(DisplayName = "Master"),
    Subordinate UMETA(DisplayName = "Subordinate")
};

UENUM(BlueprintType)
enum class EAzureK4AColorResolution : uint8
{
    Color720P UMETA(DisplayName = "Color720P")
};

UENUM(BlueprintType)
enum class EAzureK4ADepthResolution : uint8
{
    //Depth288_NFoV UMETA(DisplayName = "320 x 288 (NFoV)"),
    Depth576_NFoV UMETA(DisplayName = "640 x 576 (NFoV)"),
    //Depth512_WFoV UMETA(DisplayName = "512 x 512 (WFoV)"),
    //Depth1024_WFoV UMETA(DisplayName = "1024 x 1024 (WFoV)"),
};

UENUM(BlueprintType)
enum class EAzureK4AFramerate : uint8
{
    FPS5 UMETA(DisplayName = "5 FPS"),
    FPS15 UMETA(DisplayName = "15 FPS"),
    FPS30 UMETA(DisplayName = "30 FPS"),
};

USTRUCT(BlueprintType)
struct FAzureK4AConfig
{
    GENERATED_BODY()

    /** Camera Modes larger than the minimum is not usefull. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AzureKinect|Camera", meta = (EditCondition = "false"))
    EAzureK4AColorResolution ColorResolution = EAzureK4AColorResolution::Color720P;

    /** Currently, only a fixed resolution is allowed that may not change */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AzureKinect|Camera", meta = (EditCondition = "false"))
    EAzureK4ADepthResolution DepthResolution = EAzureK4ADepthResolution::Depth576_NFoV;

    /** Framerate of the camera */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AzureKinect|Camera")
    EAzureK4AFramerate FPS = EAzureK4AFramerate::FPS30;

    /** Current transform of the camera */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AzureKinect|Camera")
    FMatrix44f modelMatrix;

    /** If you using the sync cable, set one camera to Master and all the others to Subordinate */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AzureKinect|Sync")
    EAzureK4ASyncMode SyncMode = EAzureK4ASyncMode::Standalone;

    /** Delay in microseconds for subordinate cameras (to avoid interference) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AzureKinect|Sync", meta = (EditCondition = "SyncMode==EAzureK4ASyncMode::Subordinate"))
    int32 SubordinateDelayUS = 0;

    /** Delay in microseconds between RGB and depth sensor */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AzureKinect|Depth")
    int32 DepthDelayOffColorUS = 0;

    /** Synchronized images only between RGB and depth sensor */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AzureKinect|Depth")
    bool bSynchronizedImagesOnly = true;

    /** Automatic white balance */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AzureKinect|Color")
    bool AutoWhiteBalance = true;

    /** Manual white balance value*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AzureKinect|Color", meta = (EditCondition = "AutoWhiteBalance==false"))
    int32 WhiteBalance = 5200;

    /** Automatic exposure */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AzureKinect|Color")
    bool AutoExposure = true;

    /** Manual exposure value in microseconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AzureKinect|Color", meta = (EditCondition = "AutoExposure==false"))
    int32 ExposureUS = 24000;

    /** Brightness gain */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AzureKinect|Color")
    int32 Gain = 240;

    /** Brightness value */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AzureKinect|Color")
    int32 Brightness = 128;

};