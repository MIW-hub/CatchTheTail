// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "InputCoreTypes.h"
#include "CTSettingsSaveGame.generated.h"

// 설정 창 값. 로컬 저장
USTRUCT(BlueprintType)
struct FCTSettingsData
{
	GENERATED_BODY()

	// 소리
	// 배경음 볼륨 0~1
	UPROPERTY(BlueprintReadWrite, Category = "CT|Settings")
	float MusicVolume = 0.7f;

	// 효과음 볼륨 0~1
	UPROPERTY(BlueprintReadWrite, Category = "CT|Settings")
	float SfxVolume = 1.0f;

	// 전체 음소거
	UPROPERTY(BlueprintReadWrite, Category = "CT|Settings")
	bool bMuteAll = false;

	// 언어
	// 문화 코드 (ko, en, ja). 비어 있으면 처음 실행으로 보고 영어
	UPROPERTY(BlueprintReadWrite, Category = "CT|Settings")
	FString Language;

	// 언어 선택 화면에서 직접 고른 적이 있는가. false면 처음 한 번 언어 선택 화면을 보여줌
	UPROPERTY(BlueprintReadWrite, Category = "CT|Settings")
	bool bLanguageChosen = false;

	// 화면
	UPROPERTY(BlueprintReadWrite, Category = "CT|Settings")
	bool bFullscreen = true;

	// 끄면 카메라 흔들림 없음
	UPROPERTY(BlueprintReadWrite, Category = "CT|Settings")
	bool bScreenShake = true;

	// 접근성
	// 색 이름 + 도형 표시
	UPROPERTY(BlueprintReadWrite, Category = "CT|Settings")
	bool bTeamColorAssist = false;

	// 머리 위 팀/색 표시
	UPROPERTY(BlueprintReadWrite, Category = "CT|Settings")
	bool bShowTeamMarker = true;

	// 조작
	// 바꾼 키. 행동 Id별, 안 바꾼 행동은 없음
	UPROPERTY()
	TMap<FName, FKey> KeyOverrides;
};

// 설정 세이브 (Saved/SaveGames/CTSettings.sav)
UCLASS()
class CATCHTHETAIL_API UCTSettingsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FCTSettingsData Data;
};
