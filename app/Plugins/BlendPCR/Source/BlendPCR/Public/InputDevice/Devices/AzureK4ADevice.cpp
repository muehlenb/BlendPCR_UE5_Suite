#include "AzureK4ADevice.h"

#include "../../Registration/CameraRegistrator.h"


void UAzureK4ADevice::Initialize(uint32_t deviceIndex, std::function<void(std::shared_ptr<RGBDImage>)> pImageCallback)
{
    index = deviceIndex;
    imageCallback = pImageCallback;

    OpenDevice();
    if (!LoadConfig())
        SaveConfig();
}

UAzureK4ADevice::~UAzureK4ADevice()
{
    StopCamera();
    CloseDevice();
    delete[] lookupTable;
}

bool UAzureK4ADevice::OpenDevice()
{
    if (device)
        return true;

    if (k4a_device_open(index, &device) != K4A_RESULT_SUCCEEDED)
    {
        UE_LOG(LogTemp, Error, TEXT("AzureK4ADevice: Failed to open device %d"), index);
        device = nullptr;
        return false;
    }

    char serial[256];
    size_t size = sizeof(serial);
    if (k4a_device_get_serialnum(device, serial, &size) == K4A_BUFFER_RESULT_SUCCEEDED)
        deviceId = "Azure_" + std::string(serial);
    else
        deviceId = "AzureNoSerial_" + std::to_string(index);

    return true;
}

void UAzureK4ADevice::CloseDevice()
{
    if (!device)
        return;

    k4a_device_close(device);
    device = nullptr;
}

bool UAzureK4ADevice::StartCamera()
{
    if (!device || running)
        return false;

    if (!ApplyConfigAndStart())
        return false;

    running = true;
    worker = std::thread(&UAzureK4ADevice::CaptureLoop, this);

    UE_LOG(LogTemp, Display, TEXT("AzureK4ADevice: Camera started (%s)"), *FString(deviceId.c_str()));
    return true;
}

void UAzureK4ADevice::StopCamera()
{
    if (!running)
        return;

    running = false;
    if (worker.joinable())
        worker.join();

    if (transformation)
    {
        k4a_transformation_destroy(transformation);
        transformation = nullptr;
    }

    if (device)
        k4a_device_stop_cameras(device);

    UE_LOG(LogTemp, Display, TEXT("AzureK4ADevice: Camera stopped (%s)"), *FString(deviceId.c_str()));
}

bool UAzureK4ADevice::ApplyConfig()
{
    StopCamera();
    SaveConfig();

    return StartCamera();
}

FString UAzureK4ADevice::GetConfigPath() const
{
    return FPaths::ProjectSavedDir() / TEXT("CameraConfig") / FString::Printf(TEXT("%s.json"), *FString(deviceId.c_str()));
}

bool UAzureK4ADevice::LoadConfig()
{
    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *GetConfigPath()))
        return false;

    return FJsonObjectConverter::JsonObjectStringToUStruct(Json, &config, 0, 0);
}

bool UAzureK4ADevice::SaveConfig() const
{
    FString Json;
    if (!FJsonObjectConverter::UStructToJsonObjectString(FAzureK4AConfig::StaticStruct(), &config, Json, 0, 0))
        return false;

    IFileManager::Get().MakeDirectory(*FPaths::GetPath(GetConfigPath()), true);
    return FFileHelper::SaveStringToFile(Json, *GetConfigPath());
}


void UAzureK4ADevice::create_xy_table() {
    if (lookupTable != nullptr)
        delete[] lookupTable;

    int width = calibration.depth_camera_calibration.resolution_width;
    int height = calibration.depth_camera_calibration.resolution_height;

    UE_LOG(LogTemp, Warning, TEXT("LOOKUP TABLE: %i x %i"), width, height)

    lookupTable = new float[width * height * 2];

    k4a_float2_t p;
    k4a_float3_t ray;
    int valid;

    for (int y = 0, idx = 0; y < height; y++)
    {
        p.xy.y = (float)y;
        for (int x = 0; x < width; x++, idx++)
        {
            p.xy.x = (float)x;

            k4a_calibration_2d_to_3d(&calibration, &p, 1.f, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_DEPTH, &ray, &valid);

            if (valid) {
                lookupTable[idx * 2] = ray.xyz.x;
                lookupTable[idx * 2 + 1] = ray.xyz.y;
            }
            else {
                lookupTable[idx * 2] = nanf("");
                lookupTable[idx * 2 + 1] = nanf("");
            }
        }
    }
}

bool UAzureK4ADevice::ApplyConfigAndStart()
{
    k4a_device_configuration_t cfg = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;

    cfg.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32; // : K4A_IMAGE_FORMAT_COLOR_MJPG;

    cfg.color_resolution = K4A_COLOR_RESOLUTION_720P;

    cfg.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;

    cfg.camera_fps =
        config.FPS == EAzureK4AFramerate::FPS5 ? K4A_FRAMES_PER_SECOND_5 :
        config.FPS == EAzureK4AFramerate::FPS15 ? K4A_FRAMES_PER_SECOND_15 :
        K4A_FRAMES_PER_SECOND_30;

    cfg.synchronized_images_only = config.bSynchronizedImagesOnly;
    cfg.depth_delay_off_color_usec = config.DepthDelayOffColorUS;

    cfg.wired_sync_mode =
        config.SyncMode == EAzureK4ASyncMode::Master ? K4A_WIRED_SYNC_MODE_MASTER :
        config.SyncMode == EAzureK4ASyncMode::Subordinate ? K4A_WIRED_SYNC_MODE_SUBORDINATE :
        K4A_WIRED_SYNC_MODE_STANDALONE;

    cfg.subordinate_delay_off_master_usec = config.SubordinateDelayUS;

    if (k4a_device_start_cameras(device, &cfg) != K4A_RESULT_SUCCEEDED)
        return false;

    k4a_device_set_color_control(device, K4A_COLOR_CONTROL_WHITEBALANCE,
        config.AutoWhiteBalance ? K4A_COLOR_CONTROL_MODE_AUTO : K4A_COLOR_CONTROL_MODE_MANUAL,
        config.WhiteBalance);

    k4a_device_set_color_control(device, K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
        config.AutoExposure ? K4A_COLOR_CONTROL_MODE_AUTO : K4A_COLOR_CONTROL_MODE_MANUAL,
        config.ExposureUS);

    k4a_device_set_color_control(device, K4A_COLOR_CONTROL_GAIN,
        K4A_COLOR_CONTROL_MODE_MANUAL, config.Gain);

    k4a_device_set_color_control(device, K4A_COLOR_CONTROL_BRIGHTNESS,
        K4A_COLOR_CONTROL_MODE_MANUAL, config.Brightness);

    k4a_device_get_calibration(device, cfg.depth_mode, cfg.color_resolution, &calibration);
    transformation = k4a_transformation_create(&calibration);

    create_xy_table();

    return true;
}


std::shared_ptr<RGBDImage> UAzureK4ADevice::grabFrame()
{
    k4a_capture_t capture = nullptr;

    if (k4a_device_get_capture(device, &capture, 0) != K4A_WAIT_RESULT_SUCCEEDED)
        return nullptr;

    k4a_image_t depth = k4a_capture_get_depth_image(capture);
    k4a_image_t color = k4a_capture_get_color_image(capture);

    if (!depth || !color) {
        k4a_capture_release(capture);
        return nullptr;
    }

    int w = k4a_image_get_width_pixels(depth);
    int h = k4a_image_get_height_pixels(depth);

    k4a_image_t registered = nullptr;
    k4a_image_create(K4A_IMAGE_FORMAT_COLOR_BGRA32, w, h, w * 4, &registered);

    if (k4a_transformation_color_image_to_depth_camera(transformation, depth, color, registered)
        != K4A_RESULT_SUCCEEDED)
    {
        k4a_image_release(depth);
        k4a_image_release(color);
        k4a_capture_release(capture);
        k4a_image_release(registered);
        return nullptr;
    }

    auto img = std::make_shared<RGBDImage>();
    img->width = w;
    img->height = h;

    img->depth = new uint16_t[w * h];
    img->color = new uint32_t[w * h];

    img->lookup2D = lookupTable;

    memcpy(img->depth, k4a_image_get_buffer(depth), w * h * sizeof(uint16_t));
    memcpy(img->color, k4a_image_get_buffer(registered), w * h * sizeof(uint32_t));

    img->cameraId = deviceId;

    img->modelMatrix = CameraRegistrator::getMatrix(img->cameraId);

    k4a_image_release(depth);
    k4a_image_release(color);
    k4a_image_release(registered);
    k4a_capture_release(capture);

    return img;
}

void UAzureK4ADevice::CaptureLoop()
{
    while (running)
    {
        std::shared_ptr<RGBDImage> image = grabFrame();
        if (!image || !imageCallback)
            continue;

        try
        {
            imageCallback(image);
        }
        catch (const std::exception& e)
        {
            UE_LOG(LogTemp, Error, TEXT("AzureK4ADevice: callback exception: %hs"), e.what());
        }
        catch (...)
        {
            UE_LOG(LogTemp, Error, TEXT("AzureK4ADevice: callback threw unknown exception"));
        }
    }
}