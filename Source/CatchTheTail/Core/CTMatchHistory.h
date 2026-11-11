// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CTMatchHistory.generated.h"

// 한 판의 순위표 한 줄 (1~3등만 저장)
USTRUCT(BlueprintType)
struct FCTMatchRecordEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "CT|History")
	FString PlayerName;

	UPROPERTY(BlueprintReadWrite, Category = "CT|History")
	int32 Rank = 0;

	UPROPERTY(BlueprintReadWrite, Category = "CT|History")
	int32 TailCount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "CT|History")
	bool bIsMe = false;
};

// 한 판의 전적
USTRUCT(BlueprintType)
struct FCTMatchRecord
{
	GENERATED_BODY()

	// 끝난 시각 (이 컴퓨터 시계)
	UPROPERTY(BlueprintReadWrite, Category = "CT|History")
	FDateTime Time;

	// 내 등수 (공동이면 같은 등수)
	UPROPERTY(BlueprintReadWrite, Category = "CT|History")
	int32 MyRank = 0;

	UPROPERTY(BlueprintReadWrite, Category = "CT|History")
	int32 MyTailCount = 0;

	// 그 판 맵 이름
	UPROPERTY(BlueprintReadWrite, Category = "CT|History")
	FString MapName;

	// 그 판 참가 인원
	UPROPERTY(BlueprintReadWrite, Category = "CT|History")
	int32 PlayerCount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "CT|History")
	TArray<FCTMatchRecordEntry> Entries;
};

// 방 전적 보관. 게임 메모리에만 두며 방을 나가면 지움 (방 전적이지 내 전적이 아님). 최근 판이 앞에 옴
UCLASS()
class CATCHTHETAIL_API UCTMatchHistory : public UObject
{
	GENERATED_BODY()

public:
	static TArray<FCTMatchRecord> LoadRecords();
	static void AddRecord(const FCTMatchRecord& Record);

	// 방을 나갈 때 비움
	static void ClearRecords();

	static constexpr int32 MaxRecords = 100;
};
