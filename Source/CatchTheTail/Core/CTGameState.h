// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "../Type/CTTypes.h"
#include "CTGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCTPhaseChangedSignature, ECTGamePhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCTRoundChangedSignature, int32, NewRoundNumber);

/**
 * 모두가 같이 보는 게임 정보: 단계, 라운드 번호, 남은 시간.
 *
 * 남은 시간은 "끝나는 시각"만 복제하고, 남은 초는 각자 계산합니다.
 * (매초 복제하지 않아도 되고, 모두 같은 시간을 보게 됩니다.)
 */
UCLASS()
class CATCHTHETAIL_API ACTGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ACTGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ----- 읽기 -----
	UFUNCTION(BlueprintPure, Category = "CT|Game")
	ECTGamePhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "CT|Game")
	int32 GetRoundNumber() const { return RoundNumber; }

	/** 현재 라운드 남은 초 */
	UFUNCTION(BlueprintPure, Category = "CT|Game")
	float GetRoundRemainingSeconds() const;

	/** 전체 게임 남은 초 */
	UFUNCTION(BlueprintPure, Category = "CT|Game")
	float GetGameRemainingSeconds() const;

	/** 색이 바뀌기 직전 카운트다운 중인가? (기본 5초 전부터) */
	UFUNCTION(BlueprintPure, Category = "CT|Game")
	bool IsCountdownActive() const;

	/** 카운트다운 숫자 (5, 4, 3, 2, 1). 카운트다운이 아니면 0 */
	UFUNCTION(BlueprintPure, Category = "CT|Game")
	int32 GetCountdownNumber() const;

	double GetRoundEndServerTime() const { return RoundEndServerTime; }
	double GetGameEndServerTime() const { return GameEndServerTime; }

	// ----- 쓰기 (서버에서만 동작, GameMode가 호출) -----
	void SetPhase(ECTGamePhase NewPhase);
	void SetRoundInfo(int32 NewRoundNumber, double NewRoundEndServerTime);
	void SetGameInfo(double NewGameEndServerTime, float NewRoundDurationSeconds, float NewCountdownSeconds);

	UPROPERTY(BlueprintAssignable, Category = "CT|Game")
	FCTPhaseChangedSignature OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "CT|Game")
	FCTRoundChangedSignature OnRoundChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Phase, BlueprintReadOnly, Category = "CT|Game")
	ECTGamePhase Phase = ECTGamePhase::WaitingForPlayers;

	UPROPERTY(ReplicatedUsing = OnRep_RoundNumber, BlueprintReadOnly, Category = "CT|Game")
	int32 RoundNumber = 0;

	/** 현재 라운드가 끝나는 서버 시각 */
	UPROPERTY(Replicated)
	double RoundEndServerTime = 0.0;

	/** 전체 게임이 끝나는 서버 시각 */
	UPROPERTY(Replicated)
	double GameEndServerTime = 0.0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "CT|Game")
	float RoundDurationSeconds = 60.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "CT|Game")
	float CountdownSeconds = 5.f;

	UFUNCTION()
	void OnRep_Phase();

	UFUNCTION()
	void OnRep_RoundNumber();
};
