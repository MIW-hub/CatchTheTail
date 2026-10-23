// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CTUserWidget.h"
#include "CTHUDWidget.generated.h"

class UCTScoreEntryWidget;
class UProgressBar;
class UVerticalBox;
class UWidget;

UCLASS(Abstract)
class CATCHTHETAIL_API UCTHUDWidget : public UCTUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 블루프린트 이벤트 (연출용)
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnPhaseChanged(ECTGamePhase NewPhase);

	// 새 라운드 시작
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnRoundChanged(int32 NewRoundNumber);

	// 카운트다운 숫자 변경
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnCountdownTick(int32 Number);

	// 경기 전 카운트다운 숫자 변경
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnGameStartCountdownTick(int32 Number);

	// 색/목표/꼬리 수/유령 상태 변경
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnMyStateChanged(ACTPlayerState* MyPlayerState);

	// 유령 전환/부활
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnMyGhostChanged(bool bIsGhost);

	// 내 꼬리 수 변경
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnMyTailCountChanged(int32 NewTailCount, int32 Delta);

	// 설정
	// 점수판 한 줄 위젯
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	TSubclassOf<UCTScoreEntryWidget> ScoreEntryClass;

	// 목록 확인 간격(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI", meta = (ClampMin = "0.05"))
	float ScoreboardSyncInterval = 0.25f;

	// 위젯 (WBP 같은 이름 자동 연결)
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
	// 연결 (늦게 도착해서 매 프레임 확인)
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

	// 갱신
	void UpdateTimeDisplay();
	void UpdateActionGauges();
	void RefreshMyState();
	void SyncScoreboardPlayers();
	void RefreshScoreboard();

	TWeakObjectPtr<ACTGameState> BoundGameState;
	TWeakObjectPtr<ACTPlayerState> BoundMyPlayerState;

	// 점수판 줄 보관
	UPROPERTY(Transient)
	TArray<TObjectPtr<UCTScoreEntryWidget>> ScoreEntries;

	// 이벤트 연결한 PlayerState
	TArray<TWeakObjectPtr<ACTPlayerState>> BoundScorePlayerStates;

	float ScoreboardSyncAccumulator = 0.f;
	bool bScoreboardDirty = true;

	int32 LastCountdownNumber = -1;
	int32 LastTailCount = INDEX_NONE;
	bool bLastIsGhost = false;
};
