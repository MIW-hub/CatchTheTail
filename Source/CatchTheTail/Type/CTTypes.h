// Copyright Moon Party Games. All Rights Reserved.
// 공용 타입 모음: 색, 게임 단계, 달리기 상태, 속도 버프
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "CTTypes.generated.h"

// 색 개수
inline constexpr int32 CTColorCount = 8;

// 플레이어 색. 값 순서 = 목표 색 순서
UENUM(BlueprintType)
enum class ECTColor : uint8
{
	Red    = 0  UMETA(DisplayName = "Red"),    // 빨강 #E53935
	Orange = 1  UMETA(DisplayName = "Orange"), // 주황 #FB8C00
	Yellow = 2  UMETA(DisplayName = "Yellow"), // 노랑 #FDD835
	Green  = 3  UMETA(DisplayName = "Green"),  // 초록 #43A047
	Sky    = 4  UMETA(DisplayName = "Sky"),    // 하늘 #29B6F6
	Blue   = 5  UMETA(DisplayName = "Blue"),   // 파랑 #2B3BFF
	Purple = 6  UMETA(DisplayName = "Purple"), // 보라 #8E24AA
	Pink   = 7  UMETA(DisplayName = "Pink"),   // 분홍 #FF4FA3
	None   = 255 UMETA(DisplayName = "None")   // 색 없음
};

// 게임 단계
UENUM(BlueprintType)
enum class ECTGamePhase : uint8
{
	WaitingForPlayers UMETA(DisplayName = "Waiting"),
	Playing           UMETA(DisplayName = "Playing"),
	Finished          UMETA(DisplayName = "Finished"),
	Countdown         UMETA(DisplayName = "Countdown"), // 시작 전 카운트다운
	Lobby             UMETA(DisplayName = "Lobby")     // 대기 로비
};

// 달리기 상태
UENUM(BlueprintType)
enum class ECTRunState : uint8
{
	Idle     UMETA(DisplayName = "Idle"),
	Running  UMETA(DisplayName = "Running"),
	Cooldown UMETA(DisplayName = "Cooldown")
};

// 캐릭터 행동 연출
UENUM(BlueprintType)
enum class ECTCharacterAction : uint8
{
	Grab   UMETA(DisplayName = "Grab"),   // 잡기 (왼손)
	Push   UMETA(DisplayName = "Push"),   // 밀기 (오른손)
	Caught UMETA(DisplayName = "Caught"), // 꼬리를 잡힘
	Pushed UMETA(DisplayName = "Pushed")  // 밀려남
};

// 속도 버프. 배율 = 1 + 값의 합
USTRUCT(BlueprintType)
struct FCTSpeedBuff
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CT|Speed")
	FName Id = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "CT|Speed")
	float Value = 0.f;
};

// 서버가 입장을 거절할 때 보내는 사유 코드. 클라이언트가 이 코드를 보고 안내 문구를 고릅니다.
namespace CTJoinReject
{
	inline constexpr const TCHAR* GameInProgress = TEXT("CT_GAME_IN_PROGRESS");
	inline constexpr const TCHAR* LobbyStarting  = TEXT("CT_LOBBY_STARTING");
	inline constexpr const TCHAR* LobbyFull      = TEXT("CT_LOBBY_FULL");
}
