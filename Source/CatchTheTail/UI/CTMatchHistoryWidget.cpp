// Copyright Moon Party Games. All Rights Reserved.
#include "CTMatchHistoryWidget.h"
#include "CTButtonHoverEffect.h"
#include "../Core/CTAudioSubsystem.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "UObject/ConstructorHelpers.h"

void UCTMatchHistoryEntryWidget::SetRecord(const FCTMatchRecord& Record, int32 GameNumber)
{
	if (Text_Index) { Text_Index->SetText(FText::AsNumber(GameNumber)); }
	if (Text_MapName) { Text_MapName->SetText(FText::FromString(Record.MapName)); }

	UTextBlock* const Names[3] = { Text_Name1, Text_Name2, Text_Name3 };
	UTextBlock* const Tails[3] = { Text_Tail1, Text_Tail2, Text_Tail3 };
	UImage* const Cells[3] = { Image_Cell1, Image_Cell2, Image_Cell3 };
	for (int32 Place = 0; Place < 3; ++Place)
	{
		// 이 등수(Place+1)인 사람을 모두 겹쳐서 적음 (동점이면 한 칸에 여럿)
		TArray<FString> NamesInCell;
		int32 CellTail = 0;
		bool bMine = false;
		for (const FCTMatchRecordEntry& Entry : Record.Entries)
		{
			if (Entry.Rank != Place + 1) { continue; }
			NamesInCell.Add(Entry.PlayerName);
			CellTail = Entry.TailCount;
			bMine |= Entry.bIsMe;
		}
		const bool bHas = !NamesInCell.IsEmpty();
		if (Names[Place]) { Names[Place]->SetText(FText::FromString(bHas ? FString::Join(NamesInCell, TEXT("\n")) : FString(TEXT("-")))); }
		if (Tails[Place]) { Tails[Place]->SetText(bHas ? FText::AsNumber(CellTail) : FText::GetEmpty()); }
		if (Cells[Place]) { Cells[Place]->SetColorAndOpacity(bMine ? MyCellColor : OtherCellColor); }
	}
}

UCTMatchHistoryWidget::UCTMatchHistoryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FClassFinder<UCTMatchHistoryEntryWidget> EntryFinder(TEXT("/Game/UI/WBP_CTMatchHistoryEntryWidget"));
	if (EntryFinder.Succeeded())
	{
		EntryClass = EntryFinder.Class;
	}
}

void UCTMatchHistoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UCTButtonHoverEffect::Attach(this);
	UCTAudioSubsystem::ApplyUISounds(this);

	if (Button_Close) { Button_Close->OnClicked.AddUniqueDynamic(this, &UCTMatchHistoryWidget::HandleCloseClicked); }

	SetIsFocusable(true);
	RefreshHistory();
}

FReply UCTMatchHistoryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		HandleCloseClicked();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UCTMatchHistoryWidget::RefreshHistory()
{
	const TArray<FCTMatchRecord> Records = UCTMatchHistory::LoadRecords();

	if (Text_Empty)
	{
		Text_Empty->SetVisibility(Records.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (Text_Total)
	{
		int32 FirstPlaceCount = 0;
		for (const FCTMatchRecord& Record : Records)
		{
			if (Record.MyRank == 1) { ++FirstPlaceCount; }
		}
		Text_Total->SetText(FText::Format(TotalText, Records.Num(), FirstPlaceCount));
		Text_Total->SetVisibility(Records.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (!Scroll_History || !EntryClass) { return; }

	Scroll_History->ClearChildren();
	for (int32 Index = 0; Index < Records.Num(); ++Index)
	{
		const FCTMatchRecord& Record = Records[Index];
		if (UCTMatchHistoryEntryWidget* Entry = CreateWidget<UCTMatchHistoryEntryWidget>(this, EntryClass))
		{
			Scroll_History->AddChild(Entry);
			Entry->SetRecord(Record, Records.Num() - Index);
		}
	}
}

void UCTMatchHistoryWidget::HandleCloseClicked()
{
	RemoveFromParent();
}
