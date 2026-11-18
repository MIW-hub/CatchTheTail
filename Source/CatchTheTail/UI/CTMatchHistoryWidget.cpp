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

	// 보물찾기: 1칸 = A팀, 2칸 = B팀 (이긴 쪽 "승", 진 쪽 "패", 같으면 "무"), 칸 오른쪽에 팀 승점. 3칸은 비움
	if (Record.bTreasure)
	{
		const int32 Wins[2] = { Record.WinsA, Record.WinsB };
		const TCHAR* const TeamNames[2] = { TEXT("A"), TEXT("B") };
		UTextBlock* const TeamNameTexts[2] = { Text_Name1, Text_Name2 };
		UTextBlock* const TeamWinTexts[2] = { Text_Tail1, Text_Tail2 };
		UImage* const TeamCells[2] = { Image_Cell1, Image_Cell2 };
		for (int32 Team = 0; Team < 2; ++Team)
		{
			const FText Result = (Wins[0] == Wins[1]) ? NSLOCTEXT("CT", "UI_History_Draw", "무") : (Wins[Team] > Wins[1 - Team] ? NSLOCTEXT("CT", "UI_History_Win", "승") : NSLOCTEXT("CT", "UI_History_Lose", "패"));
			if (TeamNameTexts[Team]) { TeamNameTexts[Team]->SetText(FText::Format(NSLOCTEXT("CT", "UI_History_TeamLine", "{0} {1}팀"), Result, FText::FromString(TeamNames[Team]))); }
			if (TeamWinTexts[Team]) { TeamWinTexts[Team]->SetText(FText::AsNumber(Wins[Team])); }
			if (TeamCells[Team]) { TeamCells[Team]->SetColorAndOpacity(Record.MyTeam == Team ? MyCellColor : OtherCellColor); }
		}
		if (Text_Name3) { Text_Name3->SetText(FText::GetEmpty()); }
		if (Text_Tail3) { Text_Tail3->SetText(FText::GetEmpty()); }
		if (Image_Cell3) { Image_Cell3->SetColorAndOpacity(OtherCellColor); }
		return;
	}

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
