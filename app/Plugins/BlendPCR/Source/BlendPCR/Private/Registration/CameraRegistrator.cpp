#include "Registration/CameraRegistrator.h"

std::map<std::string, FMatrix44f>& CameraRegistrator::GetEstimatedMatrices()
{
    static std::map<std::string, FMatrix44f> Instance;
    return Instance;
}

std::mutex CameraRegistrator::matricesMutex;