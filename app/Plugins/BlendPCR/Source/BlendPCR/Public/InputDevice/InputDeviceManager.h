#pragma once

#include <vector>
#include <memory>
#include <functional>

#include <k4a/k4a.h>

#include "InputDevice.h"
#include "Devices/AzureK4ADevice.h"
#include "Devices/OrbbecDevice.h"

#include "InputDeviceManager.generated.h"

UCLASS()
class UInputDeviceManager : public UObject {
    GENERATED_BODY()

    std::function<void(std::shared_ptr<RGBDImage>)> imageCallback;

    /** Input devices */


    /** Synchronization of devices */
    void syncDevices() {
        uint32_t numK4A = k4a_device_get_installed_count();

        for (uint32_t i = 0; i < numK4A; i++) {
            UAzureK4ADevice* device = NewObject<UAzureK4ADevice>();
            device->AddToRoot();
            device->Initialize(i, [this](std::shared_ptr<RGBDImage> pc) { if (imageCallback) imageCallback(pc); });
            
            if (device->isValid()) {
                std::string serialNummer = device->getIdentifier();
                device->StartCamera();
                k4ADevices.Add(device);
            }
        }


        // Orbbec Femto
        uint32_t numOrbbec = UOrbbecDevice::GetInstalledCount();

        for (uint32_t i = 0; i < numOrbbec; i++)
        {
            UOrbbecDevice* device = NewObject<UOrbbecDevice>(this);
            device->AddToRoot();

            device->Initialize(i, [this](std::shared_ptr<RGBDImage> pc) { if (imageCallback) { imageCallback(pc); }});

            if (device->isValid())
            {
                std::string serialNumber = device->getIdentifier();
                device->StartCamera();
                orbbecDevices.Add(device);
            }
        }
    }

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BlendPCR")
    TArray<UAzureK4ADevice*> k4ADevices;


    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BlendPCR")
    TArray<UOrbbecDevice*> orbbecDevices;

    void SetCallback(std::function<void(std::shared_ptr<RGBDImage>)> cb) {
        imageCallback = cb;
        syncDevices();
    }
};