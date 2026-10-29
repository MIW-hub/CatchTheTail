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

/**
 * 경기 설정 값 모음. 로비의 GameState에 복제되어 모두에게 보이고, 게임이 시작되면 GameMode가 읽어 적용합니다.
 * 항목마다 고를 수 있는 값(선택지)이 정해져 있어서, 화면의 < > 버튼은 선택지를 한 칸씩 이동합니다.
 */
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

	// 모든 값을 선택지 중 가장 가까운 값으로 맞춤
	void Sanitize();

	// 선택지를 한 칸 이동 (Direction -1 / +1). 끝이면 그대로. 값이 바뀌면 true
	bool Step(ECTMatchSetting Setting, int32 Direction);

	// 이 방향으로 더 이동할 수 있는가
	bool CanStep(ECTMatchSetting Setting, int32 Direction) const;

	// 지금 방 인원으로 허용되는 설정인가 (이미 들어온 인원보다 적은 최대 인원, 경기 시간보다 긴 라운드는 불가)
	bool IsAllowed(int32 NumPlayers) const;

	bool IsBushEnabled() const { return Bush != 0; }

	// 항목의 선택지 (오름차순)
	static const TArray<int32>& GetOptions(ECTMatchSetting Setting);

	// 위젯 이름에 쓰는 항목 이름 (예: Button_Setting_GameMinutes_Prev)
	static FName GetKey(ECTMatchSetting Setting);

	// 화면에 보이는 항목 이름 / 값
	static FText GetLabel(ECTMatchSetting Setting);
	static FText FormatValue(ECTMatchSetting Setting, int32 Value);

	// 로비 화면 요약 한 줄 (예: "5분 · 라운드 60초 · 최대 8명")
	FText MakeSummary() const;
};
