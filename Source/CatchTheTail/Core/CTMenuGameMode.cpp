// Copyright Moon Party Games. All Rights Reserved.
#include "CTMenuGameMode.h"
#include "../Player/CTMenuPlayerController.h"
#include "GameFramework/HUD.h"

ACTMenuGameMode::ACTMenuGameMode()
{
	// 메뉴에는 캐릭터가 필요 없습니다.
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
	PlayerControllerClass = ACTMenuPlayerController::StaticClass();
}
