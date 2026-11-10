#pragma once

#if WITH_OPENCV
#include "OpenCVHelper.h"

#include "PreOpenCVHeaders.h"
#include "opencv2/opencv.hpp"
#include "PostOpenCVHeaders.h"
#endif

#include <map>
#include <string>
#include <memory>
#include <thread>
#include <execution>

#include "Async/Async.h"
#include "DrawDebugHelpers.h"

#include "Registration/CameraRegistrationStatus.h"
#include "Registration/PoseOptimizationClient.h"

#include "PoseSolverServer.h"

#include "../RGBDImage.h"

struct ChessboardDetectionResult {
    ChessboardDetectionResult(int w, int h) {
        points3D = std::vector<cv::Point3f>(w * h);
        imagePoints = std::vector<cv::Point2f>(w * h);
    }

    ChessboardDetectionResult() {}

    cv::Point3f mid() {
        cv::Point3f result(0, 0, 0);
        for (const cv::Point3f& p : points3D) {
            result += p;
        }
        return result / static_cast<float>(points3D.size());
    }

    bool isValid() { return points3D.size() > 0;}

    std::vector<cv::Point3f> points3D;
    std::vector<cv::Point2f> imagePoints;
};

class CameraRegistrator {
public:
    std::map<std::string, bool> checkerboardRecognized;

    TArray<FCameraRegistrationStatus> GetCameraRegistrationStatuses() {
        return CameraRegistrationStatuses;
    }

    float GetThreadFPS() {
        return CurrentFPS;
    }
private:
    PoseSolverGraph poseSolverGraph;
    TArray<FCameraRegistrationStatus> CameraRegistrationStatuses;

    std::mutex imagesMutex;
    static std::mutex matricesMutex;

    std::map<std::string, std::shared_ptr<RGBDImage>> currentImages;
    std::map<std::string, std::map<int, ChessboardDetectionResult>> correspondences;

    bool isRunning = false;
    bool isFullyStopped = true;
    std::thread thread;

    int currentFrame = 0;

    double LastFPSTime = 0;
    int32 FrameCountFPS = 0;
    float CurrentFPS = 0.f;

    int checkerboardInnerWidth = 6;
    int checkerboardInnerHeight = 4;

    bool shouldClearCalibrationNextFrame = false;

    static std::map<std::string, FMatrix44f>& GetEstimatedMatrices();
    //static std::map<std::string, FMatrix44f> estimatedMatrices;

    bool SaveEstimatedMatricesToJson(){
        TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
        for (const auto& Pair : GetEstimatedMatrices())
        {
            const std::string& Name = Pair.first;
            const FMatrix44f& M = Pair.second;

            TArray<TSharedPtr<FJsonValue>> Values;
            Values.Reserve(16);

            for (int32 Row = 0; Row < 4; ++Row)
            {
                for (int32 Col = 0; Col < 4; ++Col)
                {
                    Values.Add(MakeShared<FJsonValueNumber>(M.M[Row][Col]));
                }
            }

            RootObject->SetArrayField(UTF8_TO_TCHAR(Name.c_str()), Values);
        }

        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);

        if (!FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer))
        {
            return false;
        }

        const FString Directory = FPaths::ProjectSavedDir() / TEXT("Calibration");
        IFileManager::Get().MakeDirectory(*Directory, true);
        const FString FilePath = Directory / TEXT("estimated_matrices.json");
        return FFileHelper::SaveStringToFile(OutputString, *FilePath);
    }

    bool LoadEstimatedMatricesFromJson(){
        const FString FilePath = FPaths::ProjectSavedDir() / TEXT("Calibration/estimated_matrices.json");

        FString InputString;
        if (!FFileHelper::LoadFileToString(InputString, *FilePath))
        {
            return false;
        }

        TSharedPtr<FJsonObject> RootObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InputString);

        if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
        {
            return false;
        }

        std::map<std::string, FMatrix44f> estMat;

        for (const auto& Pair : RootObject->Values)
        {
            const FString& Key = Pair.Key;
            const TSharedPtr<FJsonValue>& Value = Pair.Value;

            const TArray<TSharedPtr<FJsonValue>>* Array;
            if (!Value->TryGetArray(Array) || Array->Num() != 16)
            {
                continue;
            }

            FMatrix44f M;

            int32 Index = 0;
            for (int32 Row = 0; Row < 4; ++Row)
            {
                for (int32 Col = 0; Col < 4; ++Col)
                {
                    M.M[Row][Col] = static_cast<float>((*Array)[Index++]->AsNumber());
                }
            }

            estMat.emplace(TCHAR_TO_UTF8(*Key), M);
        }

        GetEstimatedMatrices() = estMat;

        return true;
    }


    cv::Matx44f estimateTransformSVD(const std::vector<cv::Point3f>& a, const std::vector<cv::Point3f>& b){
        CV_Assert(a.size() == b.size() && !a.empty());

        // Schwerpunkt berechnen
        cv::Point3f ca(0, 0, 0), cb(0, 0, 0);
        for (size_t i = 0; i < a.size(); ++i) {
            ca += a[i];
            cb += b[i];
        }
        ca *= 1.0f / a.size();
        cb *= 1.0f / b.size();

        // Zentrierte Punktwolken
        cv::Mat A(a.size(), 3, CV_32F);
        cv::Mat B(b.size(), 3, CV_32F);

        for (size_t i = 0; i < a.size(); ++i) {
            A.at<float>(i, 0) = a[i].x - ca.x;
            A.at<float>(i, 1) = a[i].y - ca.y;
            A.at<float>(i, 2) = a[i].z - ca.z;

            B.at<float>(i, 0) = b[i].x - cb.x;
            B.at<float>(i, 1) = b[i].y - cb.y;
            B.at<float>(i, 2) = b[i].z - cb.z;
        }

        // Kovarianzmatrix
        cv::Mat H = A.t() * B;

        // SVD
        cv::SVD svd(H, cv::SVD::FULL_UV);
        cv::Mat R = svd.vt.t() * svd.u.t();

        // Reflektionsfall korrigieren
        if (cv::determinant(R) < 0) {
            cv::Mat Vt = svd.vt.clone();
            Vt.row(2) *= -1;
            R = Vt.t() * svd.u.t();
        }

        // Translation
        cv::Mat t = (cv::Mat_<float>(3, 1) << cb.x, cb.y, cb.z) - R * (cv::Mat_<float>(3, 1) << ca.x, ca.y, ca.z);

        // 4×4 Matrix bauen
        cv::Matx44f T = cv::Matx44f::eye();
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                T(r, c) = R.at<float>(r, c);

        T(0, 3) = t.at<float>(0);
        T(1, 3) = t.at<float>(1);
        T(2, 3) = t.at<float>(2);

        return T;
    }

    inline int idx(int x, int y, int w)
    {
        return y * w + x;
    }

    std::pair<std::vector<cv::Point3f>, std::vector< cv::Point3f>> 
        findBestPairs(FMatrix44f MatrixI, ChessboardDetectionResult DetectionI, FMatrix44f MatrixJ, ChessboardDetectionResult DetectionJ)
    {
        int w = checkerboardInnerWidth;
        int h = checkerboardInnerHeight;

        const int rotations[4] = { 0, 90, 180, 270 };
        const bool mirrors[2] = { false, true };

        float minError = 99999999.f;
        std::vector<cv::Point3f> bestI;
        std::vector< cv::Point3f> bestJ;

        for (bool mirror : mirrors)
        {
            for (int rot : rotations)
            {
                std::vector<cv::Point3f> thisI;
                std::vector< cv::Point3f> thisJ;
                int w2 = w;
                int h2 = h;

                float error = 0;

                for (int y = 0; y < h; ++y)
                {
                    for (int x = 0; x < w; ++x)
                    {
                        int xr = 0, yr = 0;

                        if (rot == 0)
                        {
                            xr = x;
                            yr = y;
                            w2 = w;
                            h2 = h;
                        }
                        else if (rot == 90)
                        {
                            xr = h - 1 - y;
                            yr = x;
                            w2 = h;
                            h2 = w;
                        }
                        else if (rot == 180)
                        {
                            xr = w - 1 - x;
                            yr = h - 1 - y;
                            w2 = w;
                            h2 = h;
                        }
                        else if (rot == 270)
                        {
                            xr = y;
                            yr = w - 1 - x;
                            w2 = h;
                            h2 = w;
                        }

                        if (mirror)
                        {
                            xr = w2 - 1 - xr;
                        }

                        int old_idx = idx(x, y, w);
                        int new_idx = idx(xr, yr, w2);

                        cv::Point3f pI = DetectionI.points3D[old_idx];
                        cv::Point3f pJ = DetectionJ.points3D[new_idx];
                        thisI.push_back(pI);
                        thisJ.push_back(pJ);

                        FVector4f wI = MatrixI.GetTransposed().TransformPosition(FVector4f(pI.x, pI.y, pI.z, 1));
                        FVector4f wJ = MatrixJ.GetTransposed().TransformPosition(FVector4f(pJ.x, pJ.y, pJ.z, 1));

                        error += (wI - wJ).Size();
                    }
                }
                if (minError > error) {
                    minError = error;
                    bestI = thisI;
                    bestJ = thisJ;
                }
            }
        }

        if (minError / (w * h) < 5.f) {
            return std::pair<std::vector<cv::Point3f>, std::vector< cv::Point3f>>(bestI, bestJ);
        }

        return std::pair<std::vector<cv::Point3f>, std::vector< cv::Point3f>>();
    }

    TArray<FPGEdge> createEdgeGraph(std::vector<std::string>& keys, std::map<std::string, int>& camToNode, bool precise, TArray<FPGPose>& RoughOptimized) {
        int N = correspondences.size();

        TArray<FPGEdge> Edges;

        for (int i = 0; i < N; ++i)
        {
            for (int j = i + 1; j < N; ++j)
            {
                std::string camI = keys[i];
                std::string camJ = keys[j];

                std::vector<cv::Point3f> setI;
                std::vector<cv::Point3f> setJ;

                for (const auto& kv : correspondences[camI])
                {
                    int frame = kv.first;
                    if (correspondences[camJ].count(frame))
                    {
                        if (!precise) {
                            setI.push_back(correspondences[camI][frame].mid());
                            setJ.push_back(correspondences[camJ][frame].mid());
                        } else {
                            if (RoughOptimized.Num() > i && RoughOptimized.Num() > j) {
                                auto pair = findBestPairs(RoughOptimized[i].T, correspondences[camI][frame], RoughOptimized[j].T, correspondences[camJ][frame]);

                                if (pair.first.size() > 0 && pair.first.size() == pair.second.size()) {
                                    setI.insert(setI.end(), pair.first.begin(), pair.first.end());
                                    setJ.insert(setJ.end(), pair.second.begin(), pair.second.end());
                                }
                            }
                        }
                    }
                }

                if (setI.size() < 4)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Nicht genug gemeinsame Frames für Kamera %s und %s"),
                        *FString(camI.c_str()), *FString(camJ.c_str()));
                    continue;
                }

                cv::Matx44f T_ItoJ = estimateTransformSVD(setI, setJ);

                // In Unreal-Matrix konvertieren
                FMatrix44f Mat;
                for (int r = 0; r < 4; ++r)
                    for (int c = 0; c < 4; ++c)
                        Mat.M[r][c] = T_ItoJ(r, c);

                FMatrix44f MatInv = Mat.Inverse();

                FPGEdge E;
                E.I = camToNode[camI];
                E.J = camToNode[camJ];
                E.Tij = MatInv;

                Edges.Add(E);
            }
        }

        return Edges;
    }

    void calculateStatistics(TArray<FCameraRegistrationStatus>& newStatuses, std::vector<std::string>& keys, TArray<FMatrix44f>& Poses) {
        int N = correspondences.size();

        std::vector<int> numberOfCorrespondences;
        std::vector<float> averageError;

        for (int i = 0; i < keys.size(); ++i) {
            numberOfCorrespondences.push_back(0);
            averageError.push_back(0.f);
        }

        for (int i = 0; i < N; ++i)
        {
            for (int j = i + 1; j < N; ++j)
            {
                std::string camI = keys[i];
                std::string camJ = keys[j];

                for (const auto& kv : correspondences[camI])
                {
                    int frame = kv.first;
                    if (correspondences[camJ].count(frame))
                    {
                        cv::Point3f pI = correspondences[camI][frame].mid();
                        cv::Point3f pJ = correspondences[camJ][frame].mid();

                        FVector4f wI = Poses[i].TransformPosition(FVector4f(pI.x, pI.y, pI.z, 1.0));
                        FVector4f wJ = Poses[j].TransformPosition(FVector4f(pJ.x, pJ.y, pJ.z, 1.0));

                        float error = (wI - wJ).Size();

                        ++numberOfCorrespondences[i];
                        ++numberOfCorrespondences[j];

                        averageError[i] += error;
                        averageError[j] += error;
                    }
                }
            }
        }

        for (int i = 0; i < keys.size(); ++i) {
            averageError[i] /= numberOfCorrespondences[i];
            newStatuses[i].AverageError = averageError[i];
            newStatuses[i].NumSamples = numberOfCorrespondences[i];
        }
    }

    void estimateMatrices()
    {
        if (correspondences.size() < 2)
            return;

        std::vector<std::string> keys;
        keys.reserve(correspondences.size());
        for (const auto& kv : correspondences)
            keys.push_back(kv.first);

        int N = keys.size();

        TArray<FPGPose> Poses;
        Poses.Reserve(N);

        for (int i = 0; i < N; ++i)
        {
            FPGPose Pose;
            Pose.Id = i;
            Pose.T = FMatrix44f::Identity;
            Poses.Add(Pose);
        }

        std::map<std::string, int> camToNode;
        for (int i = 0; i < N; ++i)
            camToNode[keys[i]] = i;


        TArray<FPGPose> RoughOptimized;
        {
            TArray<FPGEdge> Edges = createEdgeGraph(keys, camToNode, false, Poses);

            if (Edges.Num() == 0) {
                UE_LOG(LogTemp, Warning, TEXT("Not enough correspondences to build PoseGraph"));
                return;
            }

            FPoseOptimizationClient Client;
            if (!Client.Connect()) {
                UE_LOG(LogTemp, Error, TEXT("Failed to connect to PoseGraph server"));
                return;
            }

            if (!Client.SendPoseGraph(Poses, Edges, RoughOptimized)) {
                UE_LOG(LogTemp, Error, TEXT("PoseGraph optimization failed"));
                return;
            }
        }

        TArray<FPGPose> FineOptimized;
        {
            TArray<FPGEdge> Edges = createEdgeGraph(keys, camToNode, true, RoughOptimized);

            if (Edges.Num() == 0) {
                UE_LOG(LogTemp, Warning, TEXT("Precise Optimization: Not enough correspondences to build PoseGraph"));
                return;
            }

            FPoseOptimizationClient Client;
            if (!Client.Connect()) {
                UE_LOG(LogTemp, Error, TEXT("Precise Optimization: Failed to connect to PoseGraph server"));
                return;
            }

            if (!Client.SendPoseGraph(Poses, Edges, FineOptimized)) {
                UE_LOG(LogTemp, Error, TEXT("Precise Optimization: PoseGraph optimization failed"));
                return;
            }
        }

        TArray<FCameraRegistrationStatus> newStatuses;
        for (std::string key : keys) {
            FCameraRegistrationStatus status;
            status.CameraId = FString(key.c_str());
            status.bCheckerboardDetected = checkerboardRecognized[key];
            newStatuses.Add(status);
        }

        TArray<FMatrix44f> FinalMatrices;
        for(FPGPose& pose : FineOptimized) {
            FinalMatrices.Add(pose.T.GetTransposed());
        }

        calculateStatistics(newStatuses, keys, FinalMatrices);

        std::map<std::string, FMatrix44f> newEstimate;
        for (const auto& Pose : FineOptimized)
        {
            FString camId = FString(keys[Pose.Id].c_str());
            newEstimate[keys[Pose.Id]] = Pose.T;

            /*UE_LOG(LogTemp, Warning,
                TEXT("Optimized Pose Camera %s:\n%s"),
                *camId,
                *Pose.T.ToString());
                
            if (Pose.Id == 1) {
                FMatrix44f m = Pose.T;

                UE_LOG(LogTemp, Warning, TEXT("s: %f %f %f %f | %f %f %f %f"), m.M[0][0], m.M[0][1], m.M[0][2], m.M[0][3], m.M[1][0], m.M[1][1], m.M[1][2], m.M[1][3])
            }*/
        }
        UE_LOG(LogTemp, Warning, TEXT("Updated Calibration!"));

        for (FCameraRegistrationStatus& status : newStatuses) {
            std::string camId = TCHAR_TO_UTF8(*status.CameraId);
            status.Transform = newEstimate[camId];
        }

        CameraRegistrationStatuses = newStatuses;

        {
            std::lock_guard<std::mutex> lock(matricesMutex);
            GetEstimatedMatrices() = newEstimate;
            SaveEstimatedMatricesToJson();
        }
    }

    void createThread() {
        isRunning = true;

        thread = std::thread([this]() {
            while (isRunning) {
                std::map<std::string, std::shared_ptr<RGBDImage>> tmpImages;

                {
                    std::lock_guard<std::mutex> lock(imagesMutex);
                    tmpImages = currentImages;
                }

                int frame = currentFrame++;

                bool newChessboardRecognized = false;

                std::for_each(std::execution::par, tmpImages.begin(), tmpImages.end(), [this, frame, &newChessboardRecognized](const auto& rgbdimage) {
                    std::shared_ptr<RGBDImage> image = rgbdimage.second;
                    if (image != nullptr) {
                        cv::Mat bgra(image->height, image->width, CV_8UC4, (void*)image->color);

                        cv::Mat gray;
                        cv::cvtColor(bgra, gray, cv::COLOR_BGRA2GRAY);

                        std::vector<cv::Point2f> corners;
                        cv::Size patternSize(checkerboardInnerWidth, checkerboardInnerHeight);

                        bool found = cv::findChessboardCorners(gray, patternSize, corners, cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE);

                        if (found) {
                            ChessboardDetectionResult result(patternSize.width, patternSize.height);

                            int numPoints = patternSize.width * patternSize.height;

                            bool isValid = true;
                            bool isFirst = true;
                            cv::Point3f firstP;

                            for (int i = 0; i < numPoints; ++i) {
                                result.imagePoints[i] = corners[i];

                                float x, y, z;
                                rgbdimage.second->get3DCoord(corners[i].x, corners[i].y, x, y, z);

                                cv::Point3f p = cv::Point3f(x, y, z);;
                                result.points3D[i] = p;

                                if (!(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z) && p.z > 0)) {
                                    isValid = false;
                                    break;
                                }

                                if (isFirst) {
                                    isFirst = false;
                                    firstP = p;
                                }
                                else if (cv::norm(firstP - p) > 50) {
                                    isValid = false;
                                }
                            }

                            if (isValid) {
                                correspondences[rgbdimage.second->cameraId].emplace(frame, result);
                                newChessboardRecognized = true;
                                checkerboardRecognized[image->cameraId] = true;
                            } else {
                                checkerboardRecognized[image->cameraId] = false;
                            }
                        } else {
                            checkerboardRecognized[image->cameraId] = false;
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                });
                
                if (newChessboardRecognized) {
                    estimateMatrices();
                }
                else 
                {   
                    // At least update camera detected status:
                    for (FCameraRegistrationStatus& status : CameraRegistrationStatuses) {
                        std::string test2 = std::string(TCHAR_TO_UTF8(*status.CameraId));
                        status.bCheckerboardDetected = checkerboardRecognized[test2];
                    }
                }

                ++FrameCountFPS;

                double Now = FPlatformTime::Seconds();
                if (Now - LastFPSTime >= 1.0)
                {
                    CurrentFPS = FrameCountFPS / (Now - LastFPSTime);
                    FrameCountFPS = 0;
                    LastFPSTime = Now;
                }

                if (shouldClearCalibrationNextFrame) {
                    shouldClearCalibrationNextFrame = false;
                    correspondences.clear();

                    {
                        std::lock_guard<std::mutex> lock(matricesMutex);
                        GetEstimatedMatrices().clear();
                        SaveEstimatedMatricesToJson();
                    }

                    for (FCameraRegistrationStatus& status : CameraRegistrationStatuses) {
                        std::string test2 = std::string(TCHAR_TO_UTF8(*status.CameraId));
                        status.AverageError = 0.f;
                        status.NumSamples = 0;
                    }
                }
            }
            isFullyStopped = true;
        });
    }



public:
    CameraRegistrator(){
        LoadEstimatedMatricesFromJson();
    }

    ~CameraRegistrator() {
        stop();
    }

    bool start() {
        if (isFullyStopped) {
            isFullyStopped = false;
            createThread();

            TArray<FCameraRegistrationStatus> newStatuses;
            
            for (auto& pair : currentImages) {
                FCameraRegistrationStatus status;
                status.CameraId = FString(pair.first.c_str());
                newStatuses.Add(status);
            }

            CameraRegistrationStatuses = newStatuses;
            return true;
        }
        return false;
    }

    bool isCurrentlyRunning() {
        return isRunning;
    }

    void clearCalibration() {
        shouldClearCalibrationNextFrame = true;
    }

    void stop() {
        if (isRunning) {
            isRunning = false;
            if (thread.joinable())
                thread.join();
            isFullyStopped = true;
        }
    }

    bool setCheckerboardSize(int innerWidth, int innerHeight){
        if (innerWidth > 2 && innerHeight > 2 && innerWidth < 20 && innerHeight < 20) {
            checkerboardInnerWidth = innerWidth;
            checkerboardInnerHeight = innerHeight;
            return true;
        }
        return false;
    }

    int getCheckerboardWidth() {
        return checkerboardInnerWidth;
    }

    int getCheckerboardHeight() {
        return checkerboardInnerHeight;
    }

    void updateRGBDImage(std::shared_ptr<RGBDImage> rgbdimage) {
        std::lock_guard<std::mutex> lock(imagesMutex);
        std::string cameraId = rgbdimage->cameraId;
        if (currentImages.count(cameraId) == 0) {
            currentImages.emplace(cameraId, rgbdimage);
        }
        else
        {
            currentImages[cameraId] = rgbdimage;
        }
    }

    static FMatrix44f getMatrix(std::string cameraId) {
        std::lock_guard<std::mutex> lock(matricesMutex);
        if (GetEstimatedMatrices().count(cameraId) > 0) {
            return FMatrix44f(GetEstimatedMatrices()[cameraId]);
        }
        else 
        {
            // GetEstimatedMatrices().emplace(cameraId, FMatrix44f::Identity);
        }
        return FMatrix44f::Identity;
    }
};