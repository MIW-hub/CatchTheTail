// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CTMatchSettings.generated.h"

// 방장이 로비에서 정하는 경기 설정 항목
UENUM(BlueprintType)
enum class ECTMatchSetting : uint8
{
	GameMinutes   UMETA(DisplayName = "Game Minutes"),    // 전체 경기 시간(분)
	RoundSeconds  UMETA(DisplayName = "Round Seconds"),   // 색이 바뀌는 라운드 길이(초)
	MaxPlayers    UMETA(DisplayName = "Max Players"),     // 방 최대 인원
	InitialTails  UMETA(DisplayName = "Initial Tails"),   // 시작 꼬리 수
	PhoneUses     UMETA(DisplayName = "Phone Uses"),      // 전화부스 사용 횟수 (0이면 사용 안 함)
	Bush          UMETA(DisplayName = "Bush"),            // 부쉬 기믹 (0 끄기, 1 켜기)
	Count         UMETA(Hidden)
};

// 경기 설정 값 모음. GameState로 복제
USTRUCT(BlueprintType)
struct CATCHTHETAIL_API FCTMatchSettings
{
	GENERATED_BODY()

	// 전체 경기 시간(분)
	UPROPERTY(BlueprintReadOnly, Category = "CT|Match")
	int32 GameMinutes = 5;

	// 라운드 길이(초)
	UPROPERTY(BlueprintReadOnly, Category = "CT|Match")
	int32 RoundSeconds = 60;

	// 최대 인원
	UPROPERTY(BlueprintReadOnly, Category = "CT|Match")
	int32 MaxPlayers = 8;

	// 시작 꼬리 수
	UPROPERTY(BlueprintReadOnly, Category = "CT|Match")
	int32 InitialTails = 1;

	// 전화부스 사용 횟수
	UPROPERTY(BlueprintReadOnly, Category = "CT|Match")
	int32 PhoneUses = 2;

	// 부쉬 사용 (0/1)
	UPROPERTY(BlueprintReadOnly, Category = "CT|Match")
	int32 Bush = 1;

	int32 Get(ECTMatchSetting Setting) const;
	void Set(ECTMatchSetting Setting, int32 NewValue);

	// 모든 값을 가까운 선택지로 보정
	void Sanitize();

	// 선택지 한 칸 이동. 값이 바뀌면 true
	bool Step(ECTMatchSetting Setting, int32 Direction);

	// 해당 방향으로 이동 가능한가
	bool CanStep(ECTMatchSetting Setting, int32 Direction) const;

	// 현재 인원에서 허용되는 설정인가
	bool IsAllowed(int32 NumPlayers) const;

	bool IsBushEnabled() const { return Bush != 0; }

	// 항목의 선택지 (오름차순)
	static const TArray<int32>& GetOptions(ECTMatchSetting Setting);

	// 위젯 이름용 항목 이름
	static FName GetKey(ECTMatchSetting Setting);

	// 표시용 항목 이름 / 값
	static FText GetLabel(ECTMatchSetting Setting);
	static FText FormatValue(ECTMatchSetting Setting, int32 Value);

	// 로비 요약 한 줄
	FText MakeSummary() const;
};
