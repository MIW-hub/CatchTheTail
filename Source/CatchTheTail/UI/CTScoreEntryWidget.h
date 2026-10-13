// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CTUserWidget.h"
#include "CTScoreEntryWidget.generated.h"

class UWidget;

/**
 * 점수판 / 결과 화면의 플레이어 한 줄. (WBP_ScoreEntry의 부모)
 *
 * WBP에 만들 수 있는 위젯 (이름을 똑같이, 모두 선택 사항)
 *  - Text_Rank, Text_PlayerName, Text_ColorName, Image_ColorIcon, Text_TailCount
 *  - Widget_GhostMark       : 유령일 때만 보임
 *  - Widget_LocalHighlight  : 내 줄일 때만 보임 (배경 강조 등)
 *
 * 값 갱신은 HUD / 결과 화면이 RefreshEntry를 불러서 합니다.
 */
UCLASS(Abstract)
class CATCHTHETAIL_API UCTScoreEntryWidget : public UCTUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	void SetEntryPlayerState(ACTPlayerState* InPlayerState);

	UFUNCTION(BlueprintPure, Category = "CT|UI")
	ACTPlayerState* GetEntryPlayerState() const { return EntryPlayerState.Get(); }

	/** 표시 갱신. Rank는 1부터 (동점이면 같은 순위) */
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	void RefreshEntry(int32 Rank, bool bIsLocalPlayer);

protected:
	/** 갱신이 끝난 뒤 호출 (추가 연출용) */
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnEntryRefreshed(ACTPlayerState* InPlayerState, int32 Rank, bool bIsLocalPlayer);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Rank;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_PlayerName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ColorName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_ColorIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_TailCount;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Widget_GhostMark;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Widget_LocalHighlight;

private:
	TWeakObjectPtr<ACTPlayerState> EntryPlayerState;
};
