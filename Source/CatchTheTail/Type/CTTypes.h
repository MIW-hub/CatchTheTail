// Copyright Moon Party Games. All Rights Reserved.
// 공용 타입 모음: 색, 게임 단계, 달리기 상태, 속도 버프
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "CTTypes.generated.h"

/** 사용하는 색의 개수 (사업계획서 기준 8색) */
inline constexpr int32 CTColorCount = 8;

/** 플레이어 색. 숫자 순서가 곧 "색 순서"입니다. (목표 색 계산에 사용) */
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
	None   = 255 UMETA(DisplayName = "None")   // 아직 색이 없음
};

/** 게임 진행 단계 */
UENUM(BlueprintType)
enum class ECTGamePhase : uint8
{
	WaitingForPlayers UMETA(DisplayName = "Waiting"),
	Playing           UMETA(DisplayName = "Playing"),
	Finished          UMETA(DisplayName = "Finished"),
	Countdown         UMETA(DisplayName = "Countdown") // 경기 시작 전 카운트다운
};

/** 달리기 상태 (UI의 쿨타임 표시에 사용) */
UENUM(BlueprintType)
enum class ECTRunState : uint8
{
	Idle     UMETA(DisplayName = "Idle"),
	Running  UMETA(DisplayName = "Running"),
	Cooldown UMETA(DisplayName = "Cooldown")
};

/** 캐릭터 행동 연출 종류 (몽타주 재생, 이펙트 연결에 사용) */
UENUM(BlueprintType)
enum class ECTCharacterAction : uint8
{
	Grab   UMETA(DisplayName = "Grab"),   // 잡기 (왼손)
	Push   UMETA(DisplayName = "Push"),   // 밀기 (오른손)
	Caught UMETA(DisplayName = "Caught"), // 꼬리를 잡힘
	Pushed UMETA(DisplayName = "Pushed")  // 밀려남
};

/** 속도 버프 하나. 최종 배율 = 1.0 + (모든 버프 값의 합) */
USTRUCT(BlueprintType)
struct FCTSpeedBuff
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CT|Speed")
	FName Id = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "CT|Speed")
	float Value = 0.f;
};
