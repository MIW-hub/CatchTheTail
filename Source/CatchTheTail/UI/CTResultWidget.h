// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CTUserWidget.h"
#include "CTResultWidget.generated.h"

class UCTScoreEntryWidget;
class UVerticalBox;

/**
 * 경기 결과 화면. (WBP_Result의 부모. 경기가 끝나면 PlayerController가 띄웁니다)
 *
 * WBP에 만들 수 있는 위젯 (이름을 똑같이, 모두 선택 사항)
 *  - Text_ResultTitle   : "승리!" / "패배" / "동점! 연장전"
 *  - Text_Winners       : 승자 이름들
 *  - VerticalBox_Ranking: 전체 순위 (+ ScoreEntryClass 지정)
 */
UCLASS(Abstract)
class CATCHTHETAIL_API UCTResultWidget : public UCTUserWidget
{
	GENERATED_BODY()

public:
	/** GameState의 경기 결과를 읽어 화면을 채웁니다. */
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	void ShowResult();

protected:
	/** 결과 표시가 끝난 뒤 호출 (연출, 사운드용) */
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnResultShown(bool bLocalPlayerWon, bool bIsTie);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	TSubclassOf<UCTScoreEntryWidget> ScoreEntryClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FText WinText = NSLOCTEXT("CT", "UI_Result_Win", "승리!");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FText LoseText = NSLOCTEXT("CT", "UI_Result_Lose", "패배");

	/** 동점 (연장전 필요) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FText TieText = NSLOCTEXT("CT", "UI_Result_Tie", "동점! 연장전");

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ResultTitle;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Winners;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VerticalBox_Ranking;
};
