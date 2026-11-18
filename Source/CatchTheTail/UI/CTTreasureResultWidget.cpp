// Copyright Moon Party Games. All Rights Reserved.
#include "CTTreasureResultWidget.h"
#include "CTRoundBreakRowWidget.h"
#include "../Core/CTGameState.h"
#include "../Player/CTPlayerState.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "UObject/ConstructorHelpers.h"

UCTTreasureResultWidget::UCTTreasureResultWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 라운드 사이 결과 팝업과 같은 줄 모양을 기본으로 씀 (블루프린트 값이 있으면 그 값이 우선)
	static ConstructorHelpers::FClassFinder<UCTRoundBreakRowWidget> RowFinder(TEXT("/Game/UI/WBP_CTRoundBreakRowWidget"));
	if (RowFinder.Succeeded()) { RowWidgetClass = RowFinder.Class; }
}

void UCTTreasureResultWidget::FillTeamPlayers(UPanelWidget* Box, int32 Team, const FLinearColor& Color, int32& OutTotal)
{
	OutTotal = 0;
	const ACTGameState* GameState = GetCTGameState();
	if (!GameState) { return; }

	if (Box) { Box->ClearChildren(); }
	const ACTPlayerState* Me = GetOwningCTPlayerState();
	for (ACTPlayerState* Player : GetSortedPlayers(GameState))
	{
		if (!Player || Player->GetTeamId() != Team) { continue; }

		const int32 Score = GetPlayerScore(GameState, Player);
		OutTotal += Score;
		if (!Box || !RowWidgetClass) { continue; }
		if (UCTRoundBreakRowWidget* Row = CreateWidget<UCTRoundBreakRowWidget>(GetOwningPlayer(), RowWidgetClass))
		{
			Row->SetEntry(Player->GetPlayerName(), Score, Color, Player == Me);
			Box->AddChild(Row);
		}
	}
}

void UCTTreasureResultWidget::ShowResult()
{
	// 기본 처리(전적 저장, 버튼, 제목)는 부모가 함
	Super::ShowResult();

	const ACTGameState* GameState = GetCTGameState();
	if (!GameState) { return; }

	auto TeamTint = [&](int32 Team)
	{
		const ECTColor Color = GameState->GetTeamColor(Team);
		return Color == ECTColor::None ? FallbackTeamColor : ACTPlayerState::GetLinearColorOf(Color);
	};
	const FLinearColor ColorA = TeamTint(0);
	const FLinearColor ColorB = TeamTint(1);

	int32 EnergyA = 0, EnergyB = 0;
	FillTeamPlayers(Box_TeamAPlayers, 0, ColorA, EnergyA);
	FillTeamPlayers(Box_TeamBPlayers, 1, ColorB, EnergyB);

	const int32 WinsA = GameState->GetTeamRoundWins(0);
	const int32 WinsB = GameState->GetTeamRoundWins(1);
	// 승점이 많은 팀이 승리, 같으면 무승부
	const int32 WinnerTeam = (WinsA == WinsB) ? -1 : (WinsA > WinsB ? 0 : 1);

	if (Text_TeamAName) { Text_TeamAName->SetText(NSLOCTEXT("CT", "UI_TreasureResult_TeamA", "A팀")); }
	if (Text_TeamBName) { Text_TeamBName->SetText(NSLOCTEXT("CT", "UI_TreasureResult_TeamB", "B팀")); }
	if (Image_TeamAColor) { Image_TeamAColor->SetColorAndOpacity(ColorA); }
	if (Image_TeamBColor) { Image_TeamBColor->SetColorAndOpacity(ColorB); }
	if (Text_TeamAWins) { Text_TeamAWins->SetText(FText::AsNumber(WinsA)); }
	if (Text_TeamBWins) { Text_TeamBWins->SetText(FText::AsNumber(WinsB)); }
	if (Text_TeamAEnergy) { Text_TeamAEnergy->SetText(FText::AsNumber(EnergyA)); }
	if (Text_TeamBEnergy) { Text_TeamBEnergy->SetText(FText::AsNumber(EnergyB)); }
	if (Text_RoundScore) { Text_RoundScore->SetText(FText::Format(NSLOCTEXT("CT", "UI_TreasureResult_Score", "{0} : {1}"), WinsA, WinsB)); }
	if (Text_WinnerBig)
	{
		FText Big;
		if (WinnerTeam < 0) { Big = NSLOCTEXT("CT", "UI_TreasureResult_Draw", "무승부"); }
		else if (WinnerTeam == 0) { Big = NSLOCTEXT("CT", "UI_TreasureResult_WinA", "A팀 승리!"); }
		else { Big = NSLOCTEXT("CT", "UI_TreasureResult_WinB", "B팀 승리!"); }
		Text_WinnerBig->SetText(Big);
	}

	// 팀 머리 막대와 점수 숫자 색: 이긴 쪽 강조
	for (int32 Team = 0; Team < 2; ++Team)
	{
		const bool bWin = (WinnerTeam == Team);
		UImage* Header = (Team == 0) ? Image_TeamAHeader.Get() : Image_TeamBHeader.Get();
		UTextBlock* Name = (Team == 0) ? Text_TeamAName.Get() : Text_TeamBName.Get();
		UTextBlock* ScoreText = (Team == 0) ? Text_ScoreA.Get() : Text_ScoreB.Get();
		if (Header) { Header->SetColorAndOpacity(bWin ? WinnerHeaderColor : LoserHeaderColor); }
		if (Name) { Name->SetColorAndOpacity(FSlateColor(bWin ? FLinearColor::White : DarkTextColor)); }
		if (ScoreText) { ScoreText->SetText(FText::AsNumber(Team == 0 ? WinsA : WinsB)); ScoreText->SetColorAndOpacity(FSlateColor(bWin ? ScoreWinColor : DarkTextColor)); }
	}
	if (Image_WinnerCrown) { Image_WinnerCrown->SetVisibility(WinnerTeam >= 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden); }

	const ACTPlayerState* Me = GetOwningCTPlayerState();
	BP_OnTeamResult(WinnerTeam, Me && WinnerTeam >= 0 && Me->GetTeamId() == WinnerTeam);
}
