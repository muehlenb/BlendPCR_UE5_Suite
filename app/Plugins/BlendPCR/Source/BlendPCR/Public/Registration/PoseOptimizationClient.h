#pragma once

#include "CoreMinimal.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Json.h"
#include "JsonUtilities.h"

#include "Interfaces/IPv4/IPv4Address.h"

struct FPGPose {
    int32 Id;
    FMatrix44f T;
};

struct FPGEdge {
    int32 I;
    int32 J;
    FMatrix44f Tij;
};

class FPoseOptimizationClient
{
public:
    FPoseOptimizationClient();
    ~FPoseOptimizationClient();

    bool Connect();
    bool SendPoseGraph(
        const TArray<FPGPose>& Poses,
        const TArray<FPGEdge>& Edges,
        TArray<FPGPose>& OutOptimizedPoses);

private:
    FSocket* Socket = nullptr;

    bool SendJson(const FString& Json);
    bool ReceiveJson(FString& OutJson);
};