#pragma once

#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "Logging/LogMacros.h"

/**
 * Startet PoseSolver.exe beim Erzeugen
 * und beendet den Prozess beim Zerstören.
 *
 * Erwarteter Pfad:
 *   Binaries/Win64/posegraph/PoseSolver.exe
 */
class PoseSolverGraph
{
public:
    PoseSolverGraph()
    {
        UE_LOG(LogTemp, Log, TEXT("[BlendPCR] PoseGraphServer starting"));
        Start();
    }

    ~PoseSolverGraph()
    {
        Stop();
    }

    PoseSolverGraph(const PoseSolverGraph&) = delete;
    PoseSolverGraph& operator=(const PoseSolverGraph&) = delete;

    PoseSolverGraph(PoseSolverGraph&& Other) noexcept
        : ProcessHandle(Other.ProcessHandle)
    {
        Other.ProcessHandle.Reset();
    }

    PoseSolverGraph& operator=(PoseSolverGraph&& Other) noexcept
    {
        if (this != &Other)
        {
            Stop();
            ProcessHandle = Other.ProcessHandle;
            Other.ProcessHandle.Reset();
        }
        return *this;
    }

    bool IsRunning()
    {
        return ProcessHandle.IsValid() &&
            FPlatformProcess::IsProcRunning(ProcessHandle);
    }

    void Stop()
    {
        if (!ProcessHandle.IsValid())
        {
            return;
        }

        UE_LOG(LogTemp, Log, TEXT("[BlendPCR] PoseGraphServer stopping"));

        FPlatformProcess::TerminateProc(ProcessHandle, true);
        FPlatformProcess::CloseProc(ProcessHandle);
        ProcessHandle.Reset();

        UE_LOG(LogTemp, Log, TEXT("[BlendPCR] PoseGraphServer stopped"));
    }

private:
    void Start()
    {
        if (ProcessHandle.IsValid())
        {
            UE_LOG(LogTemp, Verbose, TEXT("[BlendPCR] PoseGraphServer already running"));
            return;
        }

#if PLATFORM_WINDOWS
        constexpr const TCHAR* ExeName = TEXT("PoseSolver.exe");
#else
        constexpr const TCHAR* ExeName = TEXT("PoseSolver");
#endif

        const FString ExePath = FPaths::Combine(
            FPaths::ProjectDir(),
            TEXT("Binaries"),
            FPlatformProcess::GetBinariesSubdirectory(),
            TEXT("posegraph"),
            ExeName
        );

        if (!FPaths::FileExists(ExePath))
        {
            UE_LOG(
                LogTemp,
                Error,
                TEXT("[BlendPCR] PoseGraphServer failed to start: executable not found (%s)"),
                *ExePath
            );
            return;
        }

        ProcessHandle = FPlatformProcess::CreateProc(
            *ExePath,
            nullptr,
            true,
            false,
            false,
            nullptr,
            0,
            nullptr,
            nullptr
        );

        if (ProcessHandle.IsValid())
        {
            UE_LOG(LogTemp, Log, TEXT("[BlendPCR] PoseGraphServer started"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[BlendPCR] PoseGraphServer failed to start"));
        }
    }

private:
    FProcHandle ProcessHandle;
};