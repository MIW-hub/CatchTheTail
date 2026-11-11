// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "InputCoreTypes.h"
#include "CTSettingsSaveGame.generated.h"

// 목소리 볼륨 슬라이더 100%일 때의 배율 (슬라이더 50%가 예전 기본 크기)
constexpr float CTVoiceMaxGain = 2.0f;

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
	// 문화 코드. 비면 처음 실행(영어)
	UPROPERTY(BlueprintReadWrite, Category = "CT|Settings")
	FString Language;

	// 언어 선택 화면을 거쳤는가
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

	// 음성 채팅
	// 마이크로 말하기 (끄면 눌러도 전송 안 함)
	UPROPERTY(BlueprintReadWrite, Category = "CT|Settings")
	bool bMicEnabled = true;

	// 버튼을 누르지 않아도 계속 말하기 (켜면 눌러서 말하기 없이 상시 전송)
	UPROPERTY(BlueprintReadWrite, Category = "CT|Settings")
	bool bMicAlwaysOn = false;

	// 다른 사람 목소리 볼륨 0~1 (슬라이더 값). 실제 볼륨 = 값 x CTVoiceMaxGain, 기본 0.5가 예전 100%와 같은 크기
	UPROPERTY(BlueprintReadWrite, Category = "CT|Settings")
	float VoiceVolume = 0.5f;

	// 목소리 볼륨 척도 버전. 0이면 예전 척도(슬라이더 1 = 원래 크기)라서 불러올 때 기본값으로 바꿈
	UPROPERTY()
	int32 VoiceScaleVersion = 0;

	// 마이크 감도 0~1. 클수록 작은 소리도 보냄
	UPROPERTY(BlueprintReadWrite, Category = "CT|Settings")
	float MicSensitivity = 0.9f;

	// 이모트: 숫자 키 1, 2, 3...에 등록한 이모트 목록 번호. 비면 기본값
	UPROPERTY()
	TArray<int32> EmoteLoadout;

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
