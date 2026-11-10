#include "BlendPCRShaders.h"

// Hole Filling Shaders:
IMPLEMENT_GLOBAL_SHADER(FHoleFillingVS, "/CustomShaders/HoleFillingPass.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FHoleFillingPS, "/CustomShaders/HoleFillingPass.usf", "MainPS", SF_Pixel);

// Point Cloud Shaders:
IMPLEMENT_GLOBAL_SHADER(FPointCloudVS, "/CustomShaders/GenPointCloudPass.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FPointCloudPS, "/CustomShaders/GenPointCloudPass.usf", "MainPS", SF_Pixel);

// Rejection Shaders:
IMPLEMENT_GLOBAL_SHADER(FRejectionVS, "/CustomShaders/RejectionPass.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FRejectionPS, "/CustomShaders/RejectionPass.usf", "MainPS", SF_Pixel);

// Edge Distance:
IMPLEMENT_GLOBAL_SHADER(FEdgeDistanceVS, "/CustomShaders/EdgeDistancePass.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FEdgeDistancePS, "/CustomShaders/EdgeDistancePass.usf", "MainPS", SF_Pixel);

// MLS Pass:
IMPLEMENT_GLOBAL_SHADER(FMLSPointCloudVS, "/CustomShaders/MLSPass.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FMLSPointCloudPS, "/CustomShaders/MLSPass.usf", "MainPS", SF_Pixel);

// Normal Pass:
IMPLEMENT_GLOBAL_SHADER(FNormalVS, "/CustomShaders/NormalPass.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FNormalPS, "/CustomShaders/NormalPass.usf", "MainPS", SF_Pixel);

// Quality Pass:
IMPLEMENT_GLOBAL_SHADER(FQualityVS, "/CustomShaders/QualityPass.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FQualityPS, "/CustomShaders/QualityPass.usf", "MainPS", SF_Pixel);

// Screen, Separate Meshes:
IMPLEMENT_GLOBAL_SHADER(FSeparateMeshVS, "/CustomShaders/Screen_SeparateMeshes.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FSeparateMeshPS, "/CustomShaders/Screen_SeparateMeshes.usf", "MainPS", SF_Pixel);

// Major Cam, Separate Meshes:
IMPLEMENT_GLOBAL_SHADER(FMajorCamVS, "/CustomShaders/Screen_MajorCamera.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FMajorCamPS, "/CustomShaders/Screen_MajorCamera.usf", "MainPS", SF_Pixel);

// Camera Weights, Separate Meshes:
IMPLEMENT_GLOBAL_SHADER(FCameraWeightsVS, "/CustomShaders/Screen_CameraWeights.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FCameraWeightsPS, "/CustomShaders/Screen_CameraWeights.usf", "MainPS", SF_Pixel);

// Screen Blending:
IMPLEMENT_GLOBAL_SHADER(FScreenBlendingVS, "/CustomShaders/Screen_Blending.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FScreenBlendingPS, "/CustomShaders/Screen_Blending.usf", "MainPS", SF_Pixel);

IMPLEMENT_GLOBAL_SHADER(FCompositeWithDepthVS, "/CustomShaders/Screen_CompositeWithDepth.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FCompositeWithDepthPS, "/CustomShaders/Screen_CompositeWithDepth.usf", "MainPS", SF_Pixel);