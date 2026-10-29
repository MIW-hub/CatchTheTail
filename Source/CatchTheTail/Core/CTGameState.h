// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "../Type/CTTypes.h"
#include "CTMatchSettings.h"
#include "CTGameState.generated.h"

class ACTPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCTPhaseChangedSignature, ECTGamePhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCTRoundChangedSignature, int32, NewRoundNumber);

UCLASS()
class CATCHTHETAIL_API ACTGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ACTGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 읽기
	UFUNCTION(BlueprintPure, Category = "CT|Game")
	ECTGamePhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "CT|Game")
	int32 GetRoundNumber() const { return RoundNumber; }

	// 라운드 남은 초
	UFUNCTION(BlueprintPure, Category = "CT|Game")
	float GetRoundRemainingSeconds() const;

	// 게임 남은 초
	UFUNCTION(BlueprintPure, Category = "CT|Game")
	float GetGameRemainingSeconds() const;

	// 색 변경 직전 카운트다운 중인가
	UFUNCTION(BlueprintPure, Category = "CT|Game")
	bool IsCountdownActive() const;

	// 카운트다운 숫자. 아니면 0
	UFUNCTION(BlueprintPure, Category = "CT|Game")
	int32 GetCountdownNumber() const;

	// 시작 전 카운트다운 중인가
	UFUNCTION(BlueprintPure, Category = "CT|Game")
	bool IsStartCountdown() const { return Phase == ECTGamePhase::Countdown; }

	// 시작까지 남은 초. 아니면 0
	UFUNCTION(BlueprintPure, Category = "CT|Game")
	float GetStartCountdownRemainingSeconds() const;

	// 로비: 시작까지 남은 초. 시작 전이면 0
	UFUNCTION(BlueprintPure, Category = "CT|Lobby")
	float GetLobbyStartRemainingSeconds() const;

	// 종료 후 로비 자동 복귀까지 남은 초, 예약 없으면 0
	UFUNCTION(BlueprintPure, Category = "CT|Result")
	float GetReturnToLobbyRemainingSeconds() const;

	// 로비: 선택 맵 번호, 없으면 -1
	UFUNCTION(BlueprintPure, Category = "CT|Lobby")
	int32 GetSelectedMapIndex() const { return SelectedMapIndex; }

	// 로비: 방장이 정한 경기 설정 (모두에게 복제)
	UFUNCTION(BlueprintPure, Category = "CT|Lobby")
	const FCTMatchSettings& GetMatchSettings() const { return MatchSettings; }

	// 로비: 방 코드, 없으면 빈 문자열
	UFUNCTION(BlueprintPure, Category = "CT|Lobby")
	FString GetRoomCode() const { return RoomCode; }

	// 로비: 필요한 최소 인원
	UFUNCTION(BlueprintPure, Category = "CT|Lobby")
	int32 GetLobbyMinPlayers() const { return LobbyMinPlayers; }

	// 로비: 준비 완료한 인원 (방장 제외)
	UFUNCTION(BlueprintPure, Category = "CT|Lobby")
	int32 GetLobbyReadyCount() const;

	// 로비: 지금 시작할 수 있는가 (최소 인원 + 방장 제외 전원 준비)
	UFUNCTION(BlueprintPure, Category = "CT|Lobby")
	bool CanStartLobby() const;

	double GetRoundEndServerTime() const { return RoundEndServerTime; }
	double GetGameEndServerTime() const { return GameEndServerTime; }

	// 승자 목록 (꼬리 최다). 결과 화면용
	UFUNCTION(BlueprintPure, Category = "CT|Result")
	TArray<ACTPlayerState*> GetWinners() const;

	// 승자 2명 이상 (연장전)
	UFUNCTION(BlueprintPure, Category = "CT|Result")
	bool IsTie() const { return bIsTie; }

	UFUNCTION(BlueprintPure, Category = "CT|Result")
	bool IsWinner(const APlayerState* PlayerState) const;

	// 쓰기 (서버 전용)
	void SetPhase(ECTGamePhase NewPhase);

	// SetPhase(Countdown)보다 먼저 호출
	void SetStartCountdown(double NewStartServerTime);
	void SetLobbyMinPlayers(int32 NewMinPlayers);
	void SetRoomCode(const FString& NewRoomCode);
	void SetSelectedMapIndex(int32 NewMapIndex);
	void SetMatchSettings(const FCTMatchSettings& NewSettings);
	// 0이면 예약 없음
	void SetReturnToLobbyTime(double NewServerTime);
	void SetRoundInfo(int32 NewRoundNumber, double NewRoundEndServerTime);
	void SetGameInfo(double NewGameEndServerTime, float NewRoundDurationSeconds, float NewCountdownSeconds);

	// SetPhase(Finished)보다 먼저 호출
	void SetGameResult(const TArray<ACTPlayerState*>& NewWinners, bool bNewIsTie);

	UPROPERTY(BlueprintAssignable, Category = "CT|Game")
	FCTPhaseChangedSignature OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "CT|Game")
	FCTRoundChangedSignature OnRoundChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Phase, BlueprintReadOnly, Category = "CT|Game")
	ECTGamePhase Phase = ECTGamePhase::WaitingForPlayers;

	UPROPERTY(ReplicatedUsing = OnRep_RoundNumber, BlueprintReadOnly, Category = "CT|Game")
	int32 RoundNumber = 0;

	// 경기 시작 서버 시각
	UPROPERTY(Replicated)
	double StartServerTime = 0.0;

	// 로비 자동 복귀 서버 시각 (0이면 없음)
	UPROPERTY(Replicated)
	double ReturnToLobbyServerTime = 0.0;

	// 로비에서 선택된 맵 번호
	UPROPERTY(Replicated)
	int32 SelectedMapIndex = INDEX_NONE;

	// 로비 경기 설정
	UPROPERTY(Replicated)
	FCTMatchSettings MatchSettings;

	// 로비 방 코드
	UPROPERTY(Replicated)
	FString RoomCode;

	// 로비 시작에 필요한 최소 인원
	UPROPERTY(Replicated)
	int32 LobbyMinPlayers = 3;

	// 라운드 종료 서버 시각
	UPROPERTY(Replicated)
	double RoundEndServerTime = 0.0;

	// 게임 종료 서버 시각
	UPROPERTY(Replicated)
	double GameEndServerTime = 0.0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "CT|Game")
	float RoundDurationSeconds = 60.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "CT|Game")
	float CountdownSeconds = 5.f;

	// 결과: 승자
	UPROPERTY(Replicated)
	TArray<TObjectPtr<ACTPlayerState>> Winners;

	// 결과: 동점
	UPROPERTY(Replicated)
	bool bIsTie = false;

	UFUNCTION()
	void OnRep_Phase();

	UFUNCTION()
	void OnRep_RoundNumber();
};
