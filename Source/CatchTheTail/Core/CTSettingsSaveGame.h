// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "InputCoreTypes.h"
#include "CTSettingsSaveGame.generated.h"

/** 설정 창에서 바꾸는 값들. 이 컴퓨터에 저장됩니다. */
USTRUCT(BlueprintType)
struct FCTSettingsData
{
	GENERATED_BODY()

	// ----- 소리 -----
	/** 배경음 볼륨 (0~1) */
	UPROPERTY(BlueprintReadWrite, Category = "CT|Settings")
	float MusicVolume = 0.7f;

	/** 효과음 볼륨 (0~1) */
	UPROPERTY(BlueprintReadWrite, Category = "CT|Settings")
	float SfxVolume = 1.0f;

	/** 모든 소리 끄기 */
	UPROPERTY(BlueprintReadWrite, Category = "CT|Settings")
	bool bMuteAll = false;

	// ----- 화면 -----
	UPROPERTY(BlueprintReadWrite, Category = "CT|Settings")
	bool bFullscreen = true;

	/** 화면 흔들림 (끄면 카메라 흔들림 연출이 나오지 않음) */
	UPROPERTY(BlueprintReadWrite, Category = "CT|Settings")
	bool bScreenShake = true;

	// ----- 접근성 -----
	/** 색 구분 보조 (색 이름과 도형을 함께 표시) */
	UPROPERTY(BlueprintReadWrite, Category = "CT|Settings")
	bool bTeamColorAssist = false;

	/** 캐릭터 머리 위에 팀/색 표시 */
	UPROPERTY(BlueprintReadWrite, Category = "CT|Settings")
	bool bShowTeamMarker = true;

	// ----- 조작 -----
	/** 바꾼 키. 키는 행동 이름(Id)별로 저장합니다. (바꾸지 않은 행동은 들어 있지 않음) */
	UPROPERTY()
	TMap<FName, FKey> KeyOverrides;
};

/** 설정을 저장하는 세이브 데이터 (Saved/SaveGames/CTSettings.sav) */
UCLASS()
class CATCHTHETAIL_API UCTSettingsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FCTSettingsData Data;
};
