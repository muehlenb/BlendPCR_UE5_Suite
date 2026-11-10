// Copyright Epic Games, Inc. All Rights Reserved.

#include "BlendPCVRGameMode.h"
#include "BlendPCVRCharacter.h"
#include "UObject/ConstructorHelpers.h"

ABlendPCVRGameMode::ABlendPCVRGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
