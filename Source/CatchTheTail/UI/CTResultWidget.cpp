// Copyright Moon Party Games. All Rights Reserved.
#include "CTResultWidget.h"
#include "../Core/CTAudioSubsystem.h"
#include "CTScoreEntryWidget.h"
#include "../Core/CTGameState.h"
#include "../Core/CTGameInstance.h"
#include "../Player/CTPlayerState.h"
#include "../Player/CTPlayerController.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

void UCTResultWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 버튼 소리 (올림 / 누름)
	UCTAudioSubsystem::ApplyUISounds(this);

	if (Button_ReturnToLobby) { Button_ReturnToLobby->OnClicked.AddUniqueDynamic(this, &UCTResultWidget::HandleReturnToLobbyClicked); }
	if (Button_LeaveToMenu)   { Button_LeaveToMenu->OnClicked.AddUniqueDynamic(this, &UCTResultWidget::HandleLeaveToMenuClicked); }
}

void UCTResultWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 로비로 돌아가기: 방장에게만 버튼, 모두에게 남은 시간 안내
	const ACTGameState* GameState = GetCTGameState();
	const ACTPlayerState* MyPlayerState = GetOwningCTPlayerState();

	if (Button_ReturnToLobby)
	{
		Button_ReturnToLobby->SetVisibility((MyPlayerState && MyPlayerState->IsHost()) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (Text_ReturnHint)
	{
		const float Remaining = GameState ? GameState->GetReturnToLobbyRemainingSeconds() : 0.f;
		if (Remaining > 0.f)
		{
			Text_ReturnHint->SetText(FText::Format(NSLOCTEXT("CT", "UI_Result_ReturnHint", "{0}초 후 로비로 돌아갑니다"), FMath::CeilToInt(Remaining)));
			Text_ReturnHint->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Text_ReturnHint->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UCTResultWidget::HandleReturnToLobbyClicked()
{
	if (ACTPlayerController* Controller = GetOwningPlayer<ACTPlayerController>())
	{
		Controller->RequestReturnToLobby();
	}
}

void UCTResultWidget::HandleLeaveToMenuClicked()
{
	if (UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>())
	{
		GameInstance->LeaveToMainMenu(FText::GetEmpty());
	}
}

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
		// 승자 목록에 없으면 패배 (동점은 연장전 대상 별도)
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
