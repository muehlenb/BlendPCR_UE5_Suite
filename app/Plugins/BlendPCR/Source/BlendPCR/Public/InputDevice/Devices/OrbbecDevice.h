#pragma once

#include <thread>
#include <atomic>
#include <functional>
#include <memory>
#include <string>

#include "CoreMinimal.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "JsonObjectConverter.h"

#include "libobsensor/ObSensor.hpp"

#include "../InputDevice.h"
#include "../../RGBDImage.h"
#include "OrbbecConfig.h"

#include "OrbbecDevice.generated.h"

UCLASS(Blueprintable)
class UOrbbecDevice : public UObject, public InputDevice
{
    GENERATED_BODY()

public:
    static uint32_t GetInstalledCount();

    void Initialize(uint32_t deviceIndex, std::function<void(std::shared_ptr<RGBDImage>)> imageCallback);
    virtual ~UOrbbecDevice();

    virtual bool isValid() override { return device != nullptr; }
    virtual std::string getIdentifier() override { return deviceId; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Orbbec")
    FString GetName() { return FString(deviceId.c_str()); }

    bool StartCamera();
    void StopCamera();

    UFUNCTION(BlueprintCallable, Category = "Orbbec")
    bool ApplyConfig();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbbec Config")
    FOrbbecConfig config;

private:
    uint32_t index = 0;
    std::string deviceId;

    std::shared_ptr<ob::Context> context;
    std::shared_ptr<ob::Device> device;
    std::shared_ptr<ob::Pipeline> pipeline;

    std::thread worker;
    std::atomic<bool> running{ false };

    float* lookupTable = nullptr;

    int depthWidth = 0;
    int depthHeight = 0;

    std::function<void(std::shared_ptr<RGBDImage>)> imageCallback;

    std::shared_ptr<ob::PointCloudFilter> pointCloudFilter;
    std::shared_ptr<ob::Align> aligner;

    float* lookup3DTo2D = nullptr;

    void create_xy_table(std::shared_ptr<ob::FrameSet> frameSet);
    void generate3DTo2D(int width, int height);

    bool OpenDevice();
    void CloseDevice();

    bool ApplyConfigAndStart();

    std::shared_ptr<RGBDImage> grabFrame();
    void CaptureLoop();

    void create_xy_table();

    bool LoadConfig();
    bool SaveConfig() const;
    FString GetConfigPath() const;
};