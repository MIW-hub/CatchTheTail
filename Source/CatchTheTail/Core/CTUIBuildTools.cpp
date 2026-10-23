// Copyright Moon Party Games. All Rights Reserved.
#include "CTUIBuildTools.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/Slider.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Blueprint.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Internationalization/TextKey.h"

#if WITH_EDITOR
namespace
{
	const TCHAR* BlueprintName = TEXT("WBP_CTSettingsWidget");

	// 색 (붙여 넣은 디자인 기준)
	const FLinearColor TextMain(0.92f, 0.95f, 1.f, 1.f);
	const FLinearColor TextDim(0.5f, 0.62f, 0.74f, 1.f);
	const FLinearColor TextTitle(0.1f, 0.8f, 1.f, 1.f);
	const FLinearColor TextWarn(1.f, 0.75f, 0.2f, 1.f);
	const FLinearColor CardColor(0.025f, 0.04f, 0.075f, 1.f);

	UWidgetTree* Tree = nullptr;
	int32 WidgetCount = 0;

	template <class T>
	T* Make(const TCHAR* Name)
	{
		T* Widget = Tree->ConstructWidget<T>(T::StaticClass(), FName(Name));
		Widget->bIsVariable = true;
		++WidgetCount;
		return Widget;
	}

	// 번역 키가 붙은 글자 (이미 있는 번역표의 원문과 같은 글자를 씀)
	FText Loc(const TCHAR* WidgetName, const TCHAR* Source, bool bHint = false)
	{
		const FString Key = FString::Printf(TEXT("%s.%s%s"), BlueprintName, WidgetName, bHint ? TEXT(".Hint") : TEXT(""));
		return FText::ChangeKey(FTextKey(TEXT("CT_UI")), FTextKey(Key), FText::FromString(Source));
	}

	FSlateFontInfo MakeFont(const TCHAR* Weight, int32 Size)
	{
		const FString Path = FString::Printf(TEXT("/Game/Font/NotoSansKR-%s_Font.NotoSansKR-%s_Font"), Weight, Weight);
		UFont* Font = LoadObject<UFont>(nullptr, *Path);
		return FSlateFontInfo(Font, Size, FName(TEXT("Default")));
	}

	UTextBlock* MakeText(const TCHAR* Name, const TCHAR* Source, const TCHAR* Weight, int32 Size, const FLinearColor& Color, ETextJustify::Type Justify = ETextJustify::Left)
	{
		UTextBlock* Text = Make<UTextBlock>(Name);
		Text->SetText(Loc(Name, Source));
		Text->SetFont(MakeFont(Weight, Size));
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetVisibility(ESlateVisibility::HitTestInvisible);
		Text->SetAutoWrapText(true);
		Text->SetJustification(Justify);
		return Text;
	}

	// 서로 다른 칸의 위젯을 담는 도우미
	UVerticalBoxSlot* AddV(UVerticalBox* Box, UWidget* Child, float BottomPadding, bool bFill = false)
	{
		UVerticalBoxSlot* BoxSlot = Box->AddChildToVerticalBox(Child);
		BoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, BottomPadding));
		BoxSlot->SetHorizontalAlignment(HAlign_Fill);
		BoxSlot->SetSize(FSlateChildSize(bFill ? ESlateSizeRule::Fill : ESlateSizeRule::Automatic));
		return BoxSlot;
	}

	UHorizontalBoxSlot* AddH(UHorizontalBox* Box, UWidget* Child, float RightPadding, bool bFill, EVerticalAlignment VAlign = VAlign_Center)
	{
		UHorizontalBoxSlot* BoxSlot = Box->AddChildToHorizontalBox(Child);
		BoxSlot->SetPadding(FMargin(0.f, 0.f, RightPadding, 0.f));
		BoxSlot->SetVerticalAlignment(VAlign);
		BoxSlot->SetSize(FSlateChildSize(bFill ? ESlateSizeRule::Fill : ESlateSizeRule::Automatic));
		return BoxSlot;
	}

	// 크기를 정한 상자. Width 또는 Height가 0이면 그 방향은 정하지 않음
	USizeBox* Sized(const TCHAR* Name, UWidget* Child, float Width, float Height)
	{
		USizeBox* Box = Make<USizeBox>(Name);
		if (Width > 0.f) { Box->SetWidthOverride(Width); }
		if (Height > 0.f) { Box->SetHeightOverride(Height); }
		Box->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		USizeBoxSlot* BoxSlot = Cast<USizeBoxSlot>(Box->AddChild(Child));
		BoxSlot->SetHorizontalAlignment(HAlign_Fill);
		BoxSlot->SetVerticalAlignment(VAlign_Fill);
		return Box;
	}

	UBorder* MakeCard(const TCHAR* Name, UWidget* Child, const FLinearColor& Color, const FMargin& Padding)
	{
		UBorder* Border = Make<UBorder>(Name);
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.TintColor = FLinearColor::White;
		Border->SetBrush(Brush);
		Border->SetBrushColor(Color);
		Border->SetPadding(Padding);
		Border->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UBorderSlot* BorderSlot = Cast<UBorderSlot>(Border->AddChild(Child));
		BorderSlot->SetHorizontalAlignment(HAlign_Fill);
		BorderSlot->SetVerticalAlignment(VAlign_Fill);
		return Border;
	}

	FSlateBrush ImageBrush(const TCHAR* TextureName, const FMargin& Margin, const FLinearColor& Tint)
	{
		FSlateBrush Brush;
		const FString Path = FString::Printf(TEXT("/Game/2D/UI/%s.%s"), TextureName, TextureName);
		Brush.SetResourceObject(LoadObject<UTexture2D>(nullptr, *Path));
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.Margin = Margin;
		Brush.TintColor = Tint;
		return Brush;
	}

	// 버튼/콤보 공통 상태 4종 (보통, 올림, 누름, 비활성)
	FButtonStyle MakeButtonStyle(const TCHAR* TextureName, const FMargin& Margin)
	{
		FButtonStyle Style;
		Style.SetNormal(ImageBrush(TextureName, Margin, FLinearColor(1.f, 1.f, 1.f, 1.f)));
		Style.SetHovered(ImageBrush(TextureName, Margin, FLinearColor(1.08f, 1.08f, 1.08f, 1.f)));
		Style.SetPressed(ImageBrush(TextureName, Margin, FLinearColor(0.8f, 0.8f, 0.8f, 1.f)));
		Style.SetDisabled(ImageBrush(TextureName, Margin, FLinearColor(0.45f, 0.45f, 0.45f, 0.7f)));
		Style.SetNormalPadding(FMargin(16.f, 6.f, 16.f, 6.f));
		Style.SetPressedPadding(FMargin(16.f, 8.f, 16.f, 4.f));
		return Style;
	}

	// 글자가 들어 있는 버튼
	UButton* MakeButton(const TCHAR* Name, const TCHAR* LabelName, const TCHAR* Source, const TCHAR* TextureName, const FMargin& Margin, const FLinearColor& LabelColor)
	{
		UButton* Button = Make<UButton>(Name);
		Button->SetStyle(MakeButtonStyle(TextureName, Margin));
		Button->SetVisibility(ESlateVisibility::Visible);
		UTextBlock* Label = MakeText(LabelName, Source, TEXT("SemiBold"), 20, LabelColor, ETextJustify::Center);
		UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Button->AddChild(Label));
		ButtonSlot->SetHorizontalAlignment(HAlign_Center);
		ButtonSlot->SetVerticalAlignment(VAlign_Center);
		return Button;
	}

	// 체크박스 한 줄: 높이 44 상자 안에 체크박스, 그 안에 글자
	USizeBox* MakeCheckRow(const TCHAR* SizeName, const TCHAR* CheckName, const TCHAR* LabelName, const TCHAR* Source)
	{
		UCheckBox* CheckBox = Make<UCheckBox>(CheckName);
		CheckBox->SetVisibility(ESlateVisibility::Visible);
		CheckBox->HorizontalAlignment = HAlign_Left;
		CheckBox->AddChild(MakeText(LabelName, Source, TEXT("Regular"), 20, TextMain));
		return Sized(SizeName, CheckBox, 0.f, 44.f);
	}

	// 슬라이더 묶음: 이름 줄 + (슬라이더 | 퍼센트)
	UVerticalBox* MakeSliderGroup(const TCHAR* GroupName, const TCHAR* LabelName, const TCHAR* Source, const TCHAR* RowName, const TCHAR* HeightName, const TCHAR* SliderName, const TCHAR* WidthName, const TCHAR* ValueName)
	{
		UVerticalBox* Group = Make<UVerticalBox>(GroupName);
		Group->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		AddV(Group, MakeText(LabelName, Source, TEXT("Regular"), 19, TextDim), 8.f);

		UHorizontalBox* Row = Make<UHorizontalBox>(RowName);
		Row->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		USlider* Slider = Make<USlider>(SliderName);
		Slider->SetMinValue(0.f);
		Slider->SetMaxValue(1.f);
		Slider->SetValue(0.8f);
		AddH(Row, Sized(HeightName, Slider, 0.f, 36.f), 16.f, true);

		UTextBlock* Value = MakeText(ValueName, TEXT("80%"), TEXT("Regular"), 20, TextMain, ETextJustify::Right);
		AddH(Row, Sized(WidthName, Value, 68.f, 36.f), 0.f, false);

		AddV(Group, Row, 0.f);
		return Group;
	}
}
#endif

int32 UCTUIBuildTools::BuildSettingsWidgetDesign(UBlueprint* WidgetBlueprint)
{
#if WITH_EDITOR
	if (!WidgetBlueprint) { return 0; }

	// UWidgetBlueprint는 에디터 모듈에 있어서, 리플렉션으로 WidgetTree를 찾음
	const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
	Tree = TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
	if (!Tree) { return 0; }
	WidgetCount = 0;

	// 기존 트리를 모두 지움
	TArray<UWidget*> OldWidgets;
	Tree->GetAllWidgets(OldWidgets);
	for (UWidget* Old : OldWidgets) { Tree->RemoveWidget(Old); }
	Tree->RootWidget = nullptr;

	const FMargin ComboMargin(0.041237f, 0.235294f, 0.041237f, 0.235294f);
	const FMargin ButtonMarginA(0.041237f, 0.210526f, 0.041237f, 0.210526f);
	const FMargin ButtonMarginB(0.101266f, 0.192771f, 0.101266f, 0.192771f);

	// ===== 바깥 틀: 어두운 배경 + 화면에 맞춰 줄어드는 1440x920 창 =====
	UOverlay* Root = Make<UOverlay>(TEXT("Root_Settings"));
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Tree->RootWidget = Root;

	UImage* Dim = Make<UImage>(TEXT("Image_SettingsDim"));
	Dim->SetColorAndOpacity(FLinearColor(0.003f, 0.006f, 0.015f, 0.95f));
	Dim->SetVisibility(ESlateVisibility::HitTestInvisible);
	UOverlaySlot* DimSlot = Root->AddChildToOverlay(Dim);
	DimSlot->SetHorizontalAlignment(HAlign_Fill);
	DimSlot->SetVerticalAlignment(VAlign_Fill);

	UVerticalBox* Layout = Make<UVerticalBox>(TEXT("Layout_Settings"));
	Layout->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UBorder* Frame = MakeCard(TEXT("Frame_Settings"), Layout, FLinearColor(0.012f, 0.02f, 0.04f, 1.f), FMargin(32.f, 28.f, 32.f, 28.f));
	USizeBox* Design = Sized(TEXT("Size_SettingsDesign"), Frame, 1440.f, 920.f);

	UScaleBox* Scale = Make<UScaleBox>(TEXT("Scale_Settings"));
	Scale->SetStretch(EStretch::ScaleToFit);
	Scale->SetStretchDirection(EStretchDirection::DownOnly);
	Scale->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UScaleBoxSlot* ScaleSlot = Cast<UScaleBoxSlot>(Scale->AddChild(Design));
	ScaleSlot->SetHorizontalAlignment(HAlign_Center);
	ScaleSlot->SetVerticalAlignment(VAlign_Center);
	UOverlaySlot* ScaleOverlaySlot = Root->AddChildToOverlay(Scale);
	ScaleOverlaySlot->SetPadding(FMargin(24.f));
	ScaleOverlaySlot->SetHorizontalAlignment(HAlign_Fill);
	ScaleOverlaySlot->SetVerticalAlignment(VAlign_Fill);

	// ===== 머리글: 제목 + 안내 =====
	UHorizontalBox* Header = Make<UHorizontalBox>(TEXT("Row_SettingsHeader"));
	Header->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	AddH(Header, MakeText(TEXT("Label_SettingsTitle"), TEXT("설정"), TEXT("Bold"), 36, TextMain), 24.f, true);
	AddH(Header, MakeText(TEXT("Label_SettingsSubtitle"), TEXT("변경한 설정은 자동으로 저장됩니다"), TEXT("Regular"), 17, TextDim), 0.f, false);
	AddV(Layout, Sized(TEXT("Height_SettingsHeader"), Header, 0.f, 64.f), 16.f);

	// ===== 본문: 왼쪽 설정 카드 + 오른쪽 조작 카드 =====
	UHorizontalBox* Columns = Make<UHorizontalBox>(TEXT("Row_SettingsColumns"));
	Columns->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	AddV(Layout, Columns, 12.f, true);

	// -- 왼쪽: 스크롤되는 설정 목록
	UVerticalBox* List = Make<UVerticalBox>(TEXT("List_Settings"));
	List->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UScrollBox* ListScroll = Make<UScrollBox>(TEXT("Scroll_Settings"));
	ListScroll->SetVisibility(ESlateVisibility::Visible);
	ListScroll->SetOrientation(Orient_Vertical);
	ListScroll->AddChild(List);
	AddH(Columns, MakeCard(TEXT("Card_Settings"), ListScroll, CardColor, FMargin(24.f)), 24.f, true, VAlign_Fill);

	// 언어
	AddV(List, MakeText(TEXT("Label_Language"), TEXT("언어 / Language"), TEXT("Medium"), 24, TextTitle), 14.f);
	UComboBoxString* Language = Make<UComboBoxString>(TEXT("ComboBox_Language"));
	Language->SetVisibility(ESlateVisibility::Visible);
	Language->AddOption(TEXT("한국어"));
	Language->AddOption(TEXT("English"));
	Language->AddOption(TEXT("日本語"));
	Language->SetSelectedOption(TEXT("한국어"));
	Language->SetContentPadding(FMargin(16.f, 8.f, 16.f, 8.f));
	Language->MaxListHeight = 240.f;
	Language->Font = MakeFont(TEXT("Medium"), 20);
	Language->ForegroundColor = FSlateColor(FLinearColor::White);
	Language->WidgetStyle.ComboButtonStyle.ButtonStyle = MakeButtonStyle(TEXT("Rectangle_9"), ComboMargin);
	AddV(List, Sized(TEXT("Height_Language"), Language, 0.f, 56.f), 24.f);

	// 소리
	AddV(List, MakeText(TEXT("Label_Audio"), TEXT("소리"), TEXT("Medium"), 24, TextTitle), 14.f);
	AddV(List, MakeSliderGroup(TEXT("Group_Music"), TEXT("Label_Music"), TEXT("배경음"), TEXT("Row_Music"), TEXT("Height_Music"), TEXT("Slider_Music"), TEXT("Width_MusicValue"), TEXT("Text_MusicValue")), 18.f);
	AddV(List, MakeSliderGroup(TEXT("Group_Sfx"), TEXT("Label_Sfx"), TEXT("효과음"), TEXT("Row_Sfx"), TEXT("Height_Sfx"), TEXT("Slider_Sfx"), TEXT("Width_SfxValue"), TEXT("Text_SfxValue")), 18.f);
	AddV(List, MakeCheckRow(TEXT("Size_CheckBox_Mute"), TEXT("CheckBox_Mute"), TEXT("Label_CheckBox_Mute"), TEXT("모든 소리 끄기")), 24.f);

	// 화면
	AddV(List, MakeText(TEXT("Label_Display"), TEXT("화면"), TEXT("Medium"), 24, TextTitle), 14.f);
	AddV(List, MakeCheckRow(TEXT("Size_CheckBox_Fullscreen"), TEXT("CheckBox_Fullscreen"), TEXT("Label_CheckBox_Fullscreen"), TEXT("전체 화면")), 6.f);
	AddV(List, MakeCheckRow(TEXT("Size_CheckBox_ScreenShake"), TEXT("CheckBox_ScreenShake"), TEXT("Label_CheckBox_ScreenShake"), TEXT("화면 흔들림")), 24.f);

	// 접근성
	AddV(List, MakeText(TEXT("Label_Accessibility"), TEXT("접근성"), TEXT("Medium"), 24, TextTitle), 14.f);
	AddV(List, MakeCheckRow(TEXT("Size_CheckBox_TeamColors"), TEXT("CheckBox_TeamColors"), TEXT("Label_CheckBox_TeamColors"), TEXT("색 구분 보조")), 6.f);
	AddV(List, MakeCheckRow(TEXT("Size_CheckBox_TeamMarker"), TEXT("CheckBox_TeamMarker"), TEXT("Label_CheckBox_TeamMarker"), TEXT("캐릭터 위 팀 표시")), 24.f);

	// 플레이어 이름
	AddV(List, MakeText(TEXT("Label_Profile"), TEXT("플레이어 이름"), TEXT("Medium"), 24, TextTitle), 14.f);
	AddV(List, MakeText(TEXT("Text_PlayerName"), TEXT("PLAYER"), TEXT("Medium"), 22, TextMain), 14.f);

	UVerticalBox* ChangeName = Make<UVerticalBox>(TEXT("Panel_ChangeName"));
	ChangeName->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UHorizontalBox* NameRow = Make<UHorizontalBox>(TEXT("Row_NameChange"));
	NameRow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	UEditableTextBox* NewName = Make<UEditableTextBox>(TEXT("EditableTextBox_NewName"));
	NewName->SetVisibility(ESlateVisibility::Visible);
	NewName->SetHintText(Loc(TEXT("EditableTextBox_NewName"), TEXT("새 이름 입력"), /*bHint=*/true));
	{
		const FMargin InputMargin(0.051471f, 0.162791f, 0.051471f, 0.162791f);
		FEditableTextBoxStyle Style = NewName->WidgetStyle;
		Style.TextStyle.SetFont(MakeFont(TEXT("Regular"), 20));
		Style.TextStyle.SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Style.SetBackgroundImageNormal(ImageBrush(TEXT("Rectangle_3"), InputMargin, FLinearColor(1.f, 1.f, 1.f, 1.f)));
		Style.SetBackgroundImageHovered(ImageBrush(TEXT("Rectangle_3"), InputMargin, FLinearColor(1.1f, 1.1f, 1.1f, 1.f)));
		Style.SetBackgroundImageFocused(ImageBrush(TEXT("Rectangle_3"), InputMargin, FLinearColor(1.1f, 1.1f, 1.1f, 1.f)));
		Style.SetBackgroundImageReadOnly(ImageBrush(TEXT("Rectangle_3"), InputMargin, FLinearColor(1.f, 1.f, 1.f, 1.f)));
		Style.SetPadding(FMargin(16.f, 8.f, 16.f, 8.f));
		NewName->WidgetStyle = Style;
	}
	AddH(NameRow, Sized(TEXT("Height_NameInput"), NewName, 0.f, 56.f), 12.f, true);

	UButton* ChangeButton = MakeButton(TEXT("Button_ChangeName"), TEXT("Label_Button_ChangeName"), TEXT("이름 변경"), TEXT("Rectangle_6"), ButtonMarginA, FLinearColor(0.055f, 0.035f, 0.01f, 1.f));
	AddH(NameRow, Sized(TEXT("Size_Button_ChangeName"), ChangeButton, 160.f, 56.f), 0.f, false);
	AddV(ChangeName, NameRow, 8.f);
	AddV(ChangeName, MakeText(TEXT("Text_NameMessage"), TEXT(""), TEXT("Regular"), 17, TextWarn), 0.f);
	AddV(List, ChangeName, 0.f);

	// -- 오른쪽: 조작 카드
	UVerticalBox* ControlsColumn = Make<UVerticalBox>(TEXT("Column_Controls"));
	ControlsColumn->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	AddH(Columns, MakeCard(TEXT("Card_Controls"), ControlsColumn, CardColor, FMargin(24.f)), 0.f, true, VAlign_Fill);

	AddV(ControlsColumn, MakeText(TEXT("Label_Controls"), TEXT("조작 방법"), TEXT("Medium"), 24, TextTitle), 14.f);
	AddV(ControlsColumn, MakeText(TEXT("Label_ControlsHint"), TEXT("변경할 키를 누른 뒤 새 키를 입력하세요"), TEXT("Regular"), 17, TextDim), 20.f);

	UVerticalBox* ControlsList = Make<UVerticalBox>(TEXT("VerticalBox_Controls"));
	ControlsList->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UScrollBox* ControlsScroll = Make<UScrollBox>(TEXT("Scroll_Controls"));
	ControlsScroll->SetVisibility(ESlateVisibility::Visible);
	ControlsScroll->SetOrientation(Orient_Vertical);
	ControlsScroll->AddChild(ControlsList);
	AddV(ControlsColumn, ControlsScroll, 12.f, true);
	AddV(ControlsColumn, MakeText(TEXT("Text_ControlsMessage"), TEXT(""), TEXT("Regular"), 17, TextWarn), 0.f);

	// ===== 안내 줄 + 바닥 버튼 =====
	AddV(Layout, Sized(TEXT("Height_SettingsMessage"), MakeText(TEXT("Text_Message"), TEXT(""), TEXT("Regular"), 17, TextWarn), 0.f, 44.f), 8.f);

	UHorizontalBox* Footer = Make<UHorizontalBox>(TEXT("Row_SettingsFooter"));
	Footer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UButton* ResetButton = MakeButton(TEXT("Button_Reset"), TEXT("Label_Button_Reset"), TEXT("기본값으로 초기화"), TEXT("Rectangle_5"), ButtonMarginB, FLinearColor::White);
	AddH(Footer, Sized(TEXT("Size_Button_Reset"), ResetButton, 260.f, 56.f), 0.f, false);
	USpacer* FooterSpacer = Make<USpacer>(TEXT("Spacer_Footer"));
	FooterSpacer->SetSize(FVector2D(1.f, 1.f));
	AddH(Footer, FooterSpacer, 0.f, true);
	UButton* CloseButton = MakeButton(TEXT("Button_Close"), TEXT("Label_Button_Close"), TEXT("닫기"), TEXT("Rectangle_4"), ButtonMarginB, FLinearColor::White);
	AddH(Footer, Sized(TEXT("Size_Button_Close"), CloseButton, 200.f, 56.f), 0.f, false);
	AddV(Layout, Footer, 0.f);

	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();

	const int32 Created = WidgetCount;
	Tree = nullptr;
	return Created;
#else
	return 0;
#endif
}
