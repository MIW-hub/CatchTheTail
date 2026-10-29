// Copyright Moon Party Games. All Rights Reserved.
#include "CTUIBuildTools.h"
#include "CTMatchSettings.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Materials/MaterialInterface.h"
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

int32 UCTUIBuildTools::BuildHudSkillSlots(UBlueprint* WidgetBlueprint, UMaterialInterface* CooldownMaterial)
{
#if WITH_EDITOR
	if (!WidgetBlueprint) { return -1; }

	const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
	Tree = TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
	UCanvasPanel* Canvas = Tree ? Cast<UCanvasPanel>(Tree->FindWidget(TEXT("Canvas_HUD"))) : nullptr;
	if (!Canvas)
	{
		Tree = nullptr;
		return -1;
	}
	WidgetCount = 0;

	auto FindImage = [](const TCHAR* Name) { return Cast<UImage>(Tree->FindWidget(FName(Name))); };

	// 이미 있으면 그대로 쓰고, 없을 때만 만든다. bCreated 는 새로 만들었는지
	auto GetOrMakeImage = [&FindImage](const TCHAR* Name, const UImage* CopyFrom, bool& bCreated) -> UImage*
	{
		bCreated = false;
		if (UImage* Existing = FindImage(Name)) { return Existing; }

		UImage* Image = Make<UImage>(Name);
		if (CopyFrom)
		{
			Image->SetBrush(CopyFrom->GetBrush());
			Image->SetColorAndOpacity(CopyFrom->GetColorAndOpacity());
		}
		bCreated = true;
		return Image;
	};

	auto PlaceOnCanvas = [Canvas](UWidget* Widget, float X, float Y, float Width, float Height, FVector2D Alignment)
	{
		UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot);
		if (!CanvasSlot)
		{
			CanvasSlot = Cast<UCanvasPanelSlot>(Canvas->AddChild(Widget));
		}
		if (!CanvasSlot) { return; }

		CanvasSlot->SetAnchors(FAnchors(0.5f, 1.f));
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetPosition(FVector2D(X, Y));
		CanvasSlot->SetSize(FVector2D(Width, Height));
	};

	UImage* RunPanel = FindImage(TEXT("Image_ActionPanel"));
	UImage* RunIcon = FindImage(TEXT("Image_RunIcon"));
	UImage* RunCool = FindImage(TEXT("Image_CoolTime"));
	UImage* GrabPanel = FindImage(TEXT("Image_ActionPanel_1"));
	UImage* GrabIcon = FindImage(TEXT("Image_RunIcon_1"));
	UImage* GrabCool = FindImage(TEXT("Image_CoolTime_1"));
	if (!RunPanel || !RunIcon || !RunCool || !GrabPanel || !GrabIcon || !GrabCool)
	{
		Tree = nullptr;
		return -1;
	}

	// 밀기 칸 (잡기 칸 복사, 아이콘은 임시로 잡기 아이콘)
	bool bCreated = false;
	UImage* PushPanel = GetOrMakeImage(TEXT("Image_ActionPanel_2"), GrabPanel, bCreated);
	UImage* PushIcon = GetOrMakeImage(TEXT("Image_PushIcon"), GrabIcon, bCreated);
	UImage* PushCool = GetOrMakeImage(TEXT("Image_CoolTime_2"), GrabCool, bCreated);

	// 칸 배치: 잡기, 밀기, 달리기 (달리기 위치 기준, 간격은 기존 간격)
	const float Spacing = 196.f;
	const float RunX = 711.039f;
	const float PanelY = -173.081f;
	const float IconY = -144.f;

	struct FSlotSet { UImage* Panel; UImage* Icon; UImage* Cool; float X; };
	const FSlotSet Slots[] = {
		{ GrabPanel, GrabIcon, GrabCool, RunX - Spacing * 2.f },
		{ PushPanel, PushIcon, PushCool, RunX - Spacing },
		{ RunPanel,  RunIcon,  RunCool,  RunX },
	};

	for (const FSlotSet& Set : Slots)
	{
		PlaceOnCanvas(Set.Panel, Set.X, PanelY, 128.f, 128.f, FVector2D(0.5, 0.0));
		PlaceOnCanvas(Set.Icon, Set.X - 5.f, IconY, 70.f, 70.f, FVector2D(0.5, 0.0));
		PlaceOnCanvas(Set.Cool, Set.X, PanelY, 128.f, 128.f, FVector2D(0.5, 0.0));

		// 쿨다운 이미지: 머티리얼이 있으면 시계 방향으로 걷히는 모양, 처음에는 숨김
		if (CooldownMaterial)
		{
			FSlateBrush Brush = Set.Cool->GetBrush();
			Brush.SetResourceObject(CooldownMaterial);
			Brush.ImageSize = FVector2D(128.0, 128.0);
			Brush.TintColor = FLinearColor::White;
			Brush.DrawAs = ESlateBrushDrawType::Image;
			Set.Cool->SetBrush(Brush);
			Set.Cool->SetColorAndOpacity(FLinearColor::White);
		}
		Set.Cool->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 남은 시간 글자
	const struct { const TCHAR* Name; float X; } Texts[] = {
		{ TEXT("Text_GrabCooldown"), RunX - Spacing * 2.f },
		{ TEXT("Text_PushCooldown"), RunX - Spacing },
		{ TEXT("Text_RunCooldown"),  RunX },
	};
	for (const auto& Entry : Texts)
	{
		UTextBlock* Text = Cast<UTextBlock>(Tree->FindWidget(FName(Entry.Name)));
		if (!Text)
		{
			Text = Make<UTextBlock>(Entry.Name);
			Text->SetText(FText::GetEmpty());
			FSlateFontInfo Font = MakeFont(TEXT("Bold"), 32);
			Font.OutlineSettings.OutlineSize = 2;
			Font.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.85f);
			Text->SetFont(Font);
			Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			Text->SetJustification(ETextJustify::Center);
			Text->SetVisibility(ESlateVisibility::Collapsed);
		}
		PlaceOnCanvas(Text, Entry.X, PanelY + 64.f, 128.f, 48.f, FVector2D(0.5, 0.5));
	}

	// 변수로 쓰는 위젯은 이름-GUID 표에 올라 있어야 컴파일이 문제 삼지 않음
	if (const FMapProperty* GuidMapProperty = CastField<FMapProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetVariableNameToGuidMap"))))
	{
		FScriptMapHelper GuidMap(GuidMapProperty, GuidMapProperty->ContainerPtrToValuePtr<void>(WidgetBlueprint));
		const TCHAR* NewNames[] = {
			TEXT("Image_ActionPanel_2"), TEXT("Image_PushIcon"), TEXT("Image_CoolTime_2"),
			TEXT("Text_GrabCooldown"), TEXT("Text_PushCooldown"), TEXT("Text_RunCooldown"),
		};
		for (const TCHAR* NewName : NewNames)
		{
			FName Key(NewName);
			if (Tree->FindWidget(Key) && GuidMap.FindMapPairIndexFromHash(&Key) == INDEX_NONE)
			{
				FGuid Guid = FGuid::NewGuid();
				GuidMap.AddPair(&Key, &Guid);
			}
		}
	}

	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();

	const int32 Created = WidgetCount;
	Tree = nullptr;
	return Created;
#else
	return -1;
#endif
}

FString UCTUIBuildTools::DescribeWidgetTree(UBlueprint* WidgetBlueprint)
{
	FString Result;
#if WITH_EDITOR
	if (!WidgetBlueprint) { return Result; }

	const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
	UWidgetTree* DescribedTree = TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
	if (!DescribedTree) { return Result; }

	TArray<UWidget*> All;
	DescribedTree->GetAllWidgets(All);
	for (UWidget* Widget : All)
	{
		Result += FString::Printf(TEXT("%s [%s] parent=%s vis=%d var=%d"), *Widget->GetName(), *Widget->GetClass()->GetName(),
			Widget->GetParent() ? *Widget->GetParent()->GetName() : TEXT("-"), static_cast<int32>(Widget->GetVisibility()), Widget->bIsVariable ? 1 : 0);
		if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
		{
			const FMargin Offsets = CanvasSlot->GetOffsets();
			Result += FString::Printf(TEXT(" pos=(%.1f,%.1f) size=(%.1f,%.1f) anchors=(%.2f,%.2f) align=(%.2f,%.2f)"),
				Offsets.Left, Offsets.Top, Offsets.Right, Offsets.Bottom,
				CanvasSlot->GetAnchors().Minimum.X, CanvasSlot->GetAnchors().Minimum.Y,
				CanvasSlot->GetAlignment().X, CanvasSlot->GetAlignment().Y);
		}
		if (const USizeBox* Box = Cast<USizeBox>(Widget))
		{
			Result += FString::Printf(TEXT(" sizebox w=%s%.0f h=%s%.0f"), Box->bOverride_WidthOverride ? TEXT("") : TEXT("(off)"), Box->WidthOverride, Box->bOverride_HeightOverride ? TEXT("") : TEXT("(off)"), Box->HeightOverride);
		}
		if (const UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
		{
			Result += FString::Printf(TEXT(" text=\"%s\""), *TextBlock->GetText().ToString());
		}
		Result += TEXT("\n");
	}

	// 이름-GUID 표에 남은 지운 위젯 확인
	if (const FMapProperty* GuidMapProperty = CastField<FMapProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetVariableNameToGuidMap"))))
	{
		FScriptMapHelper GuidMap(GuidMapProperty, GuidMapProperty->ContainerPtrToValuePtr<void>(WidgetBlueprint));
		Result += FString::Printf(TEXT("GUID entries=%d\n"), GuidMap.Num());
		for (int32 MapIndex = 0; MapIndex < GuidMap.GetMaxIndex(); ++MapIndex)
		{
			if (!GuidMap.IsValidIndex(MapIndex)) { continue; }
			const FName* StoredKey = reinterpret_cast<const FName*>(GuidMap.GetKeyPtr(MapIndex));
			if (StoredKey && !DescribedTree->FindWidget(*StoredKey)) { Result += FString::Printf(TEXT("STALE GUID: %s\n"), *StoredKey->ToString()); }
		}
	}
#endif
	return Result;
}

bool UCTUIBuildTools::AddScoreEntryCrown(UBlueprint* WidgetBlueprint, UTexture2D* CrownTexture)
{
#if WITH_EDITOR
	if (!WidgetBlueprint || !CrownTexture) { return false; }

	const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
	Tree = TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
	UCanvasPanel* Canvas = Tree ? Cast<UCanvasPanel>(Tree->FindWidget(TEXT("Canvas_ScoreEntry"))) : nullptr;
	if (!Canvas)
	{
		Tree = nullptr;
		return false;
	}

	UImage* Crown = Cast<UImage>(Tree->FindWidget(TEXT("Image_Crown")));
	if (!Crown)
	{
		Crown = Make<UImage>(TEXT("Image_Crown"));
		UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Canvas->AddChild(Crown));
		if (CanvasSlot)
		{
			CanvasSlot->SetAnchors(FAnchors(0.f, 0.f));
			CanvasSlot->SetAlignment(FVector2D(0.0, 0.0));
			CanvasSlot->SetPosition(FVector2D(400.0, 16.0));
			CanvasSlot->SetSize(FVector2D(32.0, 32.0));
		}
	}
	FSlateBrush Brush;
	Brush.SetResourceObject(CrownTexture);
	Brush.ImageSize = FVector2D(32.0, 32.0);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Crown->SetBrush(Brush);
	Crown->SetVisibility(ESlateVisibility::Collapsed);

	if (const FMapProperty* GuidMapProperty = CastField<FMapProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetVariableNameToGuidMap"))))
	{
		FScriptMapHelper GuidMap(GuidMapProperty, GuidMapProperty->ContainerPtrToValuePtr<void>(WidgetBlueprint));
		FName Key(TEXT("Image_Crown"));
		if (GuidMap.FindMapPairIndexFromHash(&Key) == INDEX_NONE)
		{
			FGuid Guid = FGuid::NewGuid();
			GuidMap.AddPair(&Key, &Guid);
		}
	}

	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	Tree = nullptr;
	return true;
#else
	return false;
#endif
}

int32 UCTUIBuildTools::BuildLanguageSelectDesign(UBlueprint* WidgetBlueprint)
{
#if WITH_EDITOR
	if (!WidgetBlueprint) { return 0; }

	const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
	Tree = TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
	if (!Tree) { return 0; }
	WidgetCount = 0;
	BlueprintName = TEXT("WBP_CTLanguageSelectWidget");

	TArray<UWidget*> OldWidgets;
	Tree->GetAllWidgets(OldWidgets);
	for (UWidget* Old : OldWidgets) { Tree->RemoveWidget(Old); }
	Tree->RootWidget = nullptr;

	const FMargin ButtonMarginA(0.041237f, 0.210526f, 0.041237f, 0.210526f);
	const FMargin ButtonMarginB(0.101266f, 0.192771f, 0.101266f, 0.192771f);

	// 어두운 배경 + 가운데 카드 (설정 화면과 같은 모양)
	UOverlay* Root = Make<UOverlay>(TEXT("Root_LanguageSelect"));
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Tree->RootWidget = Root;

	UImage* Dim = Make<UImage>(TEXT("Image_LanguageDim"));
	Dim->SetColorAndOpacity(FLinearColor(0.003f, 0.006f, 0.015f, 0.95f));
	Dim->SetVisibility(ESlateVisibility::Visible);
	UOverlaySlot* DimSlot = Root->AddChildToOverlay(Dim);
	DimSlot->SetHorizontalAlignment(HAlign_Fill);
	DimSlot->SetVerticalAlignment(VAlign_Fill);

	UVerticalBox* Layout = Make<UVerticalBox>(TEXT("Layout_Language"));
	Layout->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UBorder* Frame = MakeCard(TEXT("Frame_Language"), Layout, FLinearColor(0.012f, 0.02f, 0.04f, 1.f), FMargin(48.f, 40.f, 48.f, 40.f));
	USizeBox* Design = Sized(TEXT("Size_LanguageDesign"), Frame, 760.f, 640.f);

	UScaleBox* Scale = Make<UScaleBox>(TEXT("Scale_Language"));
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

	// 제목: 세 언어를 같이 적어서 어느 언어로 보든 읽을 수 있게 함 (번역 대상 아님)
	UTextBlock* Title = MakeText(TEXT("Label_LanguageTitle"), TEXT(""), TEXT("Bold"), 40, TextMain, ETextJustify::Center);
	Title->SetText(FText::FromString(TEXT("Language  언어  言語")));
	AddV(Layout, Title, 12.f);

	UTextBlock* Subtitle = MakeText(TEXT("Label_LanguageSubtitle"), TEXT(""), TEXT("Regular"), 20, TextDim, ETextJustify::Center);
	Subtitle->SetText(FText::FromString(TEXT("Select your language  /  언어를 선택하세요  /  言語を選択")));
	AddV(Layout, Subtitle, 36.f);

	// 언어 버튼: 각 언어로 쓴 이름 (번역 대상 아님)
	struct FLanguageButton { const TCHAR* Button; const TCHAR* Label; const TCHAR* Name; };
	const FLanguageButton LanguageButtons[] = {
		{ TEXT("Button_LanguageKo"), TEXT("Label_Button_LanguageKo"), TEXT("한국어") },
		{ TEXT("Button_LanguageEn"), TEXT("Label_Button_LanguageEn"), TEXT("English") },
		{ TEXT("Button_LanguageJa"), TEXT("Label_Button_LanguageJa"), TEXT("日本語") },
	};
	for (const FLanguageButton& Entry : LanguageButtons)
	{
		UButton* Button = MakeButton(Entry.Button, Entry.Label, TEXT(""), TEXT("Rectangle_5"), ButtonMarginB, FLinearColor::White);
		if (UTextBlock* Label = Cast<UTextBlock>(Button->GetChildAt(0)))
		{
			Label->SetText(FText::FromString(Entry.Name));
			Label->SetFont(MakeFont(TEXT("SemiBold"), 26));
		}
		AddV(Layout, Sized(*FString::Printf(TEXT("Size_%s"), Entry.Button), Button, 0.f, 72.f), 16.f);
	}

	USpacer* Gap = Make<USpacer>(TEXT("Spacer_Language"));
	Gap->SetSize(FVector2D(1.f, 24.f));
	AddV(Layout, Gap, 0.f);

	// 확인 (이 글자만 번역 대상)
	UButton* Confirm = MakeButton(TEXT("Button_Confirm"), TEXT("Label_Button_Confirm"), TEXT("확인"), TEXT("Rectangle_4"), ButtonMarginB, FLinearColor::White);
	AddV(Layout, Sized(TEXT("Size_Button_Confirm"), Confirm, 0.f, 64.f), 0.f);

	BlueprintName = TEXT("WBP_CTSettingsWidget");
	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();

	const int32 Created = WidgetCount;
	Tree = nullptr;
	return Created;
#else
	return 0;
#endif
}

bool UCTUIBuildTools::RefineSettingsWidget(UBlueprint* WidgetBlueprint)
{
#if WITH_EDITOR
	if (!WidgetBlueprint) { return false; }

	const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
	Tree = TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
	if (!Tree) { return false; }
	WidgetCount = 0;
	BlueprintName = TEXT("WBP_CTSettingsWidget");

	UVerticalBox* List = Cast<UVerticalBox>(Tree->FindWidget(TEXT("List_Settings")));
	UVerticalBox* Controls = Cast<UVerticalBox>(Tree->FindWidget(TEXT("VerticalBox_Controls")));
	if (!List || !Controls)
	{
		Tree = nullptr;
		return false;
	}

	const FMargin ButtonMarginB(0.101266f, 0.192771f, 0.101266f, 0.192771f);

	// ===== 1. 언어 콤보박스 -> 버튼 3개 =====
	UWidget* OldCombo = Tree->FindWidget(TEXT("ComboBox_Language"));
	UWidget* OldComboBox = Tree->FindWidget(TEXT("Height_Language"));
	if (OldCombo && !Tree->FindWidget(TEXT("Row_Language")))
	{
		const int32 InsertIndex = OldComboBox ? List->GetChildIndex(OldComboBox) : INDEX_NONE;
		if (OldComboBox) { Tree->RemoveWidget(OldComboBox); }
		Tree->RemoveWidget(OldCombo);

		UHorizontalBox* Row = Make<UHorizontalBox>(TEXT("Row_Language"));
		Row->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		struct FLanguageButton { const TCHAR* Button; const TCHAR* Label; const TCHAR* SizeName; const TCHAR* Name; };
		const FLanguageButton Buttons[] = {
			{ TEXT("Button_LanguageKo"), TEXT("Label_Button_LanguageKo"), TEXT("Size_Button_LanguageKo"), TEXT("한국어") },
			{ TEXT("Button_LanguageEn"), TEXT("Label_Button_LanguageEn"), TEXT("Size_Button_LanguageEn"), TEXT("English") },
			{ TEXT("Button_LanguageJa"), TEXT("Label_Button_LanguageJa"), TEXT("Size_Button_LanguageJa"), TEXT("日本語") },
		};
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const FLanguageButton& Entry = Buttons[Index];
			UButton* Button = MakeButton(Entry.Button, Entry.Label, TEXT(""), TEXT("Rectangle_5"), ButtonMarginB, FLinearColor::White);
			if (UTextBlock* Label = Cast<UTextBlock>(Button->GetChildAt(0)))
			{
				Label->SetText(FText::FromString(Entry.Name));
				Label->SetFont(MakeFont(TEXT("SemiBold"), 20));
			}
			AddH(Row, Sized(Entry.SizeName, Button, 0.f, 56.f), Index < 2 ? 12.f : 0.f, true);
		}

		UPanelSlot* RowSlot = List->InsertChildAt(InsertIndex >= 0 ? InsertIndex : 1, Row);
		if (UVerticalBoxSlot* RowBoxSlot = Cast<UVerticalBoxSlot>(RowSlot))
		{
			RowBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));
			RowBoxSlot->SetHorizontalAlignment(HAlign_Fill);
			RowBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}

	// ===== 2. 조작 방법: 고정 안내 =====
	Controls->ClearChildren();
	if (UTextBlock* Hint = Cast<UTextBlock>(Tree->FindWidget(TEXT("Label_ControlsHint"))))
	{
		Hint->SetText(Loc(TEXT("Label_ControlsHint"), TEXT("게임 중에 쓰는 키입니다")));
	}

	struct FControlLine { const TCHAR* Id; const TCHAR* Name; const TCHAR* Keys; bool bKeysLocalized; };
	const FControlLine Lines[] = {
		{ TEXT("Move"), TEXT("이동"),   TEXT("W A S D"),          false },
		{ TEXT("Look"), TEXT("시점"),   TEXT("마우스 이동"),      true  },
		{ TEXT("Jump"), TEXT("점프"),   TEXT("Space"),            false },
		{ TEXT("Run"),  TEXT("달리기"), TEXT("Shift"),            false },
		{ TEXT("Grab"), TEXT("잡기"),   TEXT("마우스 왼쪽 클릭"), true  },
		{ TEXT("Push"), TEXT("밀기"),   TEXT("E"),                false },
	};
	for (const FControlLine& Line : Lines)
	{
		UHorizontalBox* Row = Make<UHorizontalBox>(*FString::Printf(TEXT("Row_Control_%s"), Line.Id));
		Row->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UTextBlock* Name = MakeText(*FString::Printf(TEXT("Label_Control_%s"), Line.Id), Line.Name, TEXT("Regular"), 22, TextMain);
		Name->SetAutoWrapText(false);
		AddH(Row, Name, 16.f, true);

		UTextBlock* Keys = MakeText(*FString::Printf(TEXT("Text_ControlKeys_%s"), Line.Id), Line.Keys, TEXT("Bold"), 22, TextTitle, ETextJustify::Right);
		Keys->SetAutoWrapText(false);
		if (!Line.bKeysLocalized) { Keys->SetText(FText::FromString(Line.Keys)); }
		AddH(Row, Keys, 0.f, false);

		AddV(Controls, Sized(*FString::Printf(TEXT("Height_Control_%s"), Line.Id), Row, 0.f, 52.f), 6.f);
	}

	// ===== 3. 버튼/체크박스 안 글자 줄바꿈 끔 =====
	TArray<UWidget*> All;
	Tree->GetAllWidgets(All);
	for (UWidget* Widget : All)
	{
		UTextBlock* Text = Cast<UTextBlock>(Widget);
		if (Text && Text->GetParent() && (Text->GetParent()->IsA<UButton>() || Text->GetParent()->IsA<UCheckBox>()))
		{
			Text->SetAutoWrapText(false);
		}
	}

	// 변수로 쓰는 위젯의 이름-GUID 등록
	if (const FMapProperty* GuidMapProperty = CastField<FMapProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetVariableNameToGuidMap"))))
	{
		FScriptMapHelper GuidMap(GuidMapProperty, GuidMapProperty->ContainerPtrToValuePtr<void>(WidgetBlueprint));
		TArray<UWidget*> Current;

		// 지운 위젯의 GUID 정리
		for (int32 MapIndex = GuidMap.GetMaxIndex() - 1; MapIndex >= 0; --MapIndex)
		{
			if (!GuidMap.IsValidIndex(MapIndex)) { continue; }
			const FName* StoredKey = reinterpret_cast<const FName*>(GuidMap.GetKeyPtr(MapIndex));
			if (StoredKey && !Tree->FindWidget(*StoredKey)) { UE_LOG(LogTemp, Log, TEXT("[CT] 지운 위젯 GUID 정리: %s"), *StoredKey->ToString()); GuidMap.RemoveAt(MapIndex); }
		}
		Tree->GetAllWidgets(Current);
		for (UWidget* Widget : Current)
		{
			if (!Widget->bIsVariable) { continue; }
			FName Key = Widget->GetFName();
			if (GuidMap.FindMapPairIndexFromHash(&Key) == INDEX_NONE)
			{
				FGuid Guid = FGuid::NewGuid();
				GuidMap.AddPair(&Key, &Guid);
			}
		}
	}

	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	Tree = nullptr;
	return true;
#else
	return false;
#endif
}

bool UCTUIBuildTools::RefineNameChange(UBlueprint* WidgetBlueprint)
{
#if WITH_EDITOR
	if (!WidgetBlueprint) { return false; }

	const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
	Tree = TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
	if (!Tree) { return false; }
	WidgetCount = 0;
	BlueprintName = TEXT("WBP_CTSettingsWidget");

	UVerticalBox* Panel = Cast<UVerticalBox>(Tree->FindWidget(TEXT("Panel_ChangeName")));
	UWidget* Row = Tree->FindWidget(TEXT("Row_NameChange"));
	UTextBlock* ConfirmLabel = Cast<UTextBlock>(Tree->FindWidget(TEXT("Label_Button_ChangeName")));
	if (!Panel || !Row)
	{
		Tree = nullptr;
		return false;
	}

	// 입력 칸은 접어 두고, 안의 버튼은 "확인"
	Row->SetVisibility(ESlateVisibility::Collapsed);
	if (ConfirmLabel) { ConfirmLabel->SetText(Loc(TEXT("Label_Button_ChangeName"), TEXT("확인"))); }

	// 펼치는 버튼
	if (!Tree->FindWidget(TEXT("Button_ChangeNameOpen")))
	{
		const FMargin ButtonMarginA(0.041237f, 0.210526f, 0.041237f, 0.210526f);
		UButton* OpenButton = MakeButton(TEXT("Button_ChangeNameOpen"), TEXT("Label_Button_ChangeNameOpen"), TEXT("이름 변경"), TEXT("Rectangle_6"), ButtonMarginA, FLinearColor(0.055f, 0.035f, 0.01f, 1.f));
		USizeBox* Sized160 = Sized(TEXT("Size_Button_ChangeNameOpen"), OpenButton, 200.f, 56.f);

		UPanelSlot* NewSlot = Panel->InsertChildAt(0, Sized160);
		if (UVerticalBoxSlot* BoxSlot = Cast<UVerticalBoxSlot>(NewSlot))
		{
			BoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
			BoxSlot->SetHorizontalAlignment(HAlign_Left);
			BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}

	if (const FMapProperty* GuidMapProperty = CastField<FMapProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetVariableNameToGuidMap"))))
	{
		FScriptMapHelper GuidMap(GuidMapProperty, GuidMapProperty->ContainerPtrToValuePtr<void>(WidgetBlueprint));
		TArray<UWidget*> Current;
		Tree->GetAllWidgets(Current);
		for (UWidget* Widget : Current)
		{
			if (!Widget->bIsVariable) { continue; }
			FName Key = Widget->GetFName();
			if (GuidMap.FindMapPairIndexFromHash(&Key) == INDEX_NONE)
			{
				FGuid Guid = FGuid::NewGuid();
				GuidMap.AddPair(&Key, &Guid);
			}
		}
	}

	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	Tree = nullptr;
	return true;
#else
	return false;
#endif
}

bool UCTUIBuildTools::BuildMatchSettingsPanel(UBlueprint* WidgetBlueprint)
{
#if WITH_EDITOR
	if (!WidgetBlueprint) { return false; }

	const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
	Tree = TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
	if (!Tree) { return false; }
	WidgetCount = 0;
	BlueprintName = TEXT("WBP_CTLobbyWidget");

	UCanvasPanel* DialogCanvas = Cast<UCanvasPanel>(Tree->FindWidget(TEXT("Canvas_CompactMapDialog")));
	UImage* Card = Cast<UImage>(Tree->FindWidget(TEXT("Image_MapDialogCard")));
	UScrollBox* MapScroll = Cast<UScrollBox>(Tree->FindWidget(TEXT("Scroll_MapList")));
	UButton* CloseButton = Cast<UButton>(Tree->FindWidget(TEXT("Button_MapSelectClose")));
	if (!DialogCanvas || !Card || !MapScroll || !CloseButton)
	{
		Tree = nullptr;
		return false;
	}

	// 이름 바꾸기: 맵 선택 -> 맵 설정
	if (UTextBlock* ButtonLabel = Cast<UTextBlock>(Tree->FindWidget(TEXT("Label_Button_SelectMap"))))
	{
		ButtonLabel->SetText(Loc(TEXT("Label_Button_SelectMap"), TEXT("맵 설정")));
	}
	if (UTextBlock* DialogTitle = Cast<UTextBlock>(Tree->FindWidget(TEXT("Label_MapDialogTitle"))))
	{
		DialogTitle->SetText(Loc(TEXT("Label_MapDialogTitle"), TEXT("맵 설정")));
	}

	auto SlotOf = [](UWidget* Widget) { return Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr; };
	auto AddToCanvas = [](UCanvasPanel* Canvas, UWidget* Widget, FVector2D Position, FVector2D Size)
	{
		UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Canvas->AddChild(Widget));
		if (CanvasSlot)
		{
			CanvasSlot->SetAnchors(FAnchors(0.f, 0.f));
			CanvasSlot->SetAlignment(FVector2D(0.0, 0.0));
			CanvasSlot->SetPosition(Position);
			CanvasSlot->SetSize(Size);
		}
	};

	if (!Tree->FindWidget(TEXT("Panel_MatchSettings")))
	{
		// 창을 오른쪽으로 넓힘 (왼쪽에 설정 칸을 둘 자리)
		constexpr float Extra = 640.f;
		// Native_MapDialog 는 원래 크기, Cap_MapDialog 는 같은 비율로 줄인 한도라서 줄인 비율만큼만 넓힘
		USizeBox* NativeBox = Cast<USizeBox>(Tree->FindWidget(TEXT("Native_MapDialog")));
		USizeBox* CapBox = Cast<USizeBox>(Tree->FindWidget(TEXT("Cap_MapDialog")));
		if (NativeBox && NativeBox->bOverride_WidthOverride)
		{
			const float CapRatio = (CapBox && CapBox->bOverride_WidthOverride && NativeBox->WidthOverride > 0.f) ? CapBox->WidthOverride / NativeBox->WidthOverride : 1.f;
			NativeBox->SetWidthOverride(NativeBox->WidthOverride + Extra);
			if (CapBox && CapBox->bOverride_WidthOverride) { CapBox->SetWidthOverride(CapBox->WidthOverride + Extra * CapRatio); }
		}
		if (UCanvasPanelSlot* CardSlot = SlotOf(Card))
		{
			CardSlot->SetSize(CardSlot->GetSize() + FVector2D(Extra, 0.0));
		}
		if (UCanvasPanelSlot* CloseSlot = SlotOf(CloseButton))
		{
			CloseSlot->SetPosition(CloseSlot->GetPosition() + FVector2D(Extra, 0.0));
		}

		// 맵 목록: 오른쪽으로 옮기고 위에 소제목 자리를 둠
		constexpr float ListTop = 205.f;
		const float ListBottom = 145.f + 590.f;
		if (UCanvasPanelSlot* ListSlot = SlotOf(MapScroll))
		{
			ListSlot->SetPosition(FVector2D(60.0 + Extra, ListTop));
			ListSlot->SetSize(FVector2D(ListSlot->GetSize().X, ListBottom - ListTop));
		}

		// 소제목
		UTextBlock* SettingsHeader = MakeText(TEXT("Label_MatchSettingsHeader"), TEXT("경기 설정"), TEXT("SemiBold"), 26, TextTitle);
		AddToCanvas(DialogCanvas, SettingsHeader, FVector2D(60.0, 145.0), FVector2D(560.0, 44.0));
		UTextBlock* MapHeader = MakeText(TEXT("Label_MapListHeader"), TEXT("맵 선택"), TEXT("SemiBold"), 26, TextTitle);
		AddToCanvas(DialogCanvas, MapHeader, FVector2D(60.0 + Extra, 145.0), FVector2D(560.0, 44.0));

		// 경기 설정 칸: 항목마다 [이름 ... < 값 >]
		UVerticalBox* Panel = Make<UVerticalBox>(TEXT("Panel_MatchSettings"));
		Panel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		const FMargin ArrowMargin(0.101266f, 0.192771f, 0.101266f, 0.192771f);
		for (int32 Index = 0; Index < static_cast<int32>(ECTMatchSetting::Count); ++Index)
		{
			const ECTMatchSetting Setting = static_cast<ECTMatchSetting>(Index);
			const FString Key = FCTMatchSettings::GetKey(Setting).ToString();

			UHorizontalBox* Row = Make<UHorizontalBox>(*FString::Printf(TEXT("Row_Setting_%s"), *Key));
			Row->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			UTextBlock* Label = MakeText(*FString::Printf(TEXT("Label_Setting_%s"), *Key), TEXT(""), TEXT("Regular"), 22, TextDim);
			Label->SetText(FCTMatchSettings::GetLabel(Setting));
			USizeBox* LabelBox = Sized(*FString::Printf(TEXT("Size_Label_Setting_%s"), *Key), Label, 0.f, 56.f);
			Cast<USizeBoxSlot>(LabelBox->GetContentSlot())->SetVerticalAlignment(VAlign_Center);
			AddH(Row, LabelBox, 12.f, true);

			auto MakeArrow = [&](const TCHAR* Suffix, const TCHAR* Glyph)
			{
				UButton* Arrow = MakeButton(*FString::Printf(TEXT("Button_Setting_%s_%s"), *Key, Suffix), *FString::Printf(TEXT("Label_Button_Setting_%s_%s"), *Key, Suffix), TEXT(""), TEXT("Rectangle_5"), ArrowMargin, FLinearColor::White);
				if (UTextBlock* ArrowLabel = Cast<UTextBlock>(Arrow->GetChildAt(0)))
				{
					ArrowLabel->SetText(FText::FromString(Glyph));
				}
				return Sized(*FString::Printf(TEXT("Size_Button_Setting_%s_%s"), *Key, Suffix), Arrow, 56.f, 56.f);
			};

			AddH(Row, MakeArrow(TEXT("Prev"), TEXT("<")), 8.f, false);

			UTextBlock* Value = MakeText(*FString::Printf(TEXT("Text_Setting_%s"), *Key), TEXT(""), TEXT("SemiBold"), 24, TextMain, ETextJustify::Center);
			Value->SetText(FCTMatchSettings::FormatValue(Setting, FCTMatchSettings().Get(Setting)));
			USizeBox* ValueBox = Sized(*FString::Printf(TEXT("Size_Text_Setting_%s"), *Key), Value, 170.f, 56.f);
			Cast<USizeBoxSlot>(ValueBox->GetContentSlot())->SetVerticalAlignment(VAlign_Center);
			AddH(Row, ValueBox, 8.f, false);

			AddH(Row, MakeArrow(TEXT("Next"), TEXT(">")), 0.f, false);

			AddV(Panel, Sized(*FString::Printf(TEXT("Size_Row_Setting_%s"), *Key), Row, 0.f, 56.f), 18.f);
		}
		AddToCanvas(DialogCanvas, Panel, FVector2D(60.0, ListTop), FVector2D(560.0, ListBottom - ListTop));
	}

	// 로비 화면: 맵 아래에 설정 요약 한 줄
	if (UCanvasPanel* MapCanvas = Cast<UCanvasPanel>(Tree->FindWidget(TEXT("Canvas_CompactMap"))))
	{
		if (!Tree->FindWidget(TEXT("Text_MatchSettings")))
		{
			UTextBlock* Summary = MakeText(TEXT("Text_MatchSettings"), TEXT(""), TEXT("Regular"), 19, TextDim, ETextJustify::Center);
			Summary->SetText(FCTMatchSettings().MakeSummary());
			AddToCanvas(MapCanvas, Summary, FVector2D(0.0, 484.0), FVector2D(440.0, 30.0));
		}
	}

	if (const FMapProperty* GuidMapProperty = CastField<FMapProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetVariableNameToGuidMap"))))
	{
		FScriptMapHelper GuidMap(GuidMapProperty, GuidMapProperty->ContainerPtrToValuePtr<void>(WidgetBlueprint));
		TArray<UWidget*> Current;
		Tree->GetAllWidgets(Current);
		for (UWidget* Widget : Current)
		{
			if (!Widget->bIsVariable) { continue; }
			FName Key = Widget->GetFName();
			if (GuidMap.FindMapPairIndexFromHash(&Key) == INDEX_NONE)
			{
				FGuid Guid = FGuid::NewGuid();
				GuidMap.AddPair(&Key, &Guid);
			}
		}
	}

	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	Tree = nullptr;
	return true;
#else
	return false;
#endif
}
