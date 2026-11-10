#pragma once

#include <k4a/k4a.h>
#include <thread>
#include <atomic>
#include <functional>

#include "CoreMinimal.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "JsonObjectConverter.h"

#include "../InputDevice.h"
#include "../../RGBDImage.h"
#include "AzureK4AConfig.h"

#include "AzureK4ADevice.generated.h"


UCLASS(Blueprintable)
class UAzureK4ADevice : public UObject, public InputDevice {
    GENERATED_BODY()
public:
    void Initialize(uint32_t deviceIndex, std::function<void(std::shared_ptr<RGBDImage>)> imageCallback);
    virtual ~UAzureK4ADevice();

    virtual bool isValid() override { return device != nullptr; }
    virtual std::string getIdentifier() override { return deviceId; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Azure Kinect")
    FString GetName() { return FString(deviceId.c_str()); }

    /** Runtime control */
    bool StartCamera();
    void StopCamera();

    UFUNCTION(BlueprintCallable, Category = "Azure Kinect")
    bool ApplyConfig();

    /** Config */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Azure Kinect Config")
    FAzureK4AConfig config;
private:
    /** Device */
    uint32_t index = 0;
    std::string deviceId;
    k4a_device_t device = nullptr;

    /** Azure */
    k4a_calibration_t calibration{};
    k4a_transformation_t transformation = nullptr;

    /** Thread */
    std::thread worker;
    std::atomic<bool> running{ false };

    float* lookupTable = nullptr;

    std::function<void(std::shared_ptr<RGBDImage>)> imageCallback;

private:
    void create_xy_table();

    /** Lifecycle */
    bool OpenDevice();
    void CloseDevice();

    bool ApplyConfigAndStart();

    std::shared_ptr<RGBDImage> grabFrame();
    void CaptureLoop();

    /** Config IO */
    bool LoadConfig();
    bool SaveConfig() const;
    FString GetConfigPath() const;
};