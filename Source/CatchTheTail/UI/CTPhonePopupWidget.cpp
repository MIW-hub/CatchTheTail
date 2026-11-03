// Copyright Moon Party Games. All Rights Reserved.
#include "CTPhonePopupWidget.h"
#include "../Core/CTGameState.h"
#include "../Player/CTPlayerState.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "InputCoreTypes.h"

namespace
{
	const FLinearColor TextMain(0.92f, 0.95f, 1.f, 1.f);
	const FLinearColor TextDim(0.5f, 0.62f, 0.74f, 1.f);
	const FLinearColor TextTitle(0.1f, 0.8f, 1.f, 1.f);
	const FLinearColor CardColor(0.025f, 0.04f, 0.075f, 0.96f);

	FSlateFontInfo MakeFont(const TCHAR* Weight, int32 Size)
	{
		const FString Path = FString::Printf(TEXT("/Game/Font/NotoSansKR-%s_Font.NotoSansKR-%s_Font"), Weight, Weight);
		UFont* Font = LoadObject<UFont>(nullptr, *Path);
		return FSlateFontInfo(Font, Size, FName(TEXT("Default")));
	}

	FSlateBrush SolidBrush(const FLinearColor& Color)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.TintColor = Color;
		return Brush;
	}

	UTextBlock* MakeText(UWidgetTree* Tree, const TCHAR* Name, const FText& Source, const TCHAR* Weight, int32 Size, const FLinearColor& Color)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->SetText(Source);
		Text->SetFont(MakeFont(Weight, Size));
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetJustification(ETextJustify::Center);
		return Text;
	}

	UButton* MakeButton(UWidgetTree* Tree, const TCHAR* Name, const FText& Label, const FLinearColor& Color)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);

		FButtonStyle Style;
		Style.SetNormal(SolidBrush(Color));
		Style.SetHovered(SolidBrush(Color * 1.35f));
		Style.SetPressed(SolidBrush(Color * 0.7f));
		Style.SetDisabled(SolidBrush(FLinearColor(0.2f, 0.22f, 0.26f, 0.9f)));
		Button->SetStyle(Style);

		UTextBlock* Text = MakeText(Tree, *FString::Printf(TEXT("Label_%s"), Name), Label, TEXT("SemiBold"), 24, FLinearColor::White);
		Text->SetMinDesiredWidth(440.f);
		if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Button->AddChild(Text)))
		{
			ButtonSlot->SetPadding(FMargin(24.f, 16.f));
		}
		return Button;
	}
}

void UCTPhonePopupWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// WBP 자식이 루트를 정해 두었으면 그대로 씀
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
}

void UCTPhonePopupWidget::BuildDefaultLayout()
{
	// 화면 전체를 살짝 어둡게 덮고 가운데에 카드
	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_Dim"));
	Dim->SetBrush(SolidBrush(FLinearColor::White));
	Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.35f));
	Dim->SetHorizontalAlignment(HAlign_Center);
	Dim->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Dim;

	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_Card"));
	Card->SetBrush(SolidBrush(FLinearColor::White));
	Card->SetBrushColor(CardColor);
	Card->SetPadding(FMargin(40.f, 32.f));
	Dim->SetContent(Card);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VerticalBox_Phone"));
	Card->SetContent(Column);

	auto AddRow = [Column](UWidget* Child, float Bottom)
	{
		UVerticalBoxSlot* RowSlot = Column->AddChildToVerticalBox(Child);
		RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, Bottom));
		RowSlot->SetHorizontalAlignment(HAlign_Fill);
	};

	AddRow(MakeText(WidgetTree, TEXT("Text_Title"), NSLOCTEXT("CT", "Phone_Title", "전화 걸기"), TEXT("Bold"), 36, TextTitle), 6.f);
	Text_Status = MakeText(WidgetTree, TEXT("Text_Status"), FText::GetEmpty(), TEXT("Medium"), 20, TextDim);
	AddRow(Text_Status, 22.f);

	Button_Target = MakeButton(WidgetTree, TEXT("Button_Target"), NSLOCTEXT("CT", "Phone_AskTarget", "좌클릭: 목표 위치 확인"), FLinearColor(0.05f, 0.32f, 0.5f, 1.f));
	AddRow(Button_Target, 14.f);
	Button_Hunter = MakeButton(WidgetTree, TEXT("Button_Hunter"), NSLOCTEXT("CT", "Phone_AskHunter", "우클릭: 헌터 위치 확인"), FLinearColor(0.5f, 0.2f, 0.1f, 1.f));
	AddRow(Button_Hunter, 18.f);

	AddRow(MakeText(WidgetTree, TEXT("Text_Hint"), NSLOCTEXT("CT", "Phone_CloseHint", "F 또는 ESC: 닫기"), TEXT("Regular"), 18, TextDim), 0.f);
}

void UCTPhonePopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 버튼은 모양만 보여 줌. 선택은 마우스 좌클릭/우클릭 입력 자체로 받으므로 버튼이 클릭을 가로채지 않게 함
	if (Button_Target) { Button_Target->SetVisibility(ESlateVisibility::HitTestInvisible); }
	if (Button_Hunter) { Button_Hunter->SetVisibility(ESlateVisibility::HitTestInvisible); }

	RefreshStatus();
}

void UCTPhonePopupWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshStatus();
}

void UCTPhonePopupWidget::RefreshStatus()
{
	const ACTPlayerState* MyState = GetOwningCTPlayerState();
	const ACTGameState* CTGameState = GetCTGameState();

	const double Remaining = (MyState && CTGameState) ? MyState->GetPhoneReadyServerTime() - CTGameState->GetServerWorldTimeSeconds() : 0.0;
	bCanCall = Remaining <= 0.0;

	if (Text_Status)
	{
		Text_Status->SetText(bCanCall
			? NSLOCTEXT("CT", "Phone_Ready", "지금 통화할 수 있습니다")
			: FText::Format(NSLOCTEXT("CT", "Phone_NextCall", "다음 통화까지 {0}초"), FText::AsNumber(FMath::CeilToInt(Remaining))));
		Text_Status->SetColorAndOpacity(FSlateColor(bCanCall ? TextMain : FLinearColor(1.f, 0.75f, 0.2f, 1.f)));
	}
	if (Button_Target) { Button_Target->SetIsEnabled(bCanCall); }
	if (Button_Hunter) { Button_Hunter->SetIsEnabled(bCanCall); }
}

FReply UCTPhonePopupWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 팝업 어디를 눌러도 좌클릭은 목표, 우클릭은 헌터. 게임 입력(잡기, 밀기)으로는 넘어가지 않음
	const FKey Pressed = InMouseEvent.GetEffectingButton();
	if (Pressed == EKeys::LeftMouseButton)
	{
		Select(ECTPhoneQuery::Target);
	}
	else if (Pressed == EKeys::RightMouseButton)
	{
		Select(ECTPhoneQuery::Hunter);
	}
	return FReply::Handled();
}

void UCTPhonePopupWidget::Select(ECTPhoneQuery Query)
{
	if (!bCanCall) { return; }
	OnQuerySelected.Broadcast(Query);
}
