// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CTUserWidget.h"
#include "CTHUDWidget.generated.h"

class UCTScoreEntryWidget;
class UMaterialInstanceDynamic;
class UVerticalBox;
class UWidget;

UCLASS(Abstract)
class CATCHTHETAIL_API UCTHUDWidget : public UCTUserWidget
{
	GENERATED_BODY()

public:
	// 화면 위쪽에 안내 문구를 잠깐 띄움 (전화부스 결과 등). Text_Notice 가 없으면 코드에서 만듦
	void ShowNotice(const FText& Message, float Seconds);

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

	// 화면에 보여줄 최대 순위 줄 수. 0이면 전부 (전체 순위는 결과 화면)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI", meta = (ClampMin = "0"))
	int32 MaxScoreboardEntries = 4;

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

	// 나를 잡으러 오는 사람 (나를 목표로 가진 사람)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_HunterColor;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Hunter;

	// 나
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_MyColor;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_My;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_TargetColor;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_TargetColor;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_TailCount;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Widget_Ghost;

	/**
	 * 스킬 슬롯 (달리기 / 잡기 / 밀기)
	 * Image_CoolTime, _1, _2 : 달리기, 잡기, 밀기 슬롯 위에 덮는 쿨다운 이미지.
	 *                          브러시를 M_CT_CooldownRadial 머티리얼로 두면 시계 방향으로 걷히고,
	 *                          일반 이미지면 쿨타임 동안만 어둡게 표시됩니다.
	 * Text_RunCooldown, Text_GrabCooldown, Text_PushCooldown : 남은 시간(초). 쿨타임이 아닐 때는 비어 있습니다.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_CoolTime;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_RunCooldown;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_CoolTime_1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_GrabCooldown;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_CoolTime_2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_PushCooldown;

	// 쿨다운 머티리얼의 0~1 파라미터 이름 (남은 비율)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FName CooldownProgressParameter = TEXT("Progress");

	// 달리는 중 남은 시간 글자색
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FLinearColor RunActiveTextColor = FLinearColor(0.1f, 0.8f, 1.f, 1.f);

	// 쿨타임 남은 시간 글자색
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FLinearColor CooldownTextColor = FLinearColor::White;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VerticalBox_Scoreboard;

	// 안내 문구 (선택). 없으면 ShowNotice 가 화면 위 가운데에 만듭니다
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Notice;

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
	void UpdateSkillSlots();
	void RefreshMyState();
	void RefreshHunter(bool bForce);

	// 슬롯 하나 갱신. Remaining/Total 이 쿨타임, bRunning 이면 달리는 중 표시, bAvailable false 면 사용 불가(어둡게)
	void UpdateSkillSlot(UImage* Overlay, UTextBlock* Text, float Remaining, float Total, bool bRunning, bool bAvailable);

	UMaterialInstanceDynamic* GetOverlayMaterial(UImage* Overlay);

	// 쿨다운 이미지별 동적 머티리얼
	UPROPERTY(Transient)
	TMap<TObjectPtr<UImage>, TObjectPtr<UMaterialInstanceDynamic>> OverlayMaterials;

	UTextBlock* EnsureNoticeText();
	float NoticeRemaining = 0.f;

	ECTColor LastHunterColor = ECTColor::None;
	bool bHunterInitialized = false;
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
