// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CTMatchSettings.generated.h"

// 게임 모드 (로비에서 방장이 고름). 값은 FCTMatchSettings::Mode 에 숫자로 저장
UENUM(BlueprintType)
enum class ECTGameModeType : uint8
{
	TailChase    UMETA(DisplayName = "Tail Chase"),     // 꼬리잡기 (기본)
	TreasureHunt UMETA(DisplayName = "Treasure Hunt"),  // 보물찾기 (팀전, 개발 중)
	Count        UMETA(Hidden)
};

// 방장이 로비에서 정하는 경기 설정 항목
UENUM(BlueprintType)
enum class ECTMatchSetting : uint8
{
	GameMinutes   UMETA(DisplayName = "Game Minutes"),    // 전체 경기 시간(분)
	RoundSeconds  UMETA(DisplayName = "Round Seconds"),   // 색이 바뀌는 라운드 길이(초)
	MaxPlayers    UMETA(DisplayName = "Max Players"),     // 방 최대 인원
	InitialTails  UMETA(DisplayName = "Initial Tails"),   // 시작 꼬리 수
	PhoneUses     UMETA(DisplayName = "Phone Uses"),      // 전화 재사용 대기 시간(초). 0이면 사용 안 함. 로비 위젯 이름 때문에 이전 이름을 유지
	Bush          UMETA(DisplayName = "Bush"),            // 부쉬 기믹 (0 끄기, 1 켜기)
	AiPlayers     UMETA(DisplayName = "AI Players"),      // AI 플레이어 수. 0이면 없음. 사람과 합쳐 최대 인원을 넘지 않음
	Mode          UMETA(DisplayName = "Game Mode"),       // 게임 모드 (ECTGameModeType 값). 번호가 밀리지 않게 맨 끝에 둠
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

	// 전화 재사용 대기 시간(초), 0이면 사용 안 함
	UPROPERTY(BlueprintReadOnly, Category = "CT|Match")
	int32 PhoneUses = 30;

	// 부쉬 사용 (0/1)
	UPROPERTY(BlueprintReadOnly, Category = "CT|Match")
	int32 Bush = 1;

	// AI 플레이어 수 (0이면 없음)
	UPROPERTY(BlueprintReadOnly, Category = "CT|Match")
	int32 AiPlayers = 0;

	// 게임 모드 (ECTGameModeType 값, 기본 0 = 꼬리잡기)
	UPROPERTY(BlueprintReadOnly, Category = "CT|Match")
	int32 Mode = 0;

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

	ECTGameModeType GetMode() const { return Mode == static_cast<int32>(ECTGameModeType::TreasureHunt) ? ECTGameModeType::TreasureHunt : ECTGameModeType::TailChase; }

	// 지금 실제로 시작할 수 있는 모드인가 (보물찾기는 개발 중이라 아직 막음)
	bool IsModePlayable() const { return GetMode() == ECTGameModeType::TailChase; }

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
