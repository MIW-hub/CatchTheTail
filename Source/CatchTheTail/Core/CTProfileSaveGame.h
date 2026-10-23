// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CTProfileSaveGame.generated.h"

UCLASS()
class CATCHTHETAIL_API UCTProfileSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// 메인 메뉴 전에 입력한 이름
	UPROPERTY()
	FString PlayerName;
};
