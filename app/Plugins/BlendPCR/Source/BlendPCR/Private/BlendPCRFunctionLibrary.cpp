#include "BlendPCRFunctionLibrary.h"
#include "Misc/Paths.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/PlatformFilemanager.h"

CameraRegistrator UBlendPCRFunctionLibrary::registrator;
GPUTextureManager UBlendPCRFunctionLibrary::gpuTextureManager;
UInputDeviceManager* UBlendPCRFunctionLibrary::deviceManager = nullptr;

UBlendPCRFunctionLibrary::UBlendPCRFunctionLibrary() {
	UBlendPCRFunctionLibrary::GetDeviceManager()->SetCallback([this](std::shared_ptr<RGBDImage> pc) {
		UBlendPCRFunctionLibrary::UpdateRGBDImage(pc);
	});
}

void UBlendPCRFunctionLibrary::StartCalibration(){
	registrator.start();
}

void UBlendPCRFunctionLibrary::FinishCalibration(){
	registrator.stop();
}

void UBlendPCRFunctionLibrary::CancelCalibration(){

}

bool UBlendPCRFunctionLibrary::SetCheckerboardSize(int innerWidth, int innerHeight) {
	return registrator.setCheckerboardSize(innerWidth, innerHeight);
}

int UBlendPCRFunctionLibrary::GetCheckerboardWidth() {
	return registrator.getCheckerboardWidth();
}

int UBlendPCRFunctionLibrary::GetCheckerboardHeight() {
	return registrator.getCheckerboardHeight();
}

void UBlendPCRFunctionLibrary::ClearCalibration(){
	registrator.clearCalibration();
}

bool UBlendPCRFunctionLibrary::IsCalibrating(){
	return registrator.isCurrentlyRunning();
}

bool UBlendPCRFunctionLibrary::EnableRGBDProvider(const FString& type){
	return true;
}

bool UBlendPCRFunctionLibrary::DisableRGBDProvider(const FString& type){
	return true;
}

void UBlendPCRFunctionLibrary::UpdateRGBDImage(std::shared_ptr<RGBDImage> rbgdimage) {
	registrator.updateRGBDImage(rbgdimage);
	gpuTextureManager.updatePointCloud(rbgdimage);
}

TArray<FCameraRegistrationStatus> UBlendPCRFunctionLibrary::GetCameraRegistrationStatus(){
    return registrator.GetCameraRegistrationStatuses();
}

float UBlendPCRFunctionLibrary::GetRegistrationThreadFPS()
{
	return registrator.GetThreadFPS();
}

void UBlendPCRFunctionLibrary::CopyToClipboard(const FString& Text)
{
	FPlatformApplicationMisc::ClipboardCopy(*Text);
}

void UBlendPCRFunctionLibrary::TickFromOrigin(float DeltaTime) {
} 


GPUTextureManager& UBlendPCRFunctionLibrary::GetGPUTextureManager() {
	return gpuTextureManager;
}

UInputDeviceManager* UBlendPCRFunctionLibrary::GetDeviceManager() {
	if (!deviceManager)
	{
		deviceManager = NewObject<UInputDeviceManager>();
		deviceManager->AddToRoot();
	}
	return deviceManager;
}