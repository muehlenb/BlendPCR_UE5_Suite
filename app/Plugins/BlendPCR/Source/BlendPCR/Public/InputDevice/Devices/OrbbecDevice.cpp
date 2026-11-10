#include "OrbbecDevice.h"

#include "../../Registration/CameraRegistrator.h"

#include <cstring>
#include <cmath>
#include <algorithm>

uint32_t UOrbbecDevice::GetInstalledCount()
{
    try
    {
        ob::Context ctx;
        auto deviceList = ctx.queryDeviceList();
        return deviceList ? deviceList->deviceCount() : 0;
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogTemp, Error, TEXT("OrbbecDevice: GetInstalledCount exception: %hs"), e.what());
        return 0;
    }
}

void UOrbbecDevice::Initialize(uint32_t deviceIndex, std::function<void(std::shared_ptr<RGBDImage>)> pImageCallback)
{
    index = deviceIndex;
    imageCallback = pImageCallback;

    OpenDevice();

    if (!LoadConfig())
    {
        SaveConfig();
    }
}

UOrbbecDevice::~UOrbbecDevice()
{
    StopCamera();
    CloseDevice();

    delete[] lookupTable;
    lookupTable = nullptr;

    delete[] lookup3DTo2D;
    lookup3DTo2D = nullptr;
}

bool UOrbbecDevice::OpenDevice()
{
    if (device)
    {
        return true;
    }

    try
    {
        context = std::make_shared<ob::Context>();
        auto deviceList = context->queryDeviceList();

        if (!deviceList || index >= deviceList->deviceCount())
        {
            UE_LOG(LogTemp, Error, TEXT("OrbbecDevice: Device index %d not available"), index);
            return false;
        }

        device = deviceList->getDevice(index);

        auto info = device->getDeviceInfo();
        if (info)
        {
            const char* serial = info->serialNumber();
            deviceId = serial && strlen(serial) > 0
                ? "Orbbec_" + std::string(serial)
                : "OrbbecNoSerial_" + std::to_string(index);
        }
        else
        {
            deviceId = "OrbbecNoInfo_" + std::to_string(index);
        }

        pipeline = std::make_shared<ob::Pipeline>(device);

        aligner = std::make_shared<ob::Align>(OB_STREAM_DEPTH);
        pointCloudFilter = std::make_shared<ob::PointCloudFilter>();
        pointCloudFilter->setCreatePointFormat(OB_FORMAT_POINT);

        return true;
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogTemp, Error, TEXT("OrbbecDevice: Failed to open device %d: %hs"), index, e.what());
        device = nullptr;
        pipeline = nullptr;
        context = nullptr;
        aligner = nullptr;
        pointCloudFilter = nullptr;
        return false;
    }
}

void UOrbbecDevice::CloseDevice()
{
    pipeline = nullptr;
    device = nullptr;
    context = nullptr;
    aligner = nullptr;
    pointCloudFilter = nullptr;
}

bool UOrbbecDevice::StartCamera()
{
    if (!device || !pipeline || running)
    {
        return false;
    }

    if (!ApplyConfigAndStart())
    {
        return false;
    }

    running = true;
    worker = std::thread(&UOrbbecDevice::CaptureLoop, this);

    UE_LOG(LogTemp, Display, TEXT("OrbbecDevice: Camera started (%s)"), *FString(deviceId.c_str()));
    return true;
}

void UOrbbecDevice::StopCamera()
{
    if (!running && !pipeline)
    {
        return;
    }

    running = false;

    if (worker.joinable())
    {
        worker.join();
    }

    try
    {
        if (pipeline)
        {
            pipeline->stop();
        }
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogTemp, Warning, TEXT("OrbbecDevice: stop exception: %hs"), e.what());
    }

    UE_LOG(LogTemp, Display, TEXT("OrbbecDevice: Camera stopped (%s)"), *FString(deviceId.c_str()));
}

bool UOrbbecDevice::ApplyConfig()
{
    StopCamera();
    SaveConfig();
    return StartCamera();
}

bool UOrbbecDevice::ApplyConfigAndStart()
{
    try
    {
        auto obConfig = std::make_shared<ob::Config>();

        const int fps =
            config.FPS == EOrbbecFramerate::FPS5 ? 5 :
            config.FPS == EOrbbecFramerate::FPS15 ? 15 :
            30;

        auto depthProfiles = pipeline->getStreamProfileList(OB_SENSOR_DEPTH);
        auto colorProfiles = pipeline->getStreamProfileList(OB_SENSOR_COLOR);

        auto depthProfile = depthProfiles->getVideoStreamProfile(640, 576, OB_FORMAT_Y16, fps);

        std::shared_ptr<ob::VideoStreamProfile> colorProfile = nullptr;

        try
        {
            colorProfile = colorProfiles->getVideoStreamProfile(640, 576, OB_FORMAT_BGRA, fps);
        }
        catch (...)
        {
            try
            {
                colorProfile = colorProfiles->getVideoStreamProfile(640, 480, OB_FORMAT_BGRA, fps);
            }
            catch (...)
            {
                try
                {
                    colorProfile = colorProfiles->getVideoStreamProfile(1280, 720, OB_FORMAT_BGRA, fps);
                }
                catch (...)
                {
                    colorProfile = colorProfiles->getVideoStreamProfile(1280, 720, OB_FORMAT_RGB, fps);
                }
            }
        }

        obConfig->enableStream(depthProfile);
        obConfig->enableStream(colorProfile);
        obConfig->setFrameAggregateOutputMode(OB_FRAME_AGGREGATE_OUTPUT_FULL_FRAME_REQUIRE);

        if (config.bEnableFrameSync)
        {
            pipeline->enableFrameSync();
        }

        try
        {
            if (device->isPropertySupported(OB_PROP_COLOR_AUTO_EXPOSURE_BOOL, OB_PERMISSION_WRITE))
            {
                device->setBoolProperty(OB_PROP_COLOR_AUTO_EXPOSURE_BOOL, config.AutoExposure);
            }

            if (!config.AutoExposure &&
                device->isPropertySupported(OB_PROP_COLOR_EXPOSURE_INT, OB_PERMISSION_WRITE))
            {
                device->setIntProperty(OB_PROP_COLOR_EXPOSURE_INT, config.Exposure);
            }

            if (device->isPropertySupported(OB_PROP_COLOR_AUTO_WHITE_BALANCE_BOOL, OB_PERMISSION_WRITE))
            {
                device->setBoolProperty(OB_PROP_COLOR_AUTO_WHITE_BALANCE_BOOL, config.AutoWhiteBalance);
            }

            if (!config.AutoWhiteBalance &&
                device->isPropertySupported(OB_PROP_COLOR_WHITE_BALANCE_INT, OB_PERMISSION_WRITE))
            {
                device->setIntProperty(OB_PROP_COLOR_WHITE_BALANCE_INT, config.WhiteBalance);
            }

            if (device->isPropertySupported(OB_PROP_COLOR_GAIN_INT, OB_PERMISSION_WRITE))
            {
                device->setIntProperty(OB_PROP_COLOR_GAIN_INT, config.Gain);
            }

            if (device->isPropertySupported(OB_PROP_COLOR_BRIGHTNESS_INT, OB_PERMISSION_WRITE))
            {
                device->setIntProperty(OB_PROP_COLOR_BRIGHTNESS_INT, config.Brightness);
            }

            if (device->isPropertySupported(OB_PROP_COLOR_CONTRAST_INT, OB_PERMISSION_WRITE))
            {
                device->setIntProperty(OB_PROP_COLOR_CONTRAST_INT, config.Contrast);
            }

            if (device->isPropertySupported(OB_PROP_COLOR_SATURATION_INT, OB_PERMISSION_WRITE))
            {
                device->setIntProperty(OB_PROP_COLOR_SATURATION_INT, config.Saturation);
            }

            if (device->isPropertySupported(OB_PROP_COLOR_SHARPNESS_INT, OB_PERMISSION_WRITE))
            {
                device->setIntProperty(OB_PROP_COLOR_SHARPNESS_INT, config.Sharpness);
            }
        }
        catch (const std::exception& e)
        {
            UE_LOG(LogTemp, Warning, TEXT("OrbbecDevice: Failed to apply color settings: %hs"), e.what());
        }

        pipeline->start(obConfig);

        depthWidth = depthProfile->width();
        depthHeight = depthProfile->height();

        delete[] lookupTable;
        lookupTable = nullptr;

        delete[] lookup3DTo2D;
        lookup3DTo2D = nullptr;

        return true;
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogTemp, Error, TEXT("OrbbecDevice: ApplyConfigAndStart exception: %hs"), e.what());
        return false;
    }
}

void UOrbbecDevice::create_xy_table(std::shared_ptr<ob::FrameSet> frameSet)
{
    if (lookupTable)
    {
        return;
    }

    if (!frameSet || !frameSet->depthFrame() || !aligner || !pointCloudFilter)
    {
        return;
    }

    const int width = frameSet->depthFrame()->width();
    const int height = frameSet->depthFrame()->height();

    if (width <= 0 || height <= 0)
    {
        return;
    }

    std::vector<uint16_t> originalDepth(width * height);

    uint16_t* depthData = reinterpret_cast<uint16_t*>(frameSet->depthFrame()->data());
    std::memcpy(originalDepth.data(), depthData, width * height * sizeof(uint16_t));

    for (int i = 0; i < width * height; ++i)
    {
        depthData[i] = 1000;
    }

    std::shared_ptr<ob::FrameSet> alignedFrameSet =
        std::static_pointer_cast<ob::FrameSet>(aligner->process(frameSet));

    std::shared_ptr<ob::Frame> pointFrame = pointCloudFilter->process(alignedFrameSet);

    std::memcpy(depthData, originalDepth.data(), width * height * sizeof(uint16_t));

    if (!pointFrame || !pointFrame->data())
    {
        return;
    }

    lookupTable = new float[width * height * 2];

    const float* points = reinterpret_cast<const float*>(pointFrame->data());

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const int idx = x + y * width;

            lookupTable[idx * 2] = points[idx * 3] / 1000.0f;
            lookupTable[idx * 2 + 1] = points[idx * 3 + 1] / 1000.0f;
        }
    }

    generate3DTo2D(width, height);
}

void UOrbbecDevice::generate3DTo2D(int width, int height)
{
    if (!lookupTable || width <= 1 || height <= 1)
    {
        return;
    }

    delete[] lookup3DTo2D;
    lookup3DTo2D = nullptr;

    constexpr int LookupSize = 1024;
    lookup3DTo2D = new float[LookupSize * LookupSize * 2];

    for (int i = 0; i < LookupSize * LookupSize; ++i)
    {
        lookup3DTo2D[i * 2] = -1.0f;
        lookup3DTo2D[i * 2 + 1] = -1.0f;
    }

    for (int y = 0; y < height - 1; ++y)
    {
        for (int x = 0; x < width - 1; ++x)
        {
            const int id = y * width + x;

            float luX00 = lookupTable[id * 2];
            float luY00 = lookupTable[id * 2 + 1];

            const int id01 = (y + 1) * width + x;
            float luY01 = lookupTable[id01 * 2 + 1];

            const int id10 = y * width + (x + 1);
            float luX10 = lookupTable[id10 * 2];

            float fStartX = (luX00 * 0.5f + 0.5f) * (LookupSize - 1);
            float fEndX = (luX10 * 0.5f + 0.5f) * (LookupSize - 1);
            float fStartY = (luY00 * 0.5f + 0.5f) * (LookupSize - 1);
            float fEndY = (luY01 * 0.5f + 0.5f) * (LookupSize - 1);

            if (fStartX > fEndX)
            {
                std::swap(fStartX, fEndX);
                std::swap(luX00, luX10);
            }

            if (fStartY > fEndY)
            {
                std::swap(fStartY, fEndY);
                std::swap(luY00, luY01);
            }

            const int startPX = std::max(0, static_cast<int>(fStartX));
            const int endPX = std::min(LookupSize - 1, static_cast<int>(std::ceil(fEndX)));
            const int startPY = std::max(0, static_cast<int>(fStartY));
            const int endPY = std::min(LookupSize - 1, static_cast<int>(std::ceil(fEndY)));

            for (int pY = startPY; pY <= endPY; ++pY)
            {
                for (int pX = startPX; pX <= endPX; ++pX)
                {
                    const float rangeX = fEndX - fStartX;
                    const float rangeY = fEndY - fStartY;

                    const float progressX = rangeX != 0.0f ? (pX - fStartX) / rangeX : 0.0f;
                    const float progressY = rangeY != 0.0f ? (pY - fStartY) / rangeY : 0.0f;

                    const int oid = pY * LookupSize + pX;

                    lookup3DTo2D[oid * 2] = x + progressX;
                    lookup3DTo2D[oid * 2 + 1] = y + progressY;
                }
            }
        }
    }
}

static void CopyRGBToBGRA(const uint8_t* SrcRgb, uint32_t* DstBgra, int PixelCount)
{
    for (int i = 0; i < PixelCount; ++i)
    {
        const uint8_t r = SrcRgb[i * 3 + 0];
        const uint8_t g = SrcRgb[i * 3 + 1];
        const uint8_t b = SrcRgb[i * 3 + 2];

        DstBgra[i] =
            (static_cast<uint32_t>(255) << 24) |
            (static_cast<uint32_t>(r) << 16) |
            (static_cast<uint32_t>(g) << 8) |
            static_cast<uint32_t>(b);
    }
}

std::shared_ptr<RGBDImage> UOrbbecDevice::grabFrame()
{
    try
    {
        auto frameSet = pipeline->waitForFrames(100);
        if (!frameSet)
        {
            return nullptr;
        }

        auto depthFrame = frameSet->depthFrame();
        auto colorFrame = frameSet->colorFrame();

        if (!depthFrame || !colorFrame)
        {
            return nullptr;
        }

        auto depthVideoFrame = depthFrame->as<ob::VideoFrame>();

        const int w = depthVideoFrame->width();
        const int h = depthVideoFrame->height();

        if (w <= 0 || h <= 0)
        {
            return nullptr;
        }

        if (!lookupTable)
        {
            create_xy_table(frameSet);
        }

        std::shared_ptr<ob::FrameSet> alignedFrameSet =
            std::static_pointer_cast<ob::FrameSet>(aligner->process(frameSet));

        auto alignedColorFrame = alignedFrameSet ? alignedFrameSet->colorFrame() : nullptr;

        auto img = std::make_shared<RGBDImage>();
        img->width = w;
        img->height = h;

        img->depth = new uint16_t[w * h];
        img->color = new uint32_t[w * h];

        img->lookup2D = lookupTable;

        std::memcpy(img->depth, depthFrame->data(), w * h * sizeof(uint16_t));

        if (alignedColorFrame && alignedColorFrame->data())
        {
            auto alignedColorVideoFrame = alignedColorFrame->as<ob::VideoFrame>();
            const int colorW = alignedColorVideoFrame->width();
            const int colorH = alignedColorVideoFrame->height();

            if (colorW == w && colorH == h)
            {
                auto profile = alignedColorFrame->getStreamProfile();
                auto videoProfile = profile->as<ob::VideoStreamProfile>();
                const OBFormat format = videoProfile->format();

                if (format == OB_FORMAT_BGRA)
                {
                    std::memcpy(img->color, alignedColorFrame->data(), w * h * sizeof(uint32_t));
                }
                else if (format == OB_FORMAT_RGB)
                {
                    CopyRGBToBGRA(
                        reinterpret_cast<const uint8_t*>(alignedColorFrame->data()),
                        img->color,
                        w * h
                    );
                }
                else
                {
                    std::memset(img->color, 0, w * h * sizeof(uint32_t));
                    UE_LOG(LogTemp, Warning, TEXT("OrbbecDevice: Unsupported aligned color format"));
                }
            }
            else
            {
                std::memset(img->color, 0, w * h * sizeof(uint32_t));
                UE_LOG(LogTemp, Warning, TEXT("OrbbecDevice: Aligned color size mismatch: color %dx%d, depth %dx%d"), colorW, colorH, w, h);
            }
        }
        else
        {
            std::memset(img->color, 0, w * h * sizeof(uint32_t));
        }

        img->cameraId = deviceId;
        img->modelMatrix = CameraRegistrator::getMatrix(img->cameraId);

        return img;
    }
    catch (const ob::Error&)
    {
        UE_LOG(LogTemp, Error, TEXT("OrbbecDevice: grabFrame ob::Error"));
        return nullptr;
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogTemp, Error, TEXT("OrbbecDevice: grabFrame exception: %hs"), e.what());
        return nullptr;
    }
}

void UOrbbecDevice::CaptureLoop()
{
    while (running)
    {
        std::shared_ptr<RGBDImage> image = grabFrame();

        if (!image || !imageCallback)
        {
            continue;
        }

        try
        {
            imageCallback(image);
        }
        catch (const std::exception& e)
        {
            UE_LOG(LogTemp, Error, TEXT("OrbbecDevice: callback exception: %hs"), e.what());
        }
        catch (...)
        {
            UE_LOG(LogTemp, Error, TEXT("OrbbecDevice: callback threw unknown exception"));
        }
    }
}

FString UOrbbecDevice::GetConfigPath() const
{
    return FPaths::ProjectSavedDir() / TEXT("CameraConfig") / FString::Printf(TEXT("%s.json"), *FString(deviceId.c_str()));
}

bool UOrbbecDevice::LoadConfig()
{
    FString Json;

    if (!FFileHelper::LoadFileToString(Json, *GetConfigPath()))
    {
        return false;
    }

    return FJsonObjectConverter::JsonObjectStringToUStruct(Json, &config, 0, 0);
}

bool UOrbbecDevice::SaveConfig() const
{
    FString Json;

    if (!FJsonObjectConverter::UStructToJsonObjectString(FOrbbecConfig::StaticStruct(), &config, Json, 0, 0))
    {
        return false;
    }

    IFileManager::Get().MakeDirectory(*FPaths::GetPath(GetConfigPath()), true);
    return FFileHelper::SaveStringToFile(Json, *GetConfigPath());
}