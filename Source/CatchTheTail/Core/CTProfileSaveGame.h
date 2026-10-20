// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CTProfileSaveGame.generated.h"

/**
 * 이 컴퓨터에 저장되는 플레이어 프로필.
 * 지금은 이름만 저장합니다. (Saved/SaveGames/CTProfile.sav)
 */
UCLASS()
class CATCHTHETAIL_API UCTProfileSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** 메인 메뉴 전에 입력해서 저장한 이름 */
	UPROPERTY()
	FString PlayerName;
};
