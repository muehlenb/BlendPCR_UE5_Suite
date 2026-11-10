#pragma once

#include "RGBDImage.h"

#include <memory>

#include "Registration/CameraRegistrationStatus.h"
#include "Registration/CameraRegistrator.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "InputDevice/Devices/AzureK4AConfig.h"
#include "InputDevice/InputDeviceManager.h"

#include "GPUTextureManager.h"

#include "BlendPCRFunctionLibrary.generated.h"

UCLASS()
class UBlendPCRFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UBlendPCRFunctionLibrary();
	
	// Manages Registration:
	static CameraRegistrator registrator;

	// Manages GPU Textures used by the view extension and updated by the devices:
	static GPUTextureManager gpuTextureManager;

	// Manages all native available devices (e.g. Azure Kinect):
	static UInputDeviceManager* deviceManager;

public:
	UFUNCTION(BlueprintCallable, Category = "BlendPCR|Registration")
	static TArray<FCameraRegistrationStatus> GetCameraRegistrationStatus();

	UFUNCTION(BlueprintCallable, Category="BlendPCR")
	static void StartCalibration();

	UFUNCTION(BlueprintCallable, Category="BlendPCR")
	static void FinishCalibration();

	UFUNCTION(BlueprintCallable, Category="BlendPCR")
	static void CancelCalibration();

	UFUNCTION(BlueprintCallable, Category="BlendPCR")
	static void ClearCalibration();

	UFUNCTION(BlueprintCallable, Category = "BlendPCR")
	static bool IsCalibrating();

	UFUNCTION(BlueprintCallable, Category = "BlendPCR")
	static bool SetCheckerboardSize(int innerWidth, int innerHeight);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BlendPCR")
	static int GetCheckerboardWidth();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BlendPCR")
	static int GetCheckerboardHeight();

	UFUNCTION(BlueprintCallable, Category="BlendPCR")
	static bool EnableRGBDProvider(const FString& type);

	UFUNCTION(BlueprintCallable, Category="BlendPCR")
	static bool DisableRGBDProvider(const FString& type);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "BlendPCR")
	static float GetRegistrationThreadFPS();

	UFUNCTION(BlueprintCallable, Category = "BlendPCR")
	static void CopyToClipboard(const FString& Text);

	static void UpdateRGBDImage(std::shared_ptr<RGBDImage> pc);

	static void TickFromOrigin(float DeltaTime);

	static GPUTextureManager& GetGPUTextureManager();

	UFUNCTION(BlueprintCallable, Category = "BlendPCR")
	static UInputDeviceManager* GetDeviceManager();
};