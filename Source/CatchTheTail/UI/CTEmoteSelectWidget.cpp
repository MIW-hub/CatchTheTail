// Copyright Moon Party Games. All Rights Reserved.
#include "CTEmoteSelectWidget.h"
#include "../Player/CTCharacter.h"
#include "../Core/CTSettingsSubsystem.h"
#include "Engine/GameInstance.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

// 이모트 한 칸

void UCTEmoteCellWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (Button_Cell) { Button_Cell->OnClicked.AddDynamic(this, &UCTEmoteCellWidget::HandleClicked); }
	if (Widget_Selected) { Widget_Selected->SetVisibility(ESlateVisibility::Collapsed); }
}

void UCTEmoteCellWidget::Init(int32 InIndex, const FText& KeyText)
{
	Index = InIndex;
	if (Text_Key)
	{
		Text_Key->SetText(KeyText);
		Text_Key->SetVisibility(KeyText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UCTEmoteCellWidget::SetIcon(UTexture2D* Texture, FVector2D UVMin, FVector2D UVMax)
{
	if (!Image_Icon) { return; }
	if (!Texture)
	{
		Image_Icon->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// 크기와 비율은 WBP 에서 정한 값을 그대로 쓰고, 그림과 잘라낼 칸만 바꿈
	FSlateBrush Brush = Image_Icon->GetBrush();
	Brush.SetResourceObject(Texture);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.SetUVRegion(FBox2f(FVector2f(UVMin), FVector2f(UVMax)));
	Image_Icon->SetBrush(Brush);
	Image_Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UCTEmoteCellWidget::SetSelected(bool bSelected)
{
	if (Widget_Selected) { Widget_Selected->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }
	BP_OnSelectedChanged(bSelected);
}

void UCTEmoteCellWidget::HandleClicked()
{
	OnCellClicked.Broadcast(Index);
}

// 이모트 고르는 창

UCTEmoteSelectWidget::UCTEmoteSelectWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 기본 칸 모양. 창 WBP 의 Class Defaults 에서 바꿀 수 있음
	static ConstructorHelpers::FClassFinder<UCTEmoteCellWidget> CellFinder(TEXT("/Game/UI/WBP_CTEmoteCellWidget"));
	if (CellFinder.Succeeded()) { EmoteCellClass = CellFinder.Class; }
}

void UCTEmoteSelectWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (Button_Close) { Button_Close->OnClicked.AddDynamic(this, &UCTEmoteSelectWidget::HandleCloseClicked); }
	SetIsFocusable(true);
}

bool UCTEmoteSelectWidget::Setup(ACTCharacter* InCharacter)
{
	if (!InCharacter || !EmoteCellClass) { return false; }
	if (!SlotCells.IsEmpty()) { return true; }
	Character = InCharacter;

	// 숫자 키 자리
	if (Row_EmoteSlots)
	{
		Row_EmoteSlots->ClearChildren();
		const int32 SlotCount = Character->GetEmoteLoadout().Num();
		for (int32 Index = 0; Index < SlotCount; ++Index)
		{
			UCTEmoteCellWidget* Cell = CreateWidget<UCTEmoteCellWidget>(GetOwningPlayer(), EmoteCellClass);
			if (!Cell) { continue; }
			Cell->Init(Index, FText::AsNumber(Index + 1));
			Cell->OnCellClicked.AddDynamic(this, &UCTEmoteSelectWidget::HandleSlotClicked);
			Row_EmoteSlots->AddChildToHorizontalBox(Cell);
			SlotCells.Add(Cell);
		}
	}

	// 전체 이모트 목록
	if (Grid_EmoteLibrary)
	{
		Grid_EmoteLibrary->ClearChildren();
		const TArray<FCTEmoteSlot>& Library = Character->GetEmoteLibrary();
		const int32 Columns = FMath::Max(1, LibraryColumns);
		for (int32 Index = 0; Index < Library.Num(); ++Index)
		{
			UCTEmoteCellWidget* Cell = CreateWidget<UCTEmoteCellWidget>(GetOwningPlayer(), EmoteCellClass);
			if (!Cell) { continue; }
			Cell->Init(Index, FText::GetEmpty());
			FVector2D UVMin, UVMax;
			Library[Index].GetUVRegion(UVMin, UVMax);
			Cell->SetIcon(Library[Index].GetTexture(), UVMin, UVMax);
			Cell->OnCellClicked.AddDynamic(this, &UCTEmoteSelectWidget::HandleLibraryClicked);
			Grid_EmoteLibrary->AddChildToUniformGrid(Cell, Index / Columns, Index % Columns);
			LibraryCells.Add(Cell);
		}
	}

	RefreshSlots();
	RefreshSelection();
	return true;
}

void UCTEmoteSelectWidget::RefreshSlots()
{
	if (!Character) { return; }
	const TArray<int32>& Loadout = Character->GetEmoteLoadout();
	const TArray<FCTEmoteSlot>& Library = Character->GetEmoteLibrary();
	for (int32 Index = 0; Index < SlotCells.Num(); ++Index)
	{
		if (!Loadout.IsValidIndex(Index) || !Library.IsValidIndex(Loadout[Index])) { continue; }
		FVector2D UVMin, UVMax;
		Library[Loadout[Index]].GetUVRegion(UVMin, UVMax);
		SlotCells[Index]->SetIcon(Library[Loadout[Index]].GetTexture(), UVMin, UVMax);
	}
}

void UCTEmoteSelectWidget::RefreshSelection()
{
	for (int32 Index = 0; Index < SlotCells.Num(); ++Index)
	{
		SlotCells[Index]->SetSelected(Index == SelectedSlot);
	}
}

void UCTEmoteSelectWidget::HandleSlotClicked(int32 Index)
{
	SelectedSlot = Index;
	RefreshSelection();
}

void UCTEmoteSelectWidget::HandleLibraryClicked(int32 Index)
{
	if (!Character || SlotCells.IsEmpty()) { return; }
	Character->SetEmoteLoadout(SelectedSlot, Index);
	RefreshSlots();

	// 등록하면 다음 자리로 넘어감 (마지막이면 그대로)
	SelectedSlot = FMath::Min(SelectedSlot + 1, SlotCells.Num() - 1);
	RefreshSelection();
}

void UCTEmoteSelectWidget::HandleCloseClicked()
{
	OnCloseRequested.Broadcast();
}

FReply UCTEmoteSelectWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	// 이모트 창 키(설정에서 바꾼 키, 기본 R)를 계속 누르고 있어도 열렸다 닫혔다 반복하지 않게 반복 입력은 무시
	FKey MenuKey = EKeys::R;
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UCTSettingsSubsystem* KeySettings = GameInstance->GetSubsystem<UCTSettingsSubsystem>())
		{
			MenuKey = KeySettings->GetCodeKey(CTCodeKeys::EmoteMenu);
		}
	}

	if (Key == MenuKey || Key == EKeys::Escape)
	{
		if (InKeyEvent.IsRepeat()) { return FReply::Handled(); }
		OnCloseRequested.Broadcast();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
