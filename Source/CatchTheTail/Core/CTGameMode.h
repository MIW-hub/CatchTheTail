// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/OnlineReplStructs.h"
#include "../Type/CTTypes.h"
#include "CTMatchSettings.h"
#include "CTGameMode.generated.h"

class ACTPlayerState;
class ACTGameState;
class UCTGameRule;
class ACTCharacter;

UCLASS()
class CATCHTHETAIL_API ACTGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACTGameMode();

	// 로비에서 정한 경기 설정 적용
	virtual void InitGameState() override;

	// 진행 중이면 새 입장 거절
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	// 이탈 시 목표 색 정리, 혼자 남으면 종료
	virtual void Logout(AController* Exiting) override;
	// Seamless Travel 플레이어는 PostLogin이 없어 여기서 초기화
	virtual void HandleSeamlessTravelPlayer(AController*& C) override;

	// 로비 시작 버튼에서 호출
	UFUNCTION(BlueprintCallable, Category = "CT|Game")
	void StartGame();

	// 전화 재사용 대기(초), 0이면 전화 사용 불가
	float GetPhoneCooldownSeconds() const { return PhoneCooldownSeconds; }

	// 시간 종료 시 자동 호출
	UFUNCTION(BlueprintCallable, Category = "CT|Game")
	void EndGame();

	// 서버, 방장 로비 복귀 요청 (종료 후만)
	bool RequestReturnToLobby(APlayerController* Requester);

	// 서버, 꼬리를 잡음. 대결(F 연타)이 켜져 있으면 대결을 시작하고, 꺼져 있으면 바로 FinalizeTailGrabbed
	bool HandleTailGrabbed(ACTPlayerState* Grabber, ACTPlayerState* Victim);

	// 서버, 점수 변경 + 유령 처리 (꼬리를 실제로 뺏음). 대결에서 잡은 쪽이 이기면 호출됨
	bool FinalizeTailGrabbed(ACTPlayerState* Grabber, ACTPlayerState* Victim);

	// 서버, 캐릭터가 에너지를 먹음 (보물찾기). 규칙이 처리하면 true
	bool HandleEnergyPickup(ACTCharacter* Picker, int32 Amount);

	// 서버, 기권 요청. 경기 중이고 기권이 열린 뒤에만 가능. 꼬리를 잃고 순위에서 빠져 유령으로 남음
	bool HandleForfeit(ACTPlayerState* Forfeiter);

	// 서버, 유령이 F키로 무작위 PlayerStart 로 이동. 유령 상태마다 한 번만 가능하며 유령 상태는 그대로이고 부활은 다음 라운드 시작 때
	bool HandleGhostRelocate(ACTPlayerState* Ghost);

	// 블루프린트 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Events")
	void OnGameStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Events")
	void OnRoundStarted(int32 RoundNumber);

	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Events")
	void OnTailGrabbed(ACTPlayerState* Grabber, ACTPlayerState* Victim);

	// 게임 종료. bIsTie면 동점(연장전)
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Events")
	void OnGameFinished(const TArray<ACTPlayerState*>& Winners, bool bIsTie);

protected:
	// 설정
	// 전체 게임 시간(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Game", meta = (ClampMin = "10"))
	float GameDurationSeconds = 300.f;

	// 라운드 길이(초). 끝나면 색 재배정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Game", meta = (ClampMin = "5"))
	float RoundDurationSeconds = 60.f;

	// 색 변경 전 카운트다운 시간(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Game", meta = (ClampMin = "0"))
	float CountdownSeconds = 5.f;

	// 시작 꼬리 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Game", meta = (ClampMin = "0"))
	int32 InitialTailCount = 1;

	// 전화를 한 번 건 뒤 다시 걸기까지의 대기 시간(초, 플레이어마다). 0이면 전화 사용 불가
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Gimmick", meta = (ClampMin = "0"))
	float PhoneCooldownSeconds = 30.f;

	// 꼬리가 이 수보다 적으면 새 라운드에 채움, 0이면 안 채움
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Game", meta = (ClampMin = "0"))
	int32 MinTailCountAtRoundStart = 1;

	// 꼬리를 잡으면 바로 뺏지 않고 F 연타 줄다리기로 결판 (끄면 예전처럼 즉시 뺏음)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Game")
	bool bUseGrappleStruggle = true;

	// 꼬리를 잡으면 상대가 가진 꼬리를 모두 가져옴 (끄면 1개만)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Game")
	bool bTakeAllTailsOnGrab = true;

	// 유령이 이동할 시작점에서 이 거리 안에 다른 캐릭터가 있으면 그 시작점은 피함 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Game", meta = (ClampMin = "0"))
	float GhostRelocateClearRadius = 200.f;
	// 전체 경기 시간의 이 비율이 지나면 기권할 수 있음 (0.5 = 절반)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Game", meta = (ClampMin = "0", ClampMax = "1"))
	float ForfeitAvailableRatio = 0.5f;

	// 종료 후 자동 로비 복귀 시간(초), 0이면 없음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Game", meta = (ClampMin = "0"))
	float ReturnToLobbyDelaySeconds = 20.f;

	// true면 사용 중인 색 중 다음 색이 목표
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Game")
	bool bTargetFromActiveColorsOnly = true;

	// 경기 맵에 들어온 뒤 본 경기 시작 전 카운트다운(초). 숫자와 소리가 나옴
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Game", meta = (ClampMin = "0.1"))
	float GameStartCountdownSeconds = 3.f;

	// AI 플레이어를 조종할 컨트롤러 클래스 (기본 ACTAIController)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI")
	TSubclassOf<class ACTAIController> AIControllerClass;

	// 로비 설정 없이 시작한 테스트용 AI 수 (로비에서 시작하면 로비 설정을 씀)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI", meta = (ClampMin = "0"))
	int32 DebugAIPlayerCount = 0;

	// 테스트용 자동 시작. 로비 만들면 끄기
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Debug")
	bool bAutoStartForTesting = true;

	// 사용 안 함 (GameStartCountdownSeconds 로 대체). 이 값이 저장된 블루프린트가 있어 남겨 둠
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Debug", meta = (ClampMin = "0"))
	float AutoStartDelaySeconds = 5.f;

private:
	// 모드별 규칙 객체가 이 클래스의 꼬리잡기 함수들을 부름
	friend class UCTRule_TailChase;
	friend class UCTRule_TreasureHunt;

	// 지금 모드의 규칙 객체 (없으면 꼬리잡기로 만듦)
	UCTGameRule* GetRule();

	// 모드에 맞는 규칙 객체 생성
	void CreateRule(ECTGameModeType Mode);

	UPROPERTY(Transient)
	TObjectPtr<UCTGameRule> Rule;


	// 이탈자 반영: 남은 사람끼리 목표 색 재연결
	void HandlePlayerLeftDuringGame(const ACTPlayerState* LeavingPlayer);

	// PostLogin / 심리스 트래블 공통 초기화
	void InitPlayerForGame(APlayerController* NewPlayer);

	// 시작 전 카운트다운. 끝나면 StartGame
	void BeginStartCountdown();

	// 경기 시작 때 설정한 수만큼 AI 플레이어를 만들어 시작 지점에 세움 (한 번만). 사람과 합쳐 최대 인원, 색 수를 넘지 않음
	void SpawnAIPlayers();
	bool bAIPlayersSpawned = false;

	// 유령 부활, 색 배정, 라운드 시간 설정
	void StartNewRound();

	// 0.2초마다 라운드/게임 종료 확인
	void TickGameFlow();

	// 전원 추격 버프 재계산
	void RefreshAllChaseBuffs();

	// 관전자와 기권한 플레이어를 뺀, 경기에 참여 중인 플레이어
	TArray<ACTPlayerState*> CollectPlayers() const;

	// 대기 로비로 Seamless Travel
	void ReturnToLobby();

	FTimerHandle ReturnToLobbyTimerHandle;	FTimerHandle FlowTimerHandle;
	FTimerHandle AutoStartTimerHandle;
};
