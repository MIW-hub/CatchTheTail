// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CTResultWidget.h"
#include "CTTreasureResultWidget.generated.h"

class UImage;
class UTextBlock;
class UPanelWidget;
class UCTRoundBreakRowWidget;

// 보물찾기 경기 결과 화면. 모양(디자인)은 WBP_CTTreasureResultWidget 에서 정함.
// 왼쪽 A팀, 오른쪽 B팀(팀 번호 0, 1 고정), 가운데에 큰 글자로 이긴 팀을 보여줌.
// 로비 복귀/나가기 버튼과 안내 글자는 꼬리잡기 결과 화면과 같은 이름을 씀 (Button_ReturnToLobby, Button_LeaveToMenu, Text_ReturnHint).
// 아래 이름으로 위젯을 만들어 두면 자동으로 채워집니다 (없는 이름은 건너뜀).
//   Text_WinnerBig                           가운데 큰 글자 "A팀 승리!" / "B팀 승리!" / "무승부"
//   Text_RoundScore                          가운데 라운드 승점 "2 : 1"
//   Text_TeamAName / Text_TeamBName          "A팀" / "B팀"
//   Image_TeamAColor / Image_TeamBColor      팀 색
//   Text_TeamAWins / Text_TeamBWins          팀 라운드 승점
//   Text_TeamAEnergy / Text_TeamBEnergy      팀 에너지 총합 (경기 동안 모은 에너지)
//   Box_TeamAPlayers / Box_TeamBPlayers      개인 점수 줄(이름, 에너지)이 들어갈 세로 상자 (RowWidgetClass 로 채움)
UCLASS(Abstract)
class CATCHTHETAIL_API UCTTreasureResultWidget : public UCTResultWidget
{
	GENERATED_BODY()

public:
	UCTTreasureResultWidget(const FObjectInitializer& ObjectInitializer);

	virtual void ShowResult() override;

protected:
	// 결과가 정해졌을 때 (연출용). WinnerTeam: 0 = A팀, 1 = B팀, -1 = 무승부
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnTeamResult(int32 WinnerTeam, bool bMyTeamWon);

	// 개인 점수 줄 모양 (WBP_CTRoundBreakRowWidget 와 같은 줄을 씀)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	TSubclassOf<UCTRoundBreakRowWidget> RowWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FLinearColor FallbackTeamColor = FLinearColor(0.7f, 0.7f, 0.7f, 1.f);

	// 팀 머리 막대: 이긴 팀은 분홍 + 흰 글자, 아니면 연보라 + 진한 글자
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FLinearColor WinnerHeaderColor = FLinearColor(0.888f, 0.024f, 0.147f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FLinearColor LoserHeaderColor = FLinearColor(0.83f, 0.78f, 0.89f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FLinearColor DarkTextColor = FLinearColor(0.058f, 0.0006f, 0.032f, 1.f);

	// 이긴 쪽 점수 숫자 색
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FLinearColor ScoreWinColor = FLinearColor(0.888f, 0.024f, 0.147f, 1.f);

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_WinnerBig;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_ScoreA;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_ScoreB;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Image_TeamAHeader;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Image_TeamBHeader;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Image_WinnerCrown;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_RoundScore;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_TeamAName;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_TeamBName;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Image_TeamAColor;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Image_TeamBColor;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_TeamAWins;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_TeamBWins;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_TeamAEnergy;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_TeamBEnergy;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UPanelWidget> Box_TeamAPlayers;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UPanelWidget> Box_TeamBPlayers;

private:
	void FillTeamPlayers(UPanelWidget* Box, int32 Team, const FLinearColor& Color, int32& OutTotal);
};
