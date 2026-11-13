// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CTUserWidget.h"
#include "CTResultWidget.generated.h"

class UCTScoreEntryWidget;
class UButton;
class UImage;
class UVerticalBox;

UCLASS(Abstract)
class CATCHTHETAIL_API UCTResultWidget : public UCTUserWidget
{
	GENERATED_BODY()

public:
	// 경기 결과 채우기
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	void ShowResult();

public:
	UCTResultWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 결과를 채우기 전에 모드를 알려 줌 (모드별로 표 모양을 나눌 때 씀)
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnModeKnown(ECTGameModeType Mode);

	// 결과 표시 끝 (연출용)
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnResultShown(bool bLocalPlayerWon, bool bIsTie);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	TSubclassOf<UCTScoreEntryWidget> ScoreEntryClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FText WinText = NSLOCTEXT("CT", "UI_Result_Win", "승리!");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FText LoseText = NSLOCTEXT("CT", "UI_Result_Lose", "패배");

	// 제목에 쓰는 내 등수 문구 ({0} = 등수). 승리/패배 대신 표시
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FText RankText = NSLOCTEXT("CT", "UI_Result_Rank", "{0}등");

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ResultTitle;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Winners;

	// 내 등수 옆 왕관 (1등일 때만 보임)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_RankCrown;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VerticalBox_Ranking;

	// 로비로 돌아가기 (방장에게만 보임)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_ReturnToLobby;

	// 메인 메뉴로 나가기
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_LeaveToMenu;

	// "N초 후 로비로 돌아갑니다" 안내
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ReturnHint;

private:
	// 이 판의 결과를 전적에 한 번만 저장
	bool bRecordedToHistory = false;

	UFUNCTION()
	void HandleReturnToLobbyClicked();

	UFUNCTION()
	void HandleLeaveToMenuClicked();
};
