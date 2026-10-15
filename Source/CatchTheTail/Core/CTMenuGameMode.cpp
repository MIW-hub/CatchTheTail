// Copyright Moon Party Games. All Rights Reserved.
// [보류 중] 켜는 방법은 CTGameInstance.h 맨 위 설명을 보세요.
#if 0
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
#endif // 보류: 메인 메뉴 / 방 만들기
