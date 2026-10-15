// Copyright Moon Party Games. All Rights Reserved.
#include "CTUserWidget.h"
#include "../Core/CTGameState.h"
#include "../Player/CTCharacter.h"
#include "../Player/CTPlayerState.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

ACTGameState* UCTUserWidget::GetCTGameState() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetGameState<ACTGameState>() : nullptr;
}

ACTPlayerState* UCTUserWidget::GetOwningCTPlayerState() const
{
	const APlayerController* PlayerController = GetOwningPlayer();
	return PlayerController ? PlayerController->GetPlayerState<ACTPlayerState>() : nullptr;
}

ACTCharacter* UCTUserWidget::GetOwningCTCharacter() const
{
	return Cast<ACTCharacter>(GetOwningPlayerPawn());
}

UTexture2D* UCTUserWidget::GetColorIcon(ECTColor Color) const
{
	const TObjectPtr<UTexture2D>* Found = ColorIcons.Find(Color);
	return Found ? Found->Get() : nullptr;
}

void UCTUserWidget::ApplyColorToWidgets(ECTColor Color, UTextBlock* NameText, UImage* IconImage, FText NoneText) const
{
	const bool bHasColor = Color != ECTColor::None;
	const FLinearColor LinearColor = ACTPlayerState::GetLinearColorOf(Color);

	if (NameText)
	{
		NameText->SetText(bHasColor ? ACTPlayerState::GetColorDisplayName(Color) : NoneText);
		NameText->SetColorAndOpacity(FSlateColor(bHasColor ? LinearColor : FLinearColor::White));
	}

	if (IconImage)
	{
		if (!bHasColor)
		{
			IconImage->SetVisibility(ESlateVisibility::Collapsed);
			return;
		}

		IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);

		// 아이콘을 지정하지 않았으면 기본 이미지(사각형)에 색만 입힙니다.
		if (UTexture2D* Icon = GetColorIcon(Color))
		{
			IconImage->SetBrushFromTexture(Icon);
		}
		IconImage->SetColorAndOpacity(bTintColorIcon ? LinearColor : FLinearColor::White);
	}
}

FText UCTUserWidget::FormatTime(float Seconds)
{
	// 0.2초 남았을 때 "0:00"이 아니라 "0:01"로 보이도록 올림합니다.
	const int32 TotalSeconds = FMath::Max(0, FMath::CeilToInt(Seconds));
	return FText::FromString(FString::Printf(TEXT("%d:%02d"), TotalSeconds / 60, TotalSeconds % 60));
}

TArray<ACTPlayerState*> UCTUserWidget::GetSortedPlayers(const ACTGameState* GameState)
{
	TArray<ACTPlayerState*> Result;
	if (!GameState) { return Result; }

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		ACTPlayerState* CTPlayerState = Cast<ACTPlayerState>(PlayerState);
		if (CTPlayerState && !CTPlayerState->IsOnlyASpectator())
		{
			Result.Add(CTPlayerState);
		}
	}

	Result.Sort([](const ACTPlayerState& A, const ACTPlayerState& B)
	{
		if (A.GetTailCount() != B.GetTailCount())
		{
			return A.GetTailCount() > B.GetTailCount();
		}
		return A.GetPlayerName() < B.GetPlayerName();
	});

	return Result;
}

void UCTUserWidget::InheritColorIcons(const UCTUserWidget* From)
{
	if (From && ColorIcons.IsEmpty())
	{
		ColorIcons = From->ColorIcons;
		bTintColorIcon = From->bTintColorIcon;
	}
}
