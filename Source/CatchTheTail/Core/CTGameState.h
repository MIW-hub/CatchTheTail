// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "../Type/CTTypes.h"
#include "CTMatchSettings.h"
#include "CTGameState.generated.h"

class ACTPlayerState;

// 로비 팀 변경 요청 (이동하려는 팀이 가득 찼을 때 모든 화면에 뜨는 요청). 서버 시간 기준 만료
USTRUCT()
struct FCTTeamSwapRequest
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<ACTPlayerState> Requester = nullptr;

	UPROPERTY()
	float ExpireServerTime = 0.f;
};

// 라운드 사이 결과 팝업에 표시할 개인 점수 한 줄
USTRUCT()
struct FCTRoundBreakEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	int32 TeamId = 0;

	UPROPERTY()
	int32 Score = 0;
};

// 라운드 사이 결과 팝업 내용 (서버가 정리 직전에 만들어 복제). 팀 번호 순서의 배열
USTRUCT()
struct FCTRoundBreakInfo
{
	GENERATED_BODY()

	UPROPERTY()
	int32 RoundNumber = 0;

	// 팀별 이번 라운드 에너지 합계 / 팀별 승점(이번 라운드 반영 전, 후)
	UPROPERTY()
	TArray<int32> TeamEnergy;

	UPROPERTY()
	TArray<int32> WinsBefore;

	UPROPERTY()
	TArray<int32> WinsAfter;

	UPROPERTY()
	TArray<FCTRoundBreakEntry> Entries;
};

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

	// 이 경기의 게임 모드 (경기 맵의 GameMode 가 정함)
	UFUNCTION(BlueprintPure, Category = "CT|Game")
	ECTGameModeType GetModeType() const { return ModeType; }

	// 팀전: 팀 색(팀 번호 0, 1)과 팀별 라운드 승점. 없는 번호는 None / 0
	UFUNCTION(BlueprintPure, Category = "CT|Game")
	ECTColor GetTeamColor(int32 TeamId) const { return TeamColors.IsValidIndex(TeamId) ? TeamColors[TeamId] : ECTColor::None; }

	UFUNCTION(BlueprintPure, Category = "CT|Game")
	int32 GetTeamRoundWins(int32 TeamId) const { return TeamRoundWins.IsValidIndex(TeamId) ? TeamRoundWins[TeamId] : 0; }

	// 보물찾기: 이번 라운드 팀별 에너지 합계 (두 팀 모두 모두에게 보임)
	UFUNCTION(BlueprintPure, Category = "CT|Game")
	int32 GetTeamScore(int32 TeamId) const { return TeamScores.IsValidIndex(TeamId) ? TeamScores[TeamId] : 0; }

	// 서버 전용 쓰기
	void SetModeType(ECTGameModeType NewModeType);
	void SetTeamColors(const TArray<ECTColor>& NewTeamColors);
	void SetTeamRoundWins(const TArray<int32>& NewTeamRoundWins);
	void SetTeamScores(const TArray<int32>& NewTeamScores);

	// 로비 팀 변경 요청 목록 (오래된 순). 남은 시간은 GetServerWorldTimeSeconds() 기준
	const TArray<FCTTeamSwapRequest>& GetTeamSwapRequests() const { return TeamSwapRequests; }
	void SetTeamSwapRequests(const TArray<FCTTeamSwapRequest>& NewRequests);

	// 로비: 방 코드, 없으면 빈 문자열
	UFUNCTION(BlueprintPure, Category = "CT|Lobby")
	FString GetRoomCode() const { return RoomCode; }

	// 로비: 필요한 최소 인원
	UFUNCTION(BlueprintPure, Category = "CT|Lobby")
	int32 GetLobbyMinPlayers() const { return LobbyMinPlayers; }

	// 로비: 게임에 채워질 AI 인원 (경기 설정의 AI 인원, 사람과 합쳐 최대 인원을 넘지 않음)
	UFUNCTION(BlueprintPure, Category = "CT|Lobby")
	int32 GetLobbyAIPlayerCount() const;

	// 로비: 사람 + AI 를 합친 참가 인원
	UFUNCTION(BlueprintPure, Category = "CT|Lobby")
	int32 GetLobbyParticipantCount() const;

	// 로비: 준비 완료한 인원 (방장 제외)
	UFUNCTION(BlueprintPure, Category = "CT|Lobby")
	int32 GetLobbyReadyCount() const;

	// 로비: 지금 시작할 수 있는가 (최소 인원 + 방장 제외 전원 준비)
	UFUNCTION(BlueprintPure, Category = "CT|Lobby")
	bool CanStartLobby() const;

	// 경기 중이고 기권이 열리는 시각이 지났는가
	UFUNCTION(BlueprintPure, Category = "CT|Game")
	bool CanForfeitNow() const;

	// 기권이 열릴 때까지 남은 초. 이미 열렸거나 경기 중이 아니면 0
	UFUNCTION(BlueprintPure, Category = "CT|Game")
	float GetForfeitUnlockRemainingSeconds() const;

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

	// 기권이 열리는 서버 시각
	void SetForfeitUnlockTime(double NewServerTime);

	// 서버: 라운드 사이 쉬는 시간 동안 경기 종료·기권 시각을 그만큼 늦춤 (쉬는 시간이 경기 시간을 깎지 않게)
	void ExtendGameTimes(float Seconds);

	// 서버: 라운드 사이 결과 팝업 내용과 끝나는 서버 시각 (SetPhase(RoundBreak) 보다 먼저 부름)
	void SetRoundBreak(const FCTRoundBreakInfo& NewInfo, double NewEndServerTime);
	const FCTRoundBreakInfo& GetRoundBreakInfo() const { return RoundBreakInfo; }
	UFUNCTION(BlueprintPure, Category = "CT|Game")
	float GetRoundBreakRemainingSeconds() const;

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

	// 기권이 열리는 서버 시각 (0이면 아직 정해지지 않음)
	UPROPERTY(Replicated)
	double ForfeitUnlockServerTime = 0.0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "CT|Game")
	float RoundDurationSeconds = 60.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "CT|Game")
	float CountdownSeconds = 5.f;

	// 게임 모드 (복제)
	UPROPERTY(Replicated)
	ECTGameModeType ModeType = ECTGameModeType::TailChase;

	// 팀 색 / 팀별 라운드 승점 (팀전 모드에서만 채움, 복제)
	UPROPERTY(Replicated)
	TArray<ECTColor> TeamColors;

	UPROPERTY(Replicated)
	TArray<int32> TeamRoundWins;

	UPROPERTY(Replicated)
	TArray<int32> TeamScores;

	UPROPERTY(Replicated)
	TArray<FCTTeamSwapRequest> TeamSwapRequests;

	UPROPERTY(Replicated)
	FCTRoundBreakInfo RoundBreakInfo;

	UPROPERTY(Replicated)
	double RoundBreakEndServerTime = 0.0;

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
