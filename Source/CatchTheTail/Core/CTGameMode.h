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

	// 시간 종료 시 자동 호출
	UFUNCTION(BlueprintCallable, Category = "CT|Game")
	void EndGame();

	// 서버, 방장 로비 복귀 요청 (종료 후만)
	bool RequestReturnToLobby(APlayerController* Requester);

	// 서버, 점수 변경 + 유령 처리
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

	// 전화부스 한 판 사용 횟수 (플레이어마다)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Gimmick", meta = (ClampMin = "0"))
	int32 PhoneUsesPerGame = 2;

	// 꼬리가 이 수보다 적으면 새 라운드에 채움, 0이면 안 채움
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Game", meta = (ClampMin = "0"))
	int32 MinTailCountAtRoundStart = 1;

	// 종료 후 자동 로비 복귀 시간(초), 0이면 없음
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
	// 이탈자 반영: 남은 사람끼리 목표 색 재연결
	void HandlePlayerLeftDuringGame(const ACTPlayerState* LeavingPlayer);

	// PostLogin / 심리스 트래블 공통 초기화
	void InitPlayerForGame(APlayerController* NewPlayer);

	// 시작 전 카운트다운. 끝나면 StartGame
	void BeginStartCountdown();

	// 유령 부활, 색 배정, 라운드 시간 설정
	void StartNewRound();

	// 색 배정, 직전 색 회피
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
