// Copyright Moon Party Games. All Rights Reserved.
#include "CTMenuGameMode.h"
#include "../Player/CTMenuPlayerController.h"
#include "GameFramework/HUD.h"

ACTMenuGameMode::ACTMenuGameMode()
{
	// 메뉴에 캐릭터 불필요
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
	PlayerControllerClass = ACTMenuPlayerController::StaticClass();
}
