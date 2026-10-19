// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CTUserWidget.h"
#include "CTHUDWidget.generated.h"

class UCTScoreEntryWidget;
class UProgressBar;
class UVerticalBox;
class UWidget;

/**
 * 게임 중 화면 HUD. (WBP_HUD의 부모. PlayerController가 생성합니다)
 *
 * WBP에 만들 수 있는 위젯 (이름을 똑같이, 모두 선택 사항)
 *  [시간]   Text_Phase, Text_GameTime, Text_RoundNumber, Text_RoundTime
 *           Widget_Countdown(카운트다운 중에만 보임), Text_Countdown
 *  [내 정보] Text_MyColor, Image_MyColor, Text_TargetColor, Image_TargetColor, Text_TailCount
 *           Widget_Ghost(유령일 때만 보임)
 *  [행동]   ProgressBar_Run, Text_RunState, ProgressBar_Push
 *  [점수판] VerticalBox_Scoreboard (+ ScoreEntryClass 지정)
 *
 * 연출(애니메이션, 사운드)은 BP_On... 이벤트에서 연결하세요.
 */
UCLASS(Abstract)
class CATCHTHETAIL_API UCTHUDWidget : public UCTUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ===== 블루프린트 이벤트 (연출용) =====
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnPhaseChanged(ECTGamePhase NewPhase);

	/** 새 라운드 시작 (색이 바뀜) */
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnRoundChanged(int32 NewRoundNumber);

	/** 카운트다운 숫자가 바뀔 때 (5, 4, 3, 2, 1) */
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnCountdownTick(int32 Number);

	/** 경기 시작 전 카운트다운 숫자가 바뀔 때 (5, 4, 3, 2, 1). BP_OnCountdownTick도 함께 불립니다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnGameStartCountdownTick(int32 Number);

	/** 내 색 / 목표 색 / 꼬리 수 / 유령 상태가 바뀜 */
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnMyStateChanged(ACTPlayerState* MyPlayerState);

	/** 내가 유령이 되거나 부활함 */
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnMyGhostChanged(bool bIsGhost);

	/** 내 꼬리 수가 바뀜 (Delta > 0: 잡음, < 0: 잡힘) */
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnMyTailCountChanged(int32 NewTailCount, int32 Delta);

	// ===== 설정 =====
	/** 점수판 한 줄로 쓸 위젯 (WBP_ScoreEntry) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	TSubclassOf<UCTScoreEntryWidget> ScoreEntryClass;

	/** 점수판 플레이어 목록을 확인하는 간격(초). 이름 변경, 입장/퇴장 반영용 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI", meta = (ClampMin = "0.05"))
	float ScoreboardSyncInterval = 0.25f;

	// ===== 위젯 (WBP에서 같은 이름으로 만들면 자동 연결) =====
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Phase;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_GameTime;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_RoundNumber;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_RoundTime;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Widget_Countdown;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Countdown;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_MyColor;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_MyColor;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_TargetColor;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_TargetColor;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_TailCount;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Widget_Ghost;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ProgressBar_Run;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_RunState;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ProgressBar_Push;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VerticalBox_Scoreboard;

private:
	// ----- 연결 (GameState / 내 PlayerState는 접속 직후 늦게 도착할 수 있어 매 프레임 확인) -----
	void TryBindGameState();
	void TryBindMyPlayerState();
	void UnbindAll();

	UFUNCTION()
	void HandlePhaseChanged(ECTGamePhase NewPhase);

	UFUNCTION()
	void HandleRoundChanged(int32 NewRoundNumber);

	UFUNCTION()
	void HandleMyPlayerStateChanged(ACTPlayerState* ChangedPlayerState);

	UFUNCTION()
	void HandleAnyPlayerStateChanged(ACTPlayerState* ChangedPlayerState);

	// ----- 갱신 -----
	void UpdateTimeDisplay();
	void UpdateActionGauges();
	void RefreshMyState();
	void SyncScoreboardPlayers();
	void RefreshScoreboard();

	TWeakObjectPtr<ACTGameState> BoundGameState;
	TWeakObjectPtr<ACTPlayerState> BoundMyPlayerState;

	/** 점수판 줄들 (VerticalBox에서 잠시 빠져도 사라지지 않게 보관) */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UCTScoreEntryWidget>> ScoreEntries;

	/** 이벤트를 연결한 PlayerState들 (해제용) */
	TArray<TWeakObjectPtr<ACTPlayerState>> BoundScorePlayerStates;

	float ScoreboardSyncAccumulator = 0.f;
	bool bScoreboardDirty = true;

	int32 LastCountdownNumber = -1;
	int32 LastTailCount = INDEX_NONE;
	bool bLastIsGhost = false;
};
