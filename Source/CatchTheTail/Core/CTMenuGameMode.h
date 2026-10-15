// Copyright Moon Party Games. All Rights Reserved.
// [보류 중] 켜는 방법은 CTGameInstance.h 맨 위 설명을 보세요.
#if 0
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CTMenuGameMode.generated.h"

/**
 * 메인 메뉴 맵(L_MainMenu)에서 쓰는 게임 모드.
 * 캐릭터 없이 메뉴 화면만 보여 주는 용도입니다.
 * (L_MainMenu의 월드 세팅 → GameMode Override에 이 클래스를 지정하세요)
 */
UCLASS()
class CATCHTHETAIL_API ACTMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACTMenuGameMode();
};
#endif // 보류: 메인 메뉴 / 방 만들기
