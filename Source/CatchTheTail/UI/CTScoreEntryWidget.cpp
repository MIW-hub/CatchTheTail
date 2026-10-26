// Copyright Moon Party Games. All Rights Reserved.
#include "CTScoreEntryWidget.h"
#include "../Player/CTPlayerState.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UCTScoreEntryWidget::SetEntryPlayerState(ACTPlayerState* InPlayerState)
{
	EntryPlayerState = InPlayerState;
}

void UCTScoreEntryWidget::RefreshEntry(int32 Rank, bool bIsLocalPlayer, bool bAllowCrown)
{
	ACTPlayerState* PlayerState = EntryPlayerState.Get();
	if (!PlayerState) { return; }

	if (Text_Rank)
	{
		Text_Rank->SetText(FText::AsNumber(Rank));
	}
	if (Text_PlayerName)
	{
		Text_PlayerName->SetText(FText::FromString(PlayerState->GetPlayerName()));
	}

	ApplyColorToWidgets(PlayerState->GetAssignedColor(), Text_ColorName, Image_ColorIcon,
		NSLOCTEXT("CT", "UI_NoColor", "-"));

	if (Text_TailCount)
	{
		Text_TailCount->SetText(FText::AsNumber(PlayerState->GetTailCount()));
	}
	if (Image_Crown)
	{
		Image_Crown->SetVisibility((bAllowCrown && Rank == 1) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (Widget_GhostMark)
	{
		Widget_GhostMark->SetVisibility(PlayerState->IsGhost() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (Widget_LocalHighlight)
	{
		Widget_LocalHighlight->SetVisibility(bIsLocalPlayer ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	BP_OnEntryRefreshed(PlayerState, Rank, bIsLocalPlayer);
}
