// Copyright Moon Party Games. All Rights Reserved.
#include "CTEmoteSelectWidget.h"
#include "../Player/CTCharacter.h"
#include "../Core/CTSettingsSubsystem.h"
#include "Engine/GameInstance.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "CTEmoteSelect"

namespace
{
	const FLinearColor Magenta = FLinearColor::FromSRGBColor(FColor(0xFF, 0x27, 0x83));
	const FLinearColor DarkPurple(0.058f, 0.0006f, 0.032f, 1.f);
	constexpr int32 LibraryColumns = 9;

	// 둥근 상자 그림 (채움색 + 테두리)
	FSlateBrush MakeRoundBrush(const FLinearColor& Fill, const FLinearColor& Outline, float OutlineWidth, float Radius)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(Radius, Radius, Radius, Radius), FSlateColor(Outline), OutlineWidth);
		return Brush;
	}

	void StyleText(UTextBlock* Text, int32 Size, const FLinearColor& Color)
	{
		Text->SetColorAndOpacity(FSlateColor(Color));
		if (UFont* Font = LoadObject<UFont>(nullptr, TEXT("/Game/Font/NotoSansKR-Medium_Font.NotoSansKR-Medium_Font")))
		{
			Text->SetFont(FSlateFontInfo(Font, Size, FName(TEXT("Default"))));
		}
	}
}

// 칸 하나

void UCTEmoteCellWidget::Build(int32 InIndex, float Size, const FText& CornerText)
{
	if (!WidgetTree || WidgetTree->RootWidget) { return; }
	Index = InIndex;

	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size_Cell"));
	Box->SetWidthOverride(Size);
	Box->SetHeightOverride(Size);
	WidgetTree->RootWidget = Box;

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Overlay_Cell"));
	Box->SetContent(Overlay);

	Image_Back = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_CellBack"));
	Image_Back->SetVisibility(ESlateVisibility::HitTestInvisible);
	Image_Back->SetBrush(MakeRoundBrush(FLinearColor(1.f, 1.f, 1.f, 0.92f), FLinearColor(0.8f, 0.8f, 0.8f, 1.f), 2.f, 14.f));
	UOverlaySlot* BackSlot = Overlay->AddChildToOverlay(Image_Back);
	BackSlot->SetHorizontalAlignment(HAlign_Fill);
	BackSlot->SetVerticalAlignment(VAlign_Fill);

	Image_Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_CellIcon"));
	Image_Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
	UOverlaySlot* IconSlot = Overlay->AddChildToOverlay(Image_Icon);
	IconSlot->SetHorizontalAlignment(HAlign_Fill);
	IconSlot->SetVerticalAlignment(VAlign_Fill);
	IconSlot->SetPadding(FMargin(4.f));

	if (!CornerText.IsEmpty())
	{
		UTextBlock* Corner = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_CellKey"));
		Corner->SetText(CornerText);
		Corner->SetVisibility(ESlateVisibility::HitTestInvisible);
		StyleText(Corner, 22, Magenta);
		Corner->SetShadowOffset(FVector2D(1.f, 1.f));
		Corner->SetShadowColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.9f));
		UOverlaySlot* CornerSlot = Overlay->AddChildToOverlay(Corner);
		CornerSlot->SetHorizontalAlignment(HAlign_Left);
		CornerSlot->SetVerticalAlignment(VAlign_Top);
		CornerSlot->SetPadding(FMargin(8.f, 2.f, 0.f, 0.f));
	}

	// 투명 버튼을 맨 위에 덮어 눌림만 받음
	Button_Cell = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_Cell"));
	FButtonStyle Style = Button_Cell->GetStyle();
	Style.Normal = FSlateNoResource();
	Style.Hovered = MakeRoundBrush(FLinearColor(1.f, 1.f, 1.f, 0.25f), FLinearColor::Transparent, 0.f, 14.f);
	Style.Pressed = MakeRoundBrush(FLinearColor(0.f, 0.f, 0.f, 0.15f), FLinearColor::Transparent, 0.f, 14.f);
	Button_Cell->SetStyle(Style);
	Button_Cell->OnClicked.AddDynamic(this, &UCTEmoteCellWidget::HandleClicked);
	UOverlaySlot* ButtonSlot = Overlay->AddChildToOverlay(Button_Cell);
	ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
	ButtonSlot->SetVerticalAlignment(VAlign_Fill);
}

void UCTEmoteCellWidget::SetIcon(UTexture2D* Texture, FVector2D UVMin, FVector2D UVMax)
{
	if (!Image_Icon) { return; }
	if (!Texture)
	{
		Image_Icon->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	FSlateBrush Brush;
	Brush.SetResourceObject(Texture);
	Brush.ImageSize = FVector2D(96.f, 96.f);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.SetUVRegion(FBox2f(FVector2f(UVMin), FVector2f(UVMax)));
	Image_Icon->SetBrush(Brush);
	Image_Icon->SetColorAndOpacity(FLinearColor::White);
	Image_Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UCTEmoteCellWidget::SetSelected(bool bSelected)
{
	if (!Image_Back) { return; }
	Image_Back->SetBrush(bSelected
		? MakeRoundBrush(FLinearColor(1.f, 0.88f, 0.94f, 1.f), Magenta, 5.f, 14.f)
		: MakeRoundBrush(FLinearColor(1.f, 1.f, 1.f, 0.92f), FLinearColor(0.8f, 0.8f, 0.8f, 1.f), 2.f, 14.f));
}

void UCTEmoteCellWidget::HandleClicked()
{
	OnCellClicked.Broadcast(Index);
}

// 이모트 고르는 창

void UCTEmoteSelectWidget::Setup(ACTCharacter* InCharacter)
{
	if (!WidgetTree || WidgetTree->RootWidget || !InCharacter) { return; }
	Character = InCharacter;

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas_EmoteSelect"));
	WidgetTree->RootWidget = Canvas;

	// 뒤를 어둡게
	UImage* Dim = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_EmoteDim"));
	Dim->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.45f));
	UCanvasPanelSlot* DimSlot = Canvas->AddChildToCanvas(Dim);
	DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	DimSlot->SetOffsets(FMargin(0.f));

	// 가운데 패널
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_EmotePanel"));
	Panel->SetBrush(MakeRoundBrush(FLinearColor(1.f, 0.96f, 0.98f, 1.f), Magenta, 6.f, 28.f));
	Panel->SetPadding(FMargin(32.f, 24.f));
	UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PanelSlot->SetAutoSize(true);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column_Emote"));
	Panel->SetContent(Column);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_EmoteTitle"));
	Title->SetText(LOCTEXT("Title", "이모트 설정"));
	StyleText(Title, 34, DarkPurple);
	UVerticalBoxSlot* TitleSlot = Column->AddChildToVerticalBox(Title);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);

	Text_Selected = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_EmoteHint"));
	Text_Selected->SetText(LOCTEXT("Hint", "숫자 키 자리를 고른 뒤, 아래에서 넣을 이모트를 누르세요"));
	StyleText(Text_Selected, 18, DarkPurple);
	UVerticalBoxSlot* HintSlot = Column->AddChildToVerticalBox(Text_Selected);
	HintSlot->SetHorizontalAlignment(HAlign_Center);
	HintSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 14.f));

	// 숫자 키 자리
	UHorizontalBox* SlotRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row_EmoteSlots"));
	UVerticalBoxSlot* SlotRowSlot = Column->AddChildToVerticalBox(SlotRow);
	SlotRowSlot->SetHorizontalAlignment(HAlign_Center);
	SlotRowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));

	const int32 SlotCount = Character->GetEmoteLoadout().Num();
	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		UCTEmoteCellWidget* Cell = CreateWidget<UCTEmoteCellWidget>(GetOwningPlayer(), UCTEmoteCellWidget::StaticClass());
		Cell->Build(Index, 120.f, FText::AsNumber(Index + 1));
		Cell->OnCellClicked.AddDynamic(this, &UCTEmoteSelectWidget::HandleSlotClicked);
		UHorizontalBoxSlot* CellSlot = SlotRow->AddChildToHorizontalBox(Cell);
		CellSlot->SetPadding(FMargin(8.f, 0.f));
		SlotCells.Add(Cell);
	}

	// 전체 이모트 목록
	UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("Grid_EmoteLibrary"));
	Grid->SetSlotPadding(FMargin(5.f));
	UVerticalBoxSlot* GridSlot = Column->AddChildToVerticalBox(Grid);
	GridSlot->SetHorizontalAlignment(HAlign_Center);

	const TArray<FCTEmoteSlot>& Library = Character->GetEmoteLibrary();
	for (int32 Index = 0; Index < Library.Num(); ++Index)
	{
		UCTEmoteCellWidget* Cell = CreateWidget<UCTEmoteCellWidget>(GetOwningPlayer(), UCTEmoteCellWidget::StaticClass());
		Cell->Build(Index, 96.f, FText::GetEmpty());
		FVector2D UVMin, UVMax;
		Library[Index].GetUVRegion(UVMin, UVMax);
		Cell->SetIcon(Library[Index].GetTexture(), UVMin, UVMax);
		Cell->OnCellClicked.AddDynamic(this, &UCTEmoteSelectWidget::HandleLibraryClicked);
		Grid->AddChildToUniformGrid(Cell, Index / LibraryColumns, Index % LibraryColumns);
		LibraryCells.Add(Cell);
	}

	// 닫기
	UButton* Close = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_EmoteClose"));
	Close->SetBackgroundColor(Magenta);
	Close->OnClicked.AddDynamic(this, &UCTEmoteSelectWidget::HandleCloseClicked);
	UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_EmoteClose"));
	CloseText->SetText(LOCTEXT("Close", "닫기 (R)"));
	StyleText(CloseText, 22, FLinearColor::White);
	Close->SetContent(CloseText);
	UVerticalBoxSlot* CloseSlot = Column->AddChildToVerticalBox(Close);
	CloseSlot->SetHorizontalAlignment(HAlign_Center);
	CloseSlot->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));

	SetIsFocusable(true);
	RefreshSlots();
	RefreshSelection();
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
	if (!Character) { return; }
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

#undef LOCTEXT_NAMESPACE
