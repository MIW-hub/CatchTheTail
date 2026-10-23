// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/OnlineReplStructs.h"
#include "../Type/CTTypes.h"
#include "CTGameMode.generated.h"

class ACTPlayerState;
class ACTGameState;

UCLASS()
class CATCHTHETAIL_API ACTGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACTGameMode();

	// 게임이 진행 중이면 새 입장을 거절
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	// 게임 중 플레이어가 나가면 목표 색 정리, 혼자 남으면 종료
	virtual void Logout(AController* Exiting) override;
	// 로비에서 Seamless Travel로 넘어온 플레이어는 PostLogin이 호출되지 않으므로 여기서 같은 초기화
	virtual void HandleSeamlessTravelPlayer(AController*& C) override;

	// 로비 시작 버튼에서 호출
	UFUNCTION(BlueprintCallable, Category = "CT|Game")
	void StartGame();

	// 시간 종료 시 자동 호출
	UFUNCTION(BlueprintCallable, Category = "CT|Game")
	void EndGame();

	// 서버. 방장의 로비 복귀 요청. 게임이 끝난 상태에서만 가능. 성공 시 true
	bool RequestReturnToLobby(APlayerController* Requester);

	// 서버. 점수 변경 + 유령 처리. 성공 시 true
	bool HandleTailGrabbed(ACTPlayerState* Grabber, ACTPlayerState* Victim);

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

	/**
	 * 새 라운드가 시작될 때 꼬리가 이 수보다 적은 플레이어에게 꼬리를 채워 줍니다.
	 * (꼬리를 모두 잃은 플레이어가 다음 라운드에 다시 꼬리를 갖고 시작하도록). 0이면 채워 주지 않음
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Game", meta = (ClampMin = "0"))
	int32 MinTailCountAtRoundStart = 1;

	/**
	 * 게임이 끝난 뒤 이 시간(초)이 지나면 자동으로 대기 로비로 돌아갑니다. 방장이 버튼으로 먼저 돌아갈 수도 있습니다.
	 * 0이면 자동 복귀 없음 (방장이 버튼을 눌러야 함)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Game", meta = (ClampMin = "0"))
	float ReturnToLobbyDelaySeconds = 20.f;

	// true면 사용 중인 색 중 다음 색이 목표
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Game")
	bool bTargetFromActiveColorsOnly = true;

	// 테스트용 자동 시작. 로비 만들면 끄기
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Debug")
	bool bAutoStartForTesting = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Debug", meta = (ClampMin = "0"))
	float AutoStartDelaySeconds = 5.f;

private:
	// 게임 중 나간 플레이어 반영: 남은 사람끼리 목표 색 다시 이음. 2명 미만이면 종료
	void HandlePlayerLeftDuringGame(const ACTPlayerState* LeavingPlayer);

	// PostLogin / 심리스 트래블 공통 초기화
	void InitPlayerForGame(APlayerController* NewPlayer);

	// 시작 전 카운트다운. 끝나면 StartGame
	void BeginStartCountdown();

	// 유령 부활, 색 배정, 라운드 시간 설정
	void StartNewRound();

	// 중복 없이 색 배정. 직전 색 가능하면 회피
	void AssignColorsAndTargets();

	// 0.2초마다 라운드/게임 종료 확인
	void TickGameFlow();

	// 전원 추격 버프 재계산
	void RefreshAllChaseBuffs();

	// 관전자 제외 플레이어
	TArray<ACTPlayerState*> CollectPlayers() const;

	// 대기 로비로 Seamless Travel
	void ReturnToLobby();

	FTimerHandle ReturnToLobbyTimerHandle;
	FTimerHandle FlowTimerHandle;
	FTimerHandle AutoStartTimerHandle;
};
