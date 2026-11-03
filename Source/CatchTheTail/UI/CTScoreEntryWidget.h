// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CTUserWidget.h"
#include "CTScoreEntryWidget.generated.h"

class UWidget;

UCLASS(Abstract)
class CATCHTHETAIL_API UCTScoreEntryWidget : public UCTUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	void SetEntryPlayerState(ACTPlayerState* InPlayerState);

	UFUNCTION(BlueprintPure, Category = "CT|UI")
	ACTPlayerState* GetEntryPlayerState() const { return EntryPlayerState.Get(); }

	// 표시 갱신 (Rank는 1부터)
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	void RefreshEntry(int32 Rank, bool bIsLocalPlayer, bool bAllowCrown = true);

protected:
	// 갱신 후 (연출용)
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

	// 1등(공동 포함) 표시용 왕관
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Image_Crown;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Widget_GhostMark;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Widget_LocalHighlight;

private:
	TWeakObjectPtr<ACTPlayerState> EntryPlayerState;
};
