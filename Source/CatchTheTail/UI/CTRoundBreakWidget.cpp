// Copyright Moon Party Games. All Rights Reserved.
#include "CTRoundBreakWidget.h"
#include "CTRoundBreakRowWidget.h"
#include "../Player/CTPlayerState.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"

namespace
{
	int32 At(const TArray<int32>& Values, int32 Index) { return Values.IsValidIndex(Index) ? Values[Index] : 0; }
}

void UCTRoundBreakWidget::Show(const FCTRoundBreakInfo& Info, int32 MyTeam)
{
	ShownInfo = Info;
	ShownMyTeam = MyTeam;
	Elapsed = 0.f;
	bRevealed = false;

	const int32 Enemy = 1 - MyTeam;
	const ACTGameState* GameState = GetCTGameState();
	auto TeamTint = [&](int32 Team)
	{
		const ECTColor Color = GameState ? GameState->GetTeamColor(Team) : ECTColor::None;
		return Color == ECTColor::None ? FallbackTeamColor : ACTPlayerState::GetLinearColorOf(Color);
	};
	const FLinearColor MyColor = TeamTint(MyTeam);
	const FLinearColor EnemyColor = TeamTint(Enemy);

	if (Text_RoundTitle) { Text_RoundTitle->SetText(FText::Format(NSLOCTEXT("CT", "UI_RoundBreak_Title", "라운드 {0} 결과"), Info.RoundNumber)); }
	if (Text_MyTeamLabel) { Text_MyTeamLabel->SetText(NSLOCTEXT("CT", "UI_RoundBreak_MyTeam", "우리 팀")); }
	if (Text_EnemyTeamLabel) { Text_EnemyTeamLabel->SetText(NSLOCTEXT("CT", "UI_RoundBreak_EnemyTeam", "상대 팀")); }
	if (Image_MyTeamColor) { Image_MyTeamColor->SetColorAndOpacity(MyColor); }
	if (Image_EnemyTeamColor) { Image_EnemyTeamColor->SetColorAndOpacity(EnemyColor); }

	// 에너지는 0 에서 올라가고, 승점은 이번 라운드 반영 전 값으로 시작
	if (Text_MyTeamEnergy) { Text_MyTeamEnergy->SetText(FText::AsNumber(0)); }
	if (Text_EnemyTeamEnergy) { Text_EnemyTeamEnergy->SetText(FText::AsNumber(0)); }
	if (Text_MyTeamWins) { Text_MyTeamWins->SetText(FText::AsNumber(At(Info.WinsBefore, MyTeam))); }
	if (Text_EnemyTeamWins) { Text_EnemyTeamWins->SetText(FText::AsNumber(At(Info.WinsBefore, Enemy))); }
	for (UTextBlock* Hidden : { Text_MyWinPlus.Get(), Text_EnemyWinPlus.Get(), Text_MyResult.Get(), Text_EnemyResult.Get() })
	{
		if (Hidden) { Hidden->SetVisibility(ESlateVisibility::Hidden); }
	}

	// 머리 알약은 처음에는 둘 다 연보라, 왕관은 숨김 (승패가 나오면 바뀜)
	for (UImage* Header : { Image_MyTeamHeader.Get(), Image_EnemyTeamHeader.Get() }) { if (Header) { Header->SetColorAndOpacity(LoserHeaderColor); } }
	for (UImage* Crown : { Image_MyCrown.Get(), Image_EnemyCrown.Get() }) { if (Crown) { Crown->SetVisibility(ESlateVisibility::Collapsed); } }
	for (UTextBlock* Name : { Text_MyTeamLabel.Get(), Text_EnemyTeamLabel.Get() }) { if (Name) { Name->SetColorAndOpacity(FSlateColor(DarkTextColor)); } }
	FillPlayers(Box_MyPlayers, Info, MyTeam, MyColor, MyTeam);
	FillPlayers(Box_EnemyPlayers, Info, Enemy, EnemyColor, MyTeam);

	const int32 MyEnergy = At(Info.TeamEnergy, MyTeam);
	const int32 EnemyEnergy = At(Info.TeamEnergy, Enemy);
	bMyWon = MyEnergy >= EnemyEnergy;
	bEnemyWon = EnemyEnergy >= MyEnergy;

	BP_OnShow();
}

void UCTRoundBreakWidget::FillPlayers(UPanelWidget* Box, const FCTRoundBreakInfo& Info, int32 Team, const FLinearColor& Color, int32 MyTeam)
{
	if (!Box || !RowWidgetClass) { return; }
	Box->ClearChildren();

	const ACTPlayerState* Me = GetOwningCTPlayerState();
	const FString MyName = Me ? Me->GetPlayerName() : FString();
	for (const FCTRoundBreakEntry& Entry : Info.Entries)
	{
		if (Entry.TeamId != Team) { continue; }
		if (UCTRoundBreakRowWidget* Row = CreateWidget<UCTRoundBreakRowWidget>(GetOwningPlayer(), RowWidgetClass))
		{
			Row->SetEntry(Entry.Name, Entry.Score, Color, Team == MyTeam && Entry.Name == MyName);
			Box->AddChild(Row);
		}
	}
}

void UCTRoundBreakWidget::UpdatePlus(UTextBlock* Plus, bool bWon, float T)
{
	if (!Plus) { return; }
	if (!bWon || T < 0.f) { Plus->SetVisibility(ESlateVisibility::Hidden); return; }

	// 떠오르면서 점점 투명해짐
	const float Alpha = FMath::Clamp(T / FMath::Max(PlusRiseSeconds, 0.1f), 0.f, 1.f);
	Plus->SetVisibility(ESlateVisibility::HitTestInvisible);
	Plus->SetRenderTranslation(FVector2D(0.f, -PlusRiseHeight * Alpha));
	Plus->SetRenderOpacity(Alpha < 0.7f ? 1.f : (1.f - Alpha) / 0.3f);
}

void UCTRoundBreakWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Elapsed += InDeltaTime;

	const int32 Enemy = 1 - ShownMyTeam;

	// 에너지 합계 올라가는 연출 (앞쪽은 빠르게, 끝에서 천천히)
	const float CountAlpha = FMath::Clamp((Elapsed - CountStartSeconds) / FMath::Max(CountDurationSeconds, 0.1f), 0.f, 1.f);
	const float Eased = 1.f - FMath::Square(1.f - CountAlpha);
	if (Text_MyTeamEnergy) { Text_MyTeamEnergy->SetText(FText::AsNumber(FMath::RoundToInt(At(ShownInfo.TeamEnergy, ShownMyTeam) * Eased))); }
	if (Text_EnemyTeamEnergy) { Text_EnemyTeamEnergy->SetText(FText::AsNumber(FMath::RoundToInt(At(ShownInfo.TeamEnergy, Enemy) * Eased))); }

	// 승패가 정해지는 순간: 결과 글자, 승점 +1, "+1" 떠오름
	if (!bRevealed && Elapsed >= RevealSeconds)
	{
		bRevealed = true;
		const bool bTie = bMyWon && bEnemyWon;
		auto ResultText = [&](bool bWon)
		{
			return bTie ? NSLOCTEXT("CT", "UI_RoundBreak_Draw", "무승부") : (bWon ? NSLOCTEXT("CT", "UI_RoundBreak_Win", "승리") : NSLOCTEXT("CT", "UI_RoundBreak_Lose", "패배"));
		};
		if (Text_MyResult) { Text_MyResult->SetText(ResultText(bMyWon)); Text_MyResult->SetVisibility(ESlateVisibility::HitTestInvisible); }
		if (Text_EnemyResult) { Text_EnemyResult->SetText(ResultText(bEnemyWon)); Text_EnemyResult->SetVisibility(ESlateVisibility::HitTestInvisible); }
		if (Text_MyTeamWins) { Text_MyTeamWins->SetText(FText::AsNumber(At(ShownInfo.WinsAfter, ShownMyTeam))); }
		if (Text_EnemyTeamWins) { Text_EnemyTeamWins->SetText(FText::AsNumber(At(ShownInfo.WinsAfter, Enemy))); }
		// 이긴 팀 강조 (동점이면 둘 다 강조 없이 연보라)
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const bool bWin = !bTie && (Side == 0 ? bMyWon : bEnemyWon);
			UImage* Header = (Side == 0) ? Image_MyTeamHeader.Get() : Image_EnemyTeamHeader.Get();
			UImage* Crown = (Side == 0) ? Image_MyCrown.Get() : Image_EnemyCrown.Get();
			UTextBlock* Name = (Side == 0) ? Text_MyTeamLabel.Get() : Text_EnemyTeamLabel.Get();
			UTextBlock* SideResult = (Side == 0) ? Text_MyResult.Get() : Text_EnemyResult.Get();
			if (Header) { Header->SetColorAndOpacity(bWin ? WinnerHeaderColor : LoserHeaderColor); }
			if (Crown) { Crown->SetVisibility(bWin ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }
			if (Name) { Name->SetColorAndOpacity(FSlateColor(bWin ? FLinearColor::White : DarkTextColor)); }
			if (SideResult) { SideResult->SetColorAndOpacity(FSlateColor(bWin ? WinnerHeaderColor : LoserResultColor)); }
		}
		BP_OnReveal(bMyWon, bEnemyWon);
	}

	const float PlusTime = bRevealed ? Elapsed - RevealSeconds : -1.f;
	UpdatePlus(Text_MyWinPlus, bMyWon, PlusTime);
	UpdatePlus(Text_EnemyWinPlus, bEnemyWon, PlusTime);
}
