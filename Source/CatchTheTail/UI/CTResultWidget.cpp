// Copyright Moon Party Games. All Rights Reserved.
#include "CTResultWidget.h"
#include "CTScoreEntryWidget.h"
#include "../Core/CTGameState.h"
#include "../Player/CTPlayerState.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

void UCTResultWidget::ShowResult()
{
	const ACTGameState* GameState = GetCTGameState();
	if (!GameState) { return; }

	const ACTPlayerState* MyPlayerState = GetOwningCTPlayerState();
	const TArray<ACTPlayerState*> Winners = GameState->GetWinners();
	const bool bIsTie = GameState->IsTie();
	const bool bLocalPlayerWon = GameState->IsWinner(MyPlayerState);

	if (Text_ResultTitle)
	{
		// 동점은 "연장전에 가는 사람"이 따로 있으므로, 내가 승자 목록에 없으면 패배로 표시합니다.
		const FText& Title = (bIsTie && bLocalPlayerWon) ? TieText : (bLocalPlayerWon ? WinText : LoseText);
		Text_ResultTitle->SetText(Title);
	}

	if (Text_Winners)
	{
		TArray<FString> WinnerNames;
		for (const ACTPlayerState* Winner : Winners)
		{
			WinnerNames.Add(Winner->GetPlayerName());
		}
		Text_Winners->SetText(FText::FromString(FString::Join(WinnerNames, TEXT(", "))));
	}

	if (VerticalBox_Ranking && ScoreEntryClass)
	{
		VerticalBox_Ranking->ClearChildren();

		const TArray<ACTPlayerState*> Players = GetSortedPlayers(GameState);
		int32 Rank = 0;
		int32 PreviousTailCount = INDEX_NONE;
		for (int32 Index = 0; Index < Players.Num(); ++Index)
		{
			ACTPlayerState* PlayerState = Players[Index];
			if (PlayerState->GetTailCount() != PreviousTailCount)
			{
				Rank = Index + 1;
				PreviousTailCount = PlayerState->GetTailCount();
			}

			UCTScoreEntryWidget* Entry = CreateWidget<UCTScoreEntryWidget>(this, ScoreEntryClass);
			if (!Entry) { continue; }

			Entry->InheritColorIcons(this);
			Entry->SetEntryPlayerState(PlayerState);
			VerticalBox_Ranking->AddChildToVerticalBox(Entry);
			Entry->RefreshEntry(Rank, PlayerState == MyPlayerState);
		}
	}

	BP_OnResultShown(bLocalPlayerWon, bIsTie);
}
