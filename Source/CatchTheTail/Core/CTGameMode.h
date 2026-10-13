// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "../Type/CTTypes.h"
#include "CTGameMode.generated.h"

class ACTPlayerState;
class ACTGameState;

/**
 * 게임의 규칙을 책임지는 클래스 (서버에만 존재합니다).
 *
 * 하는 일
 *  1. 게임 시작 / 종료
 *  2. 1분마다 라운드 변경: 색 랜덤 배정(중복 없음), 목표 색 계산, 유령 부활
 *  3. 꼬리 잡기 성공 판정: 점수 변경, 잡힌 사람을 유령으로 만들기
 */
UCLASS()
class CATCHTHETAIL_API ACTGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACTGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** 게임 시작. (로비에서 "시작" 버튼을 누르면 호출하세요) */
	UFUNCTION(BlueprintCallable, Category = "CT|Game")
	void StartGame();

	/** 게임 종료 (시간이 다 되면 자동으로 호출됩니다) */
	UFUNCTION(BlueprintCallable, Category = "CT|Game")
	void EndGame();

	/**
	 * 꼬리 잡기 성공 처리. 캐릭터가 서버에서 호출합니다.
	 * 조건을 모두 확인하고, 통과하면 점수 변경 + 잡힌 사람 유령 처리.
	 * @return 성공하면 true
	 */
	bool HandleTailGrabbed(ACTPlayerState* Grabber, ACTPlayerState* Victim);

	// ===== 블루프린트 이벤트 (연출, UI 연결용) =====
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Events")
	void OnGameStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Events")
	void OnRoundStarted(int32 RoundNumber);

	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Events")
	void OnTailGrabbed(ACTPlayerState* Grabber, ACTPlayerState* Victim);

	/** 게임 종료. bIsTie가 true면 동점(연장전 필요) */
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Events")
	void OnGameFinished(const TArray<ACTPlayerState*>& Winners, bool bIsTie);

protected:
	// ===== 설정 (블루프린트 GameMode에서 수정) =====
	/** 전체 게임 시간(초). 사업계획서: 5~10분 = 300~600 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Game", meta = (ClampMin = "10"))
	float GameDurationSeconds = 300.f;

	/** 한 라운드 길이(초). 끝나면 색이 다시 섞입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Game", meta = (ClampMin = "5"))
	float RoundDurationSeconds = 60.f;

	/** 색이 바뀌기 몇 초 전부터 카운트다운을 보여줄지 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Game", meta = (ClampMin = "0"))
	float CountdownSeconds = 5.f;

	/** 게임 시작 때 모두가 가지고 시작하는 꼬리 수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Game", meta = (ClampMin = "0"))
	int32 InitialTailCount = 0;

	/**
	 * true: 지금 게임에 있는 색들 중에서 "순서상 다음 색"을 목표로 삼습니다.
	 *       (예: 빨강, 파랑, 분홍만 있으면 빨강→파랑→분홍→빨강)
	 * false: 색 순서의 바로 다음 색을 그대로 씁니다. (그 색을 가진 사람이 없을 수도 있음)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Game")
	bool bTargetFromActiveColorsOnly = true;

	/** 테스트용: 접속하면 일정 시간 뒤 게임을 자동 시작 (로비를 만들면 끄세요) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Debug")
	bool bAutoStartForTesting = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Debug", meta = (ClampMin = "0"))
	float AutoStartDelaySeconds = 5.f;

private:
	/** 새 라운드 시작: 유령 부활, 색 배정, 라운드 시간 설정 */
	void StartNewRound();

	/** 사용 인원 수만큼 8색에서 중복 없이 배정, 직전 색은 가능하면 피함 */
	void AssignColorsAndTargets();

	/** 0.2초마다 시간 확인: 라운드가 끝났는지, 게임이 끝났는지 */
	void TickGameFlow();

	/** 모든 캐릭터의 추격 버프 다시 계산 */
	void RefreshAllChaseBuffs();

	/** 관전자를 뺀 모든 플레이어 */
	TArray<ACTPlayerState*> CollectPlayers() const;

	FTimerHandle FlowTimerHandle;
	FTimerHandle AutoStartTimerHandle;
};
