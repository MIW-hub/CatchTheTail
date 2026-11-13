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
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/Slider.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
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

	// 색
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

	// 번역 키 붙인 글자
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

	// 칸별 위젯 묶음
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

	// 크기 지정 상자. 0이면 해당 방향 미지정
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

	// 바깥 틀
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

	// 머리글
	UHorizontalBox* Header = Make<UHorizontalBox>(TEXT("Row_SettingsHeader"));
	Header->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	AddH(Header, MakeText(TEXT("Label_SettingsTitle"), TEXT("설정"), TEXT("Bold"), 36, TextMain), 24.f, true);
	AddH(Header, MakeText(TEXT("Label_SettingsSubtitle"), TEXT("변경한 설정은 자동으로 저장됩니다"), TEXT("Regular"), 17, TextDim), 0.f, false);
	AddV(Layout, Sized(TEXT("Height_SettingsHeader"), Header, 0.f, 64.f), 16.f);

	// 본문
	UHorizontalBox* Columns = Make<UHorizontalBox>(TEXT("Row_SettingsColumns"));
	Columns->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	AddV(Layout, Columns, 12.f, true);

	// 왼쪽: 설정 목록
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
	Language->WidgetStyle.ComboButtonStyle.ButtonStyle = MakeButtonStyle(TEXT("UI_Old_Combo_Gray"), ComboMargin);
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
		Style.SetBackgroundImageNormal(ImageBrush(TEXT("UI_Old_Input_Dark"), InputMargin, FLinearColor(1.f, 1.f, 1.f, 1.f)));
		Style.SetBackgroundImageHovered(ImageBrush(TEXT("UI_Old_Input_Dark"), InputMargin, FLinearColor(1.1f, 1.1f, 1.1f, 1.f)));
		Style.SetBackgroundImageFocused(ImageBrush(TEXT("UI_Old_Input_Dark"), InputMargin, FLinearColor(1.1f, 1.1f, 1.1f, 1.f)));
		Style.SetBackgroundImageReadOnly(ImageBrush(TEXT("UI_Old_Input_Dark"), InputMargin, FLinearColor(1.f, 1.f, 1.f, 1.f)));
		Style.SetPadding(FMargin(16.f, 8.f, 16.f, 8.f));
		NewName->WidgetStyle = Style;
	}
	AddH(NameRow, Sized(TEXT("Height_NameInput"), NewName, 0.f, 56.f), 12.f, true);

	UButton* ChangeButton = MakeButton(TEXT("Button_ChangeName"), TEXT("Label_Button_ChangeName"), TEXT("이름 변경"), TEXT("UI_Old_Btn_Yellow"), ButtonMarginA, FLinearColor(0.055f, 0.035f, 0.01f, 1.f));
	AddH(NameRow, Sized(TEXT("Size_Button_ChangeName"), ChangeButton, 160.f, 56.f), 0.f, false);
	AddV(ChangeName, NameRow, 8.f);
	AddV(ChangeName, MakeText(TEXT("Text_NameMessage"), TEXT(""), TEXT("Regular"), 17, TextWarn), 0.f);
	AddV(List, ChangeName, 0.f);

	// 오른쪽: 조작 카드
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

	// 안내 줄 + 바닥 버튼
	AddV(Layout, Sized(TEXT("Height_SettingsMessage"), MakeText(TEXT("Text_Message"), TEXT(""), TEXT("Regular"), 17, TextWarn), 0.f, 44.f), 8.f);

	UHorizontalBox* Footer = Make<UHorizontalBox>(TEXT("Row_SettingsFooter"));
	Footer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UButton* ResetButton = MakeButton(TEXT("Button_Reset"), TEXT("Label_Button_Reset"), TEXT("기본값으로 초기화"), TEXT("UI_Old_Btn_Red"), ButtonMarginB, FLinearColor::White);
	AddH(Footer, Sized(TEXT("Size_Button_Reset"), ResetButton, 260.f, 56.f), 0.f, false);
	USpacer* FooterSpacer = Make<USpacer>(TEXT("Spacer_Footer"));
	FooterSpacer->SetSize(FVector2D(1.f, 1.f));
	AddH(Footer, FooterSpacer, 0.f, true);
	UButton* CloseButton = MakeButton(TEXT("Button_Close"), TEXT("Label_Button_Close"), TEXT("닫기"), TEXT("UI_Old_Btn_Gray"), ButtonMarginB, FLinearColor::White);
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

	// 없을 때만 생성. bCreated 는 신규 여부
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

	// 밀기 칸 (잡기 칸 복사)
	bool bCreated = false;
	UImage* PushPanel = GetOrMakeImage(TEXT("Image_ActionPanel_2"), GrabPanel, bCreated);
	UImage* PushIcon = GetOrMakeImage(TEXT("Image_PushIcon"), GrabIcon, bCreated);
	UImage* PushCool = GetOrMakeImage(TEXT("Image_CoolTime_2"), GrabCool, bCreated);

	// 칸 배치: 잡기, 밀기, 달리기
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

		// 쿨다운 이미지: 머티리얼 있으면 시계 방향, 처음엔 숨김
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

	// 변수 위젯은 이름-GUID 표 등록 필요
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

#if WITH_EDITOR
namespace
{
	const FLinearColor HudDarkText(0.2f, 0.06f, 0.24f, 1.f);
	const FLinearColor HudOutline(0.33f, 0.02f, 0.22f, 1.f);

	// 외곽선 있는 굵은 글자 (카드 밖/분홍 바탕 위에 올리는 글자)
	UTextBlock* MakeHudText(const TCHAR* Name, const TCHAR* Source, float Size, const FLinearColor& Color, ETextJustify::Type Justify, int32 Outline, const FLinearColor& OutlineColor = HudOutline)
	{
		UTextBlock* Text = Make<UTextBlock>(Name);
		Text->SetText(Loc(Name, Source));
		FSlateFontInfo Font = MakeFont(TEXT("Bold"), 0);
		Font.Size = Size;
		Font.OutlineSettings.OutlineSize = Outline;
		Font.OutlineSettings.OutlineColor = OutlineColor;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetVisibility(ESlateVisibility::HitTestInvisible);
		Text->SetJustification(Justify);
		return Text;
	}

	UImage* MakeHudImage(const TCHAR* Name, const TCHAR* TextureName, const FVector2D& Size)
	{
		UImage* Image = Make<UImage>(Name);
		FSlateBrush Brush;
		const FString Full(TextureName);
		Brush.SetResourceObject(LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/2D/UI/%s.%s"), *Full, *FPaths::GetBaseFilename(Full))));
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.ImageSize = Size;
		Image->SetBrush(Brush);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Image;
	}

	UCanvasPanelSlot* PlaceHud(UCanvasPanel* Parent, UWidget* Widget, FVector2D Pos, FVector2D Size, FVector2D Anchor, FVector2D Align)
	{
		UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Parent->AddChild(Widget));
		CanvasSlot->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
		CanvasSlot->SetAlignment(Align);
		CanvasSlot->SetPosition(Pos);
		if (Size.X > 0.f || Size.Y > 0.f) { CanvasSlot->SetSize(Size); }
		else { CanvasSlot->SetAutoSize(true); }
		return CanvasSlot;
	}

	// 변수 위젯은 이름-GUID 표 등록 필요
	void RegisterWidgetGuids(UBlueprint* WidgetBlueprint)
	{
		const FMapProperty* GuidMapProperty = CastField<FMapProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetVariableNameToGuidMap")));
		if (!GuidMapProperty) { return; }

		FScriptMapHelper GuidMap(GuidMapProperty, GuidMapProperty->ContainerPtrToValuePtr<void>(WidgetBlueprint));
		// 지워진 위젯의 GUID 제거
		for (int32 Index = GuidMap.GetMaxIndex() - 1; Index >= 0; --Index)
		{
			if (!GuidMap.IsValidIndex(Index)) { continue; }
			const FName* Existing = reinterpret_cast<const FName*>(GuidMap.GetKeyPtr(Index));
			if (!Tree->FindWidget(*Existing)) { GuidMap.RemoveAt(Index); }
		}

		TArray<UWidget*> All;
		Tree->GetAllWidgets(All);
		for (UWidget* Widget : All)
		{
			FName Key = Widget->GetFName();
			if (Widget->bIsVariable && GuidMap.FindMapPairIndexFromHash(&Key) == INDEX_NONE)
			{
				FGuid Guid = FGuid::NewGuid();
				GuidMap.AddPair(&Key, &Guid);
			}
		}
	}

	bool OpenTree(UBlueprint* WidgetBlueprint, const TCHAR* Name)
	{
		const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
		Tree = TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
		if (!Tree) { return false; }
		WidgetCount = 0;
		BlueprintName = Name;

		TArray<UWidget*> OldWidgets;
		Tree->GetAllWidgets(OldWidgets);
		for (UWidget* Old : OldWidgets) { Tree->RemoveWidget(Old); }
		Tree->RootWidget = nullptr;
		return true;
	}

	int32 CloseTree(UBlueprint* WidgetBlueprint)
	{
		RegisterWidgetGuids(WidgetBlueprint);
		BlueprintName = TEXT("WBP_CTSettingsWidget");
		WidgetBlueprint->Modify();
		WidgetBlueprint->MarkPackageDirty();
		const int32 Created = WidgetCount;
		Tree = nullptr;
		return Created;
	}
}
#endif

int32 UCTUIBuildTools::BuildHudLayout(UBlueprint* WidgetBlueprint, UMaterialInterface* CooldownMaterial)
{
#if WITH_EDITOR
	if (!WidgetBlueprint || !OpenTree(WidgetBlueprint, TEXT("WBP_CTHUDWidget"))) { return -1; }

	UCanvasPanel* Canvas = Make<UCanvasPanel>(TEXT("Canvas_HUD"));
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Tree->RootWidget = Canvas;

	const FLinearColor White = FLinearColor::White;
	const FVector2D TopLeft(0.f, 0.f), TopCenter(0.5f, 0.f), TopRight(1.f, 0.f), BottomLeft(0.f, 1.f), BottomRight(1.f, 1.f), Center(0.5f, 0.5f);

	auto NewGroup = [](const TCHAR* Name)
	{
		UCanvasPanel* Group = Make<UCanvasPanel>(Name);
		Group->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		return Group;
	};

	// 긴 번역 문구가 칸을 넘으면 칸에 맞게 줄어드는 상자 (커지지는 않음)
	auto FitBox = [](const TCHAR* Name, UWidget* Child)
	{
		UScaleBox* Box = Make<UScaleBox>(Name);
		Box->SetStretch(EStretch::ScaleToFit);
		Box->SetStretchDirection(EStretchDirection::DownOnly);
		Box->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UScaleBoxSlot* BoxSlot = Cast<UScaleBoxSlot>(Box->AddChild(Child));
		BoxSlot->SetHorizontalAlignment(HAlign_Left);
		BoxSlot->SetVerticalAlignment(VAlign_Center);
		return Box;
	};

	// 왼쪽 위: 타겟 카드
	{
		UCanvasPanel* Group = NewGroup(TEXT("Group_Target"));
		PlaceHud(Canvas, Group, FVector2D(40.f, 28.f), FVector2D(200.f, 107.f), TopLeft, TopLeft);
		PlaceHud(Group, MakeHudImage(TEXT("Image_TargetPanel"), TEXT("UI_Target"), FVector2D(200.f, 107.f)), FVector2D::ZeroVector, FVector2D(200.f, 107.f), TopLeft, TopLeft);
		PlaceHud(Group, MakeHudText(TEXT("Label_Target"), TEXT("타겟"), 15, White, ETextJustify::Center, 1), FVector2D(101.118622f, 12.816816f), FVector2D(0.f, 0.f), TopLeft, Center);

		UTextBlock* Name = MakeHudText(TEXT("Text_TargetColor"), TEXT("목표 없음"), 26, HudDarkText, ETextJustify::Left, 0);
		PlaceHud(Group, FitBox(TEXT("Fit_TargetColor"), Name), FVector2D(64.f, 28.f), FVector2D(116.f, 36.f), TopLeft, TopLeft);

		UImage* Dot = Make<UImage>(TEXT("Image_TargetColor"));
		Dot->SetVisibility(ESlateVisibility::HitTestInvisible);
		PlaceHud(Group, Dot, FVector2D(16.f, 28.f), FVector2D(30.f, 30.f), TopLeft, TopLeft);
	}

	// 위 가운데: 시간 카드
	{
		UCanvasPanel* Group = NewGroup(TEXT("Group_Time"));
		PlaceHud(Canvas, Group, FVector2D(0.f, 28.f), FVector2D(216.f, 109.f), TopCenter, TopCenter);
		PlaceHud(Group, MakeHudImage(TEXT("Image_TimePanel"), TEXT("UI_Time"), FVector2D(216.f, 109.f)), FVector2D::ZeroVector, FVector2D(216.f, 109.f), TopLeft, TopLeft);
		PlaceHud(Group, MakeHudText(TEXT("Text_RoundNumber"), TEXT("라운드 1"), 15, White, ETextJustify::Center, 1), FVector2D(109.f, 12.f), FVector2D(130.f, 28.f), TopLeft, Center);
		PlaceHud(Group, MakeHudText(TEXT("Text_GameTime"), TEXT("5:00"), 31.5f, White, ETextJustify::Center, 2, FLinearColor(0.02f, 0.18f, 0.45f, 1.f)), FVector2D(108.f, 44.f), FVector2D(200.f, 62.f), TopLeft, Center);

		UHorizontalBox* EndRow = Make<UHorizontalBox>(TEXT("Row_RoundEnd"));
		EndRow->SetVisibility(ESlateVisibility::HitTestInvisible);
		UHorizontalBoxSlot* EndLabel = EndRow->AddChildToHorizontalBox(MakeHudText(TEXT("Label_RoundEnd"), TEXT("라운드 종료"), 15, White, ETextJustify::Left, 0));
		EndLabel->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		EndRow->AddChildToHorizontalBox(MakeHudText(TEXT("Text_RoundTime"), TEXT("0:30"), 15, White, ETextJustify::Left, 0));
		PlaceHud(Group, EndRow, FVector2D(107.63063f, 83.135132f), FVector2D::ZeroVector, TopLeft, Center);
	}

	// 오른쪽 위: 순위표 카드 (줄이 늘면 아래로 커짐)
	{
		UBorder* Card = Make<UBorder>(TEXT("Card_Scoreboard"));
		Card->SetBrush(ImageBrush(TEXT("UI_Ranking"), FMargin(0.14f, 0.4f, 0.14f, 0.14f), White));
		Card->SetPadding(FMargin(10.f, 3.f, 10.f, 14.f));
		Card->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UVerticalBox* Inner = Make<UVerticalBox>(TEXT("Layout_Scoreboard"));
		Inner->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Card->AddChild(Inner);

		UHorizontalBox* Header = Make<UHorizontalBox>(TEXT("Row_ScoreboardHeader"));
		Header->SetVisibility(ESlateVisibility::HitTestInvisible);
		UHorizontalBoxSlot* CrownSlot = Header->AddChildToHorizontalBox(Sized(TEXT("Size_ScoreboardCrown"), MakeHudImage(TEXT("Image_ScoreboardCrown"), TEXT("ico/ico_crown"), FVector2D(28.f, 28.f)), 28.f, 28.f));
		CrownSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		CrownSlot->SetVerticalAlignment(VAlign_Center);
		UHorizontalBoxSlot* TitleSlot = Header->AddChildToHorizontalBox(MakeHudText(TEXT("Label_Scoreboard"), TEXT("순위표"), 15, White, ETextJustify::Left, 2));
		TitleSlot->SetVerticalAlignment(VAlign_Center);
		UVerticalBoxSlot* HeaderSlot = AddV(Inner, Sized(TEXT("Size_ScoreboardHeader"), Header, 0.f, 28.f), 6.f);
		HeaderSlot->SetHorizontalAlignment(HAlign_Center);
		HeaderSlot->SetVerticalAlignment(VAlign_Center);

		const FLinearColor ColumnColor(0.42f, 0.25f, 0.48f, 1.f);
		UHorizontalBox* Columns = Make<UHorizontalBox>(TEXT("Row_ScoreboardColumns"));
		Columns->SetVisibility(ESlateVisibility::HitTestInvisible);
		AddH(Columns, Sized(TEXT("Size_ColumnRank"), MakeHudText(TEXT("Label_ColumnRank"), TEXT("순위"), 14, ColumnColor, ETextJustify::Center, 0), 52.f, 0.f), 0.f, false);
		AddH(Columns, MakeHudText(TEXT("Label_ColumnPlayer"), TEXT("플레이어"), 14, ColumnColor, ETextJustify::Left, 0), 0.f, true);
		AddH(Columns, Sized(TEXT("Size_ColumnTail"), MakeHudText(TEXT("Label_ColumnTail"), TEXT("꼬리"), 14, ColumnColor, ETextJustify::Right, 0), 56.f, 0.f), 0.f, false);
		AddV(Inner, Columns, 2.f);

		UImage* Line = Make<UImage>(TEXT("Image_ScoreboardLine"));
		Line->SetColorAndOpacity(FLinearColor(0.33f, 0.02f, 0.22f, 1.f));
		Line->SetVisibility(ESlateVisibility::HitTestInvisible);
		AddV(Inner, Sized(TEXT("Size_ScoreboardLine"), Line, 0.f, 2.f), 4.f);

		UVerticalBox* Rows = Make<UVerticalBox>(TEXT("VerticalBox_Scoreboard"));
		Rows->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		AddV(Inner, Rows, 0.f);

		USizeBox* Width = Sized(TEXT("Size_Scoreboard"), Card, 280.f, 0.f);
		PlaceHud(Canvas, Width, FVector2D(-40.f, 28.f), FVector2D::ZeroVector, TopRight, TopRight);
	}

	// 왼쪽 아래: 내 꼬리
	{
		UCanvasPanel* Group = NewGroup(TEXT("Group_Tails"));
		PlaceHud(Canvas, Group, FVector2D(40.f, -30.f), FVector2D(265.f, 70.f), BottomLeft, BottomLeft);
		PlaceHud(Group, MakeHudImage(TEXT("Image_TailsPanel"), TEXT("UI_Tail"), FVector2D(265.f, 70.f)), FVector2D::ZeroVector, FVector2D(265.f, 70.f), TopLeft, TopLeft);
		PlaceHud(Group, FitBox(TEXT("Fit_Tails"), MakeHudText(TEXT("Label_Tails"), TEXT("내 꼬리"), 22, White, ETextJustify::Left, 2)), FVector2D(76.f, 24.f), FVector2D(110.f, 32.f), TopLeft, TopLeft);
		PlaceHud(Group, MakeHudText(TEXT("Text_TailCount"), TEXT("0"), 28, HudDarkText, ETextJustify::Center, 0), FVector2D(184.f, -8.f), FVector2D(66.f, 57.543545f), TopLeft, TopLeft);
	}

	// 오른쪽 아래: 키 안내 + 쿨타임 (키 칸 위에 시계 방향 덮개, 이름 옆에 남은 시간)
	{
		UHorizontalBox* Keys = Make<UHorizontalBox>(TEXT("Layout_Keys"));
		Keys->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		struct FKeyHint { const TCHAR* Key; const TCHAR* Label; const TCHAR* Id; const TCHAR* KeyTexture; float KeyWidth; const TCHAR* Cool; const TCHAR* CoolText; };
		const FKeyHint Hints[] = {
			{ TEXT("Q"),     TEXT("꼬리잡기"), TEXT("Grab"), TEXT("UI_KeyButton_Small"),   41.f, TEXT("Image_CoolTime_1"), TEXT("Text_GrabCooldown") },
			{ TEXT("E"),     TEXT("밀기"),     TEXT("Push"), TEXT("UI_KeyButton_Small"),   41.f, TEXT("Image_CoolTime_2"), TEXT("Text_PushCooldown") },
			{ TEXT("SHIFT"), TEXT("달리기"),   TEXT("Run"),  TEXT("UI_KeyButton_Wide"), 88.f, TEXT("Image_CoolTime"),   TEXT("Text_RunCooldown") },
		};

		for (const FKeyHint& Hint : Hints)
		{
			const FString Id(Hint.Id);
			UHorizontalBox* Item = Make<UHorizontalBox>(*FString::Printf(TEXT("Row_Key%s"), *Id));
			Item->SetVisibility(ESlateVisibility::HitTestInvisible);

			UOverlay* KeyOverlay = Make<UOverlay>(*FString::Printf(TEXT("Overlay_Key%s"), *Id));
			KeyOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
			UOverlaySlot* KeyImageSlot = KeyOverlay->AddChildToOverlay(MakeHudImage(*FString::Printf(TEXT("Image_Key%s"), *Id), Hint.KeyTexture, FVector2D(Hint.KeyWidth, 36.f)));
			KeyImageSlot->SetHorizontalAlignment(HAlign_Fill);
			KeyImageSlot->SetVerticalAlignment(VAlign_Fill);

			UImage* Cool = MakeHudImage(Hint.Cool, Hint.KeyTexture, FVector2D(Hint.KeyWidth, 36.f));
			if (CooldownMaterial)
			{
				FSlateBrush Brush = Cool->GetBrush();
				Brush.SetResourceObject(CooldownMaterial);
				Cool->SetBrush(Brush);
			}
			Cool->SetVisibility(ESlateVisibility::Collapsed);
			UOverlaySlot* CoolSlot = KeyOverlay->AddChildToOverlay(Cool);
			CoolSlot->SetHorizontalAlignment(HAlign_Fill);
			CoolSlot->SetVerticalAlignment(VAlign_Fill);

			UOverlaySlot* KeyTextSlot = KeyOverlay->AddChildToOverlay(MakeHudText(*FString::Printf(TEXT("Text_Key%s"), *Id), Hint.Key, 15, White, ETextJustify::Center, 2, FLinearColor(0.f, 0.15f, 0.4f, 1.f)));
			KeyTextSlot->SetHorizontalAlignment(HAlign_Center);
			KeyTextSlot->SetVerticalAlignment(VAlign_Center);
			KeyTextSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 5.f));

			UHorizontalBoxSlot* KeySlot = Item->AddChildToHorizontalBox(Sized(*FString::Printf(TEXT("Size_Key%s"), *Id), KeyOverlay, Hint.KeyWidth, 36.f));
			KeySlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
			KeySlot->SetVerticalAlignment(VAlign_Center);

			UHorizontalBoxSlot* LabelSlot = Item->AddChildToHorizontalBox(MakeHudText(*FString::Printf(TEXT("Label_Key%s"), *Id), Hint.Label, 20, White, ETextJustify::Left, 2, FLinearColor(0.f, 0.f, 0.f, 0.85f)));
			LabelSlot->SetVerticalAlignment(VAlign_Center);

			UTextBlock* CoolText = MakeHudText(Hint.CoolText, TEXT(""), 20, White, ETextJustify::Left, 2, FLinearColor(0.f, 0.f, 0.f, 0.9f));
			CoolText->SetVisibility(ESlateVisibility::Hidden);
			// 고정 폭 칸: 숫자가 나타나거나 사라져도 줄 길이가 변하지 않음
			UHorizontalBoxSlot* CoolTextSlot = Item->AddChildToHorizontalBox(Sized(*FString::Printf(TEXT("Size_Cool%s"), *Id), CoolText, 34.f, 0.f));
			CoolTextSlot->SetPadding(FMargin(6.f, 0.f, 0.f, 0.f));
			CoolTextSlot->SetVerticalAlignment(VAlign_Center);

			UHorizontalBoxSlot* ItemSlot = Keys->AddChildToHorizontalBox(Item);
			ItemSlot->SetPadding(FMargin(20.f, 0.f, 0.f, 0.f));
		}
		PlaceHud(Canvas, Keys, FVector2D(-40.f, -34.f), FVector2D::ZeroVector, BottomRight, BottomRight);
	}

	// 상태 문구 (대기/카운트다운 안내)
	{
		UTextBlock* Phase = MakeHudText(TEXT("Text_Phase"), TEXT("플레이어를 기다리는 중"), 28, White, ETextJustify::Center, 3, FLinearColor(0.f, 0.f, 0.f, 0.85f));
		PlaceHud(Canvas, Phase, FVector2D(0.f, 160.f), FVector2D(800.f, 40.f), TopCenter, TopCenter);
	}

	// 가운데 카운트다운
	{
		UCanvasPanel* Group = Make<UCanvasPanel>(TEXT("Widget_Countdown"));
		Group->SetVisibility(ESlateVisibility::Collapsed);
		PlaceHud(Canvas, Group, FVector2D(0.f, -80.f), FVector2D(360.f, 240.f), Center, Center);
		PlaceHud(Group, MakeHudText(TEXT("Text_Countdown"), TEXT("5"), 130, White, ETextJustify::Center, 5), FVector2D(0.f, -36.f), FVector2D(360.f, 170.f), TopLeft, TopLeft);
		PlaceHud(Group, MakeHudText(TEXT("Label_Countdown"), TEXT("라운드 시작"), 30, White, ETextJustify::Center, 3, FLinearColor(0.f, 0.f, 0.f, 0.85f)), FVector2D(0.f, 180.f), FVector2D(360.f, 44.f), TopLeft, TopLeft);
	}

	// 유령 안내
	{
		UTextBlock* Ghost = MakeHudText(TEXT("Widget_Ghost"), TEXT("유령 상태 · 다음 라운드에 부활"), 26, White, ETextJustify::Center, 3, FLinearColor(0.f, 0.f, 0.f, 0.85f));
		Ghost->SetVisibility(ESlateVisibility::Collapsed);
		PlaceHud(Canvas, Ghost, FVector2D(0.f, -170.f), FVector2D(800.f, 40.f), FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f));
	}

	return CloseTree(WidgetBlueprint);
#else
	return -1;
#endif
}

int32 UCTUIBuildTools::AddVoiceSettings(UBlueprint* WidgetBlueprint)
{
#if WITH_EDITOR
	if (!WidgetBlueprint) { return -1; }

	const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
	Tree = TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
	UVerticalBox* List = Tree ? Cast<UVerticalBox>(Tree->FindWidget(TEXT("List_Settings"))) : nullptr;
	UWidget* MuteRow = Tree ? Tree->FindWidget(TEXT("Size_CheckBox_Mute")) : nullptr;
	if (!List || !MuteRow)
	{
		Tree = nullptr;
		return -1;
	}
	if (Tree->FindWidget(TEXT("Slider_Voice")))
	{
		Tree = nullptr;
		return 0; // 이미 있음
	}
	WidgetCount = 0;
	BlueprintName = TEXT("WBP_CTSettingsWidget");

	// 기존 글자 모양(글꼴, 색)을 그대로 따라 함
	auto MatchText = [](const TCHAR* NewName, const TCHAR* OldName)
	{
		UTextBlock* NewText = Cast<UTextBlock>(Tree->FindWidget(FName(NewName)));
		const UTextBlock* OldText = Cast<UTextBlock>(Tree->FindWidget(FName(OldName)));
		if (NewText && OldText)
		{
			NewText->SetFont(OldText->GetFont());
			NewText->SetColorAndOpacity(OldText->GetColorAndOpacity());
		}
	};

	int32 InsertIndex = List->GetChildIndex(MuteRow) + 1;
	auto Insert = [&](UWidget* Widget, float BottomPadding)
	{
		UVerticalBoxSlot* BoxSlot = Cast<UVerticalBoxSlot>(List->InsertChildAt(InsertIndex++, Widget));
		BoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, BottomPadding));
		BoxSlot->SetHorizontalAlignment(HAlign_Fill);
		BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	};

	Insert(MakeText(TEXT("Label_Voice"), TEXT("음성 채팅"), TEXT("Medium"), 24, TextTitle), 14.f);
	Insert(MakeCheckRow(TEXT("Size_CheckBox_MicEnabled"), TEXT("CheckBox_MicEnabled"), TEXT("Label_CheckBox_MicEnabled"), TEXT("마이크 사용")), 18.f);
	Insert(MakeSliderGroup(TEXT("Group_Voice"), TEXT("Label_VoiceVolume"), TEXT("음성 볼륨"), TEXT("Row_Voice"), TEXT("Height_Voice"), TEXT("Slider_Voice"), TEXT("Width_VoiceValue"), TEXT("Text_VoiceValue")), 18.f);
	Insert(MakeSliderGroup(TEXT("Group_MicSensitivity"), TEXT("Label_MicSensitivity"), TEXT("마이크 감도"), TEXT("Row_MicSensitivity"), TEXT("Height_MicSensitivity"), TEXT("Slider_MicSensitivity"), TEXT("Width_MicSensitivityValue"), TEXT("Text_MicSensitivityValue")), 24.f);

	MatchText(TEXT("Label_Voice"), TEXT("Label_Audio"));
	MatchText(TEXT("Label_VoiceVolume"), TEXT("Label_Sfx"));
	MatchText(TEXT("Label_MicSensitivity"), TEXT("Label_Sfx"));
	MatchText(TEXT("Text_VoiceValue"), TEXT("Text_SfxValue"));
	MatchText(TEXT("Text_MicSensitivityValue"), TEXT("Text_SfxValue"));
	MatchText(TEXT("Label_CheckBox_MicEnabled"), TEXT("Label_CheckBox_Mute"));

	return CloseTree(WidgetBlueprint);
#else
	return -1;
#endif
}

int32 UCTUIBuildTools::AddMatchSettingRow(UBlueprint* WidgetBlueprint, int32 SettingIndex)
{
#if WITH_EDITOR
	if (!WidgetBlueprint || SettingIndex < 0 || SettingIndex >= static_cast<int32>(ECTMatchSetting::Count)) { return -1; }

	const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
	Tree = TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
	UVerticalBox* Panel = Tree ? Cast<UVerticalBox>(Tree->FindWidget(TEXT("Panel_MatchSettings"))) : nullptr;
	if (!Panel)
	{
		Tree = nullptr;
		return -1;
	}

	const ECTMatchSetting Setting = static_cast<ECTMatchSetting>(SettingIndex);
	const FString Key = FCTMatchSettings::GetKey(Setting).ToString();
	if (Tree->FindWidget(*FString::Printf(TEXT("Row_Setting_%s"), *Key)))
	{
		Tree = nullptr;
		return 0; // 이미 있음
	}
	WidgetCount = 0;
	BlueprintName = TEXT("WBP_CTLobbyWidget");

	const FMargin ArrowMargin(0.101266f, 0.192771f, 0.101266f, 0.192771f);

	UHorizontalBox* Row = Make<UHorizontalBox>(*FString::Printf(TEXT("Row_Setting_%s"), *Key));
	Row->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	UTextBlock* Label = MakeText(*FString::Printf(TEXT("Label_Setting_%s"), *Key), TEXT(""), TEXT("Regular"), 22, TextDim);
	Label->SetText(FCTMatchSettings::GetLabel(Setting));
	USizeBox* LabelBox = Sized(*FString::Printf(TEXT("Size_Label_Setting_%s"), *Key), Label, 0.f, 56.f);
	Cast<USizeBoxSlot>(LabelBox->GetContentSlot())->SetVerticalAlignment(VAlign_Center);
	AddH(Row, LabelBox, 12.f, true);

	auto MakeArrow = [&](const TCHAR* Suffix, const TCHAR* Glyph)
	{
		UButton* Arrow = MakeButton(*FString::Printf(TEXT("Button_Setting_%s_%s"), *Key, Suffix), *FString::Printf(TEXT("Label_Button_Setting_%s_%s"), *Key, Suffix), TEXT(""), TEXT("UI_Old_Btn_Red"), ArrowMargin, FLinearColor::White);
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

	USizeBox* RowBox = Sized(*FString::Printf(TEXT("Size_Row_Setting_%s"), *Key), Row, 0.f, 56.f);
	AddV(Panel, RowBox, 18.f);

	// 게임 모드 줄은 목록 맨 위에 둠 (다른 줄은 아래로 덧붙임)
	if (Setting == ECTMatchSetting::Mode) { Panel->ShiftChild(0, RowBox); }

	return CloseTree(WidgetBlueprint);
#else
	return -1;
#endif
}

bool UCTUIBuildTools::SetCanvasSlotPlacement(UBlueprint* WidgetBlueprint, FName WidgetName, FVector2D AnchorMin, FVector2D AnchorMax, FVector2D Alignment, FVector2D Position, FVector2D SizeOrRightBottom)
{
#if WITH_EDITOR
	if (!WidgetBlueprint) { return false; }
	const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
	UWidgetTree* WidgetTree = TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
	UWidget* Widget = WidgetTree ? WidgetTree->FindWidget(WidgetName) : nullptr;
	UCanvasPanelSlot* CanvasSlot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
	if (!CanvasSlot) { return false; }

	CanvasSlot->SetAnchors(FAnchors(AnchorMin.X, AnchorMin.Y, AnchorMax.X, AnchorMax.Y));
	CanvasSlot->SetAlignment(Alignment);
	CanvasSlot->SetPosition(Position);
	CanvasSlot->SetSize(SizeOrRightBottom);
	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UCTUIBuildTools::SetHudKeyIcon(UBlueprint* WidgetBlueprint, FName OverlayName, FName TextName, FName IconName, UTexture2D* IconTexture, float IconSize)
{
#if WITH_EDITOR
	if (!WidgetBlueprint || !IconTexture) { return false; }
	const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
	UWidgetTree* WidgetTree = TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
	UOverlay* Overlay = WidgetTree ? Cast<UOverlay>(WidgetTree->FindWidget(OverlayName)) : nullptr;
	if (!Overlay) { return false; }

	UImage* Icon = Cast<UImage>(WidgetTree->FindWidget(IconName));
	if (!Icon)
	{
		Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), IconName);
		Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		UOverlaySlot* IconSlot = Overlay->AddChildToOverlay(Icon);
		IconSlot->SetHorizontalAlignment(HAlign_Center);
		IconSlot->SetVerticalAlignment(VAlign_Center);
	}
	FSlateBrush Brush;
	Brush.SetResourceObject(IconTexture);
	Brush.ImageSize = FVector2D(IconSize, IconSize);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Icon->SetBrush(Brush);

	if (UWidget* Text = WidgetTree->FindWidget(TextName)) { Text->SetVisibility(ESlateVisibility::Collapsed); }

	// 위젯 변수 표에 새 이름 등록
	Tree = WidgetTree;
	RegisterWidgetGuids(WidgetBlueprint);
	Tree = nullptr;
	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

#if WITH_EDITOR
namespace
{
	UWidgetTree* GetBlueprintTree(UBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint) { return nullptr; }
		const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
		return TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
	}

	FSlateBrush MakeImageBrush(UTexture2D* Texture, int32 DrawAs, const FVector4& Margin, const FLinearColor& Tint)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		if (Texture) { Brush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY()); }
		Brush.DrawAs = DrawAs == 1 ? ESlateBrushDrawType::Box : ESlateBrushDrawType::Image;
		Brush.Margin = FMargin(Margin.X, Margin.Y, Margin.Z, Margin.W);
		Brush.TintColor = Tint;
		return Brush;
	}
}
#endif

bool UCTUIBuildTools::PlaceCanvasImage(UBlueprint* WidgetBlueprint, FName ParentName, FName ImageName, UTexture2D* Texture, FVector2D AnchorMin, FVector2D AnchorMax, FVector2D Alignment, FVector2D Position, FVector2D SizeOrRightBottom, int32 ZOrder, int32 DrawAs, FVector4 Margin, FLinearColor Tint)
{
#if WITH_EDITOR
	UWidgetTree* WidgetTree = GetBlueprintTree(WidgetBlueprint);
	UCanvasPanel* Parent = WidgetTree ? Cast<UCanvasPanel>(WidgetTree->FindWidget(ParentName)) : nullptr;
	if (!Parent) { return false; }

	UImage* Image = Cast<UImage>(WidgetTree->FindWidget(ImageName));
	if (!Image)
	{
		Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), ImageName);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		Parent->AddChildToCanvas(Image);
	}
	Image->SetBrush(MakeImageBrush(Texture, DrawAs, Margin, Tint));
	Image->SetColorAndOpacity(FLinearColor::White);

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Image->Slot))
	{
		CanvasSlot->SetAnchors(FAnchors(AnchorMin.X, AnchorMin.Y, AnchorMax.X, AnchorMax.Y));
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetPosition(Position);
		CanvasSlot->SetSize(SizeOrRightBottom);
		CanvasSlot->SetZOrder(ZOrder);
	}

	Tree = WidgetTree;
	RegisterWidgetGuids(WidgetBlueprint);
	Tree = nullptr;
	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UCTUIBuildTools::SetButtonImage(UBlueprint* WidgetBlueprint, FName ButtonName, UTexture2D* Texture, int32 DrawAs, FVector4 Margin, float HoverFactor, float PressFactor)
{
#if WITH_EDITOR
	UWidgetTree* WidgetTree = GetBlueprintTree(WidgetBlueprint);
	UButton* Button = WidgetTree ? Cast<UButton>(WidgetTree->FindWidget(ButtonName)) : nullptr;
	if (!Button || !Texture) { return false; }

	FButtonStyle Style = Button->GetStyle();
	Style.SetNormal(MakeImageBrush(Texture, DrawAs, Margin, FLinearColor::White));
	Style.SetHovered(MakeImageBrush(Texture, DrawAs, Margin, FLinearColor(HoverFactor, HoverFactor, HoverFactor, 1.f)));
	Style.SetPressed(MakeImageBrush(Texture, DrawAs, Margin, FLinearColor(PressFactor, PressFactor, PressFactor, 1.f)));
	Style.SetDisabled(MakeImageBrush(Texture, DrawAs, Margin, FLinearColor(0.55f, 0.55f, 0.55f, 0.85f)));
	Style.SetNormalPadding(FMargin(0.f));
	Style.SetPressedPadding(FMargin(0.f));
	Button->SetStyle(Style);
	Button->SetBackgroundColor(FLinearColor::White);
	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UCTUIBuildTools::SetTextAppearance(UBlueprint* WidgetBlueprint, FName TextName, FLinearColor Color, int32 FontSize, FLinearColor OutlineColor, int32 OutlineSize)
{
#if WITH_EDITOR
	UWidgetTree* WidgetTree = GetBlueprintTree(WidgetBlueprint);
	UTextBlock* Text = WidgetTree ? Cast<UTextBlock>(WidgetTree->FindWidget(TextName)) : nullptr;
	if (!Text) { return false; }

	Text->SetColorAndOpacity(FSlateColor(Color));
	FSlateFontInfo Font = Text->GetFont();
	if (FontSize > 0) { Font.Size = FontSize; }
	Font.OutlineSettings.OutlineSize = OutlineSize;
	Font.OutlineSettings.OutlineColor = OutlineColor;
	Text->SetFont(Font);
	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UCTUIBuildTools::SetBorderImage(UBlueprint* WidgetBlueprint, FName BorderName, UTexture2D* Texture, int32 DrawAs, FVector4 Margin, FVector4 Padding)
{
#if WITH_EDITOR
	UWidgetTree* WidgetTree = GetBlueprintTree(WidgetBlueprint);
	UBorder* Border = WidgetTree ? Cast<UBorder>(WidgetTree->FindWidget(BorderName)) : nullptr;
	if (!Border || !Texture) { return false; }
	Border->SetBrush(MakeImageBrush(Texture, DrawAs, Margin, FLinearColor::White));
	Border->SetBrushColor(FLinearColor::White);
	Border->SetPadding(FMargin(Padding.X, Padding.Y, Padding.Z, Padding.W));
	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UCTUIBuildTools::WrapInBorder(UBlueprint* WidgetBlueprint, FName WidgetName, UTexture2D* Texture, int32 DrawAs, FVector4 Margin, FVector4 Padding)
{
#if WITH_EDITOR
	UWidgetTree* WidgetTree = GetBlueprintTree(WidgetBlueprint);
	UWidget* Widget = WidgetTree ? WidgetTree->FindWidget(WidgetName) : nullptr;
	if (!Widget || !Texture) { return false; }

	const FName PillName(*FString::Printf(TEXT("Pill_%s"), *WidgetName.ToString()));
	UBorder* Border = Cast<UBorder>(WidgetTree->FindWidget(PillName));
	if (!Border)
	{
		UPanelWidget* Parent = Widget->GetParent();
		const int32 Index = Parent ? Parent->GetChildIndex(Widget) : INDEX_NONE;
		if (Index == INDEX_NONE) { return false; }
		Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), PillName);
		Parent->ReplaceChildAt(Index, Border);
		Widget->Slot = nullptr;
		Border->SetContent(Widget);
	}
	Border->SetBrush(MakeImageBrush(Texture, DrawAs, Margin, FLinearColor::White));
	Border->SetBrushColor(FLinearColor::White);
	Border->SetPadding(FMargin(Padding.X, Padding.Y, Padding.Z, Padding.W));
	// 컴파일러가 변수 GUID를 요구하므로 변수로 두고 GUID 표에 등록
	Border->bIsVariable = true;

	Tree = WidgetTree;
	RegisterWidgetGuids(WidgetBlueprint);
	Tree = nullptr;
	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

#if WITH_EDITOR
namespace
{
	FSlateBrush MakeRoundedBrush(const FLinearColor& Fill, const FLinearColor& Outline, float Radius, float Width, const FVector2D& Size)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = Fill;
		Brush.ImageSize = Size;
		Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Brush.OutlineSettings.Color = FSlateColor(Outline);
		Brush.OutlineSettings.Width = Width;
		return Brush;
	}
}
#endif

bool UCTUIBuildTools::StyleSlider(UBlueprint* WidgetBlueprint, FName SliderName, FLinearColor BarColor, FLinearColor HandleColor, FLinearColor OutlineColor)
{
#if WITH_EDITOR
	UWidgetTree* WidgetTree = GetBlueprintTree(WidgetBlueprint);
	USlider* Slider = WidgetTree ? Cast<USlider>(WidgetTree->FindWidget(SliderName)) : nullptr;
	if (!Slider) { return false; }

	FSliderStyle Style = Slider->GetWidgetStyle();
	const FSlateBrush Bar = MakeRoundedBrush(BarColor, OutlineColor, 6.f, 0.f, FVector2D(12.f, 12.f));
	const FSlateBrush Handle = MakeRoundedBrush(HandleColor, OutlineColor, 15.f, 3.f, FVector2D(30.f, 30.f));
	Style.SetNormalBarImage(Bar);
	Style.SetHoveredBarImage(Bar);
	Style.SetDisabledBarImage(Bar);
	Style.SetNormalThumbImage(Handle);
	Style.SetHoveredThumbImage(Handle);
	Style.SetDisabledThumbImage(Handle);
	Style.SetBarThickness(12.f);
	Slider->SetWidgetStyle(Style);
	Slider->SetSliderBarColor(FLinearColor::White);
	Slider->SetSliderHandleColor(FLinearColor::White);
	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UCTUIBuildTools::StyleEditableTextBox(UBlueprint* WidgetBlueprint, FName BoxName, FLinearColor Fill, FLinearColor FocusedFill, FLinearColor OutlineColor, FLinearColor TextColor, float Radius)
{
#if WITH_EDITOR
	UWidgetTree* WidgetTree = GetBlueprintTree(WidgetBlueprint);
	UEditableTextBox* Box = WidgetTree ? Cast<UEditableTextBox>(WidgetTree->FindWidget(BoxName)) : nullptr;
	if (!Box) { return false; }

	const FVector2D Size(16.f, 16.f);
	const FSlateBrush Normal = MakeRoundedBrush(Fill, OutlineColor, Radius, 2.f, Size);
	const FSlateBrush Focused = MakeRoundedBrush(FocusedFill, OutlineColor, Radius, 2.f, Size);

	FEditableTextBoxStyle Style = Box->WidgetStyle;
	Style.SetBackgroundImageNormal(Normal);
	Style.SetBackgroundImageHovered(Normal);
	Style.SetBackgroundImageFocused(Focused);
	Style.SetBackgroundImageReadOnly(Normal);
	Style.SetForegroundColor(FSlateColor(TextColor));
	Style.SetFocusedForegroundColor(FSlateColor(TextColor));
	Style.SetReadOnlyForegroundColor(FSlateColor(TextColor));
	Style.TextStyle.SetColorAndOpacity(FSlateColor(TextColor));
	Box->WidgetStyle = Style;
	Box->SetForegroundColor(TextColor);

	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UCTUIBuildTools::StyleCheckBox(UBlueprint* WidgetBlueprint, FName CheckBoxName, FLinearColor UncheckedFill, FLinearColor CheckedFill, FLinearColor OutlineColor, float BoxSize)
{
#if WITH_EDITOR
	UWidgetTree* WidgetTree = GetBlueprintTree(WidgetBlueprint);
	UCheckBox* CheckBox = WidgetTree ? Cast<UCheckBox>(WidgetTree->FindWidget(CheckBoxName)) : nullptr;
	if (!CheckBox) { return false; }

	FCheckBoxStyle Style = CheckBox->GetWidgetStyle();
	const FVector2D Size(BoxSize, BoxSize);
	const FSlateBrush Off = MakeRoundedBrush(UncheckedFill, OutlineColor, 8.f, 3.f, Size);
	const FSlateBrush On = MakeRoundedBrush(CheckedFill, OutlineColor, 8.f, 3.f, Size);
	Style.SetUncheckedImage(Off);
	Style.SetUncheckedHoveredImage(Off);
	Style.SetUncheckedPressedImage(Off);
	Style.SetCheckedImage(On);
	Style.SetCheckedHoveredImage(On);
	Style.SetCheckedPressedImage(On);
	Style.SetUndeterminedImage(On);
	Style.SetUndeterminedHoveredImage(On);
	Style.SetUndeterminedPressedImage(On);
	Style.SetCheckBoxType(ESlateCheckBoxType::CheckBox);
	CheckBox->SetWidgetStyle(Style);
	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UCTUIBuildTools::StyleProgressBar(UBlueprint* WidgetBlueprint, FName BarName, FLinearColor Background, FLinearColor Fill, FLinearColor Outline)
{
#if WITH_EDITOR
	UWidgetTree* WidgetTree = GetBlueprintTree(WidgetBlueprint);
	UProgressBar* Bar = WidgetTree ? Cast<UProgressBar>(WidgetTree->FindWidget(BarName)) : nullptr;
	if (!Bar) { return false; }

	FProgressBarStyle Style = Bar->GetWidgetStyle();
	Style.SetBackgroundImage(MakeRoundedBrush(Background, Outline, 7.f, 3.f, FVector2D(14.f, 14.f)));
	Style.SetFillImage(MakeRoundedBrush(Fill, FLinearColor(0.f, 0.f, 0.f, 0.f), 7.f, 0.f, FVector2D(14.f, 14.f)));
	Bar->SetWidgetStyle(Style);
	Bar->SetFillColorAndOpacity(FLinearColor::White);
	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UCTUIBuildTools::SetImageRoundedBox(UBlueprint* WidgetBlueprint, FName ImageName, FLinearColor Fill, FLinearColor Outline, float Radius, float OutlineWidth)
{
#if WITH_EDITOR
	UWidgetTree* WidgetTree = GetBlueprintTree(WidgetBlueprint);
	UImage* Image = WidgetTree ? Cast<UImage>(WidgetTree->FindWidget(ImageName)) : nullptr;
	if (!Image) { return false; }
	Image->SetBrush(MakeRoundedBrush(Fill, Outline, Radius, OutlineWidth, FVector2D(32.f, 32.f)));
	Image->SetColorAndOpacity(FLinearColor::White);
	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

#if WITH_EDITOR
namespace
{
	bool IsNewStyleBrush(const FSlateBrush& Brush)
	{
		if (Brush.DrawAs == ESlateBrushDrawType::RoundedBox) { return true; }
		const UObject* Resource = Brush.GetResourceObject();
		if (!Resource) { return false; }
		const FString Path = Resource->GetPathName();
		return Path.Contains(TEXT("/2D/UI/Style/")) || Path.Contains(TEXT("/2D/UI/ico/"));
	}
}
#endif

int32 UCTUIBuildTools::FixImageTints(UBlueprint* WidgetBlueprint)
{
#if WITH_EDITOR
	UWidgetTree* WidgetTree = GetBlueprintTree(WidgetBlueprint);
	if (!WidgetTree) { return -1; }

	int32 Fixed = 0;
	TArray<UWidget*> All;
	WidgetTree->GetAllWidgets(All);
	for (UWidget* Widget : All)
	{
		if (UImage* Image = Cast<UImage>(Widget))
		{
			if (IsNewStyleBrush(Image->GetBrush()) && !Image->GetColorAndOpacity().Equals(FLinearColor::White))
			{
				Image->SetColorAndOpacity(FLinearColor::White);
				++Fixed;
			}
		}
		else if (UButton* Button = Cast<UButton>(Widget))
		{
			if (IsNewStyleBrush(Button->GetStyle().Normal))
			{
				Button->SetBackgroundColor(FLinearColor::White);
				Button->SetColorAndOpacity(FLinearColor::White);
				++Fixed;
			}
		}
		else if (UBorder* Border = Cast<UBorder>(Widget))
		{
			if (IsNewStyleBrush(Border->Background))
			{
				Border->SetBrushColor(FLinearColor::White);
				Border->SetContentColorAndOpacity(FLinearColor::White);
				++Fixed;
			}
		}
	}
	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	return Fixed;
#else
	return -1;
#endif
}

bool UCTUIBuildTools::SetButtonLabelPadding(UBlueprint* WidgetBlueprint, FName ButtonName, float BottomPadding)
{
#if WITH_EDITOR
	UWidgetTree* WidgetTree = GetBlueprintTree(WidgetBlueprint);
	UButton* Button = WidgetTree ? Cast<UButton>(WidgetTree->FindWidget(ButtonName)) : nullptr;
	UWidget* Content = Button ? Button->GetContent() : nullptr;
	UButtonSlot* ButtonSlot = Content ? Cast<UButtonSlot>(Content->Slot) : nullptr;
	if (!ButtonSlot) { return false; }

	ButtonSlot->SetPadding(FMargin(0.f, 0.f, 0.f, BottomPadding));
	ButtonSlot->SetHorizontalAlignment(HAlign_Center);
	ButtonSlot->SetVerticalAlignment(VAlign_Center);
	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UCTUIBuildTools::AddButtonIcon(UBlueprint* WidgetBlueprint, FName ButtonName, UTexture2D* IconTexture, float IconSize, float Gap)
{
#if WITH_EDITOR
	UWidgetTree* WidgetTree = GetBlueprintTree(WidgetBlueprint);
	UButton* Button = WidgetTree ? Cast<UButton>(WidgetTree->FindWidget(ButtonName)) : nullptr;
	if (!Button || !IconTexture) { return false; }

	const FName RowName(*FString::Printf(TEXT("Row_Icon_%s"), *ButtonName.ToString()));
	const FName IconName(*FString::Printf(TEXT("Icon_%s"), *ButtonName.ToString()));
	UHorizontalBox* Row = Cast<UHorizontalBox>(WidgetTree->FindWidget(RowName));
	UImage* Icon = Cast<UImage>(WidgetTree->FindWidget(IconName));

	if (!Row)
	{
		UWidget* Label = Button->GetContent();
		if (!Label) { return false; }
		Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), RowName);
		Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), IconName);
		Icon->SetVisibility(ESlateVisibility::HitTestInvisible);

		Button->ClearChildren();
		Label->Slot = nullptr;
		UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(Icon);
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(0.f, 0.f, Gap, 0.f));
		UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label);
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		Button->SetContent(Row);
	}
	if (!Icon) { return false; }

	FSlateBrush Brush;
	Brush.SetResourceObject(IconTexture);
	Brush.ImageSize = FVector2D(IconSize, IconSize);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Icon->SetBrush(Brush);
	Icon->SetColorAndOpacity(FLinearColor::White);
	if (UHorizontalBoxSlot* IconSlot = Cast<UHorizontalBoxSlot>(Icon->Slot)) { IconSlot->SetPadding(FMargin(0.f, 0.f, Gap, 0.f)); }

	Tree = WidgetTree;
	RegisterWidgetGuids(WidgetBlueprint);
	Tree = nullptr;
	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UCTUIBuildTools::RemoveWidgetByName(UBlueprint* WidgetBlueprint, FName WidgetName)
{
#if WITH_EDITOR
	UWidgetTree* WidgetTree = GetBlueprintTree(WidgetBlueprint);
	UWidget* Widget = WidgetTree ? WidgetTree->FindWidget(WidgetName) : nullptr;
	if (!Widget) { return false; }
	WidgetTree->RemoveWidget(Widget);

	Tree = WidgetTree;
	RegisterWidgetGuids(WidgetBlueprint);
	Tree = nullptr;
	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UCTUIBuildTools::AdjustFontSize(UBlueprint* WidgetBlueprint, FName TextName, int32 Delta)
{
#if WITH_EDITOR
	UWidgetTree* WidgetTree = GetBlueprintTree(WidgetBlueprint);
	UTextBlock* Text = WidgetTree ? Cast<UTextBlock>(WidgetTree->FindWidget(TextName)) : nullptr;
	if (!Text) { return false; }
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = FMath::Max(8, Font.Size + Delta);
	Text->SetFont(Font);
	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UCTUIBuildTools::SetImageColorByName(UBlueprint* WidgetBlueprint, FName ImageName, FLinearColor Color)
{
#if WITH_EDITOR
	UWidgetTree* WidgetTree = GetBlueprintTree(WidgetBlueprint);
	UImage* Image = WidgetTree ? Cast<UImage>(WidgetTree->FindWidget(ImageName)) : nullptr;
	if (!Image) { return false; }
	Image->SetColorAndOpacity(Color);
	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UCTUIBuildTools::SetCanvasSlotZOrder(UBlueprint* WidgetBlueprint, FName WidgetName, int32 ZOrder)
{
#if WITH_EDITOR
	UWidgetTree* WidgetTree = GetBlueprintTree(WidgetBlueprint);
	UWidget* Widget = WidgetTree ? WidgetTree->FindWidget(WidgetName) : nullptr;
	UCanvasPanelSlot* CanvasSlot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
	if (!CanvasSlot) { return false; }
	CanvasSlot->SetZOrder(ZOrder);
	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UCTUIBuildTools::SetTextJustificationByName(UBlueprint* WidgetBlueprint, FName TextName, int32 Justification)
{
#if WITH_EDITOR
	UWidgetTree* WidgetTree = GetBlueprintTree(WidgetBlueprint);
	UTextBlock* Text = WidgetTree ? Cast<UTextBlock>(WidgetTree->FindWidget(TextName)) : nullptr;
	if (!Text) { return false; }
	Text->SetJustification(static_cast<ETextJustify::Type>(FMath::Clamp(Justification, 0, 2)));
	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UCTUIBuildTools::SetWidgetVisibilityByName(UBlueprint* WidgetBlueprint, FName WidgetName, int32 Visibility)
{
#if WITH_EDITOR
	if (!WidgetBlueprint) { return false; }
	const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
	UWidgetTree* WidgetTree = TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
	UWidget* Widget = WidgetTree ? WidgetTree->FindWidget(WidgetName) : nullptr;
	if (!Widget) { return false; }
	Widget->SetVisibility(static_cast<ESlateVisibility>(FMath::Clamp(Visibility, 0, 4)));
	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UCTUIBuildTools::SetSizeBoxOverride(UBlueprint* WidgetBlueprint, FName WidgetName, float Width, float Height)
{
#if WITH_EDITOR
	if (!WidgetBlueprint) { return false; }
	const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
	UWidgetTree* WidgetTree = TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
	USizeBox* Box = WidgetTree ? Cast<USizeBox>(WidgetTree->FindWidget(WidgetName)) : nullptr;
	if (!Box) { return false; }

	if (Width > 0.f) { Box->SetWidthOverride(Width); } else { Box->ClearWidthOverride(); }
	if (Height > 0.f) { Box->SetHeightOverride(Height); } else { Box->ClearHeightOverride(); }
	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

int32 UCTUIBuildTools::AddMicAlwaysOnSetting(UBlueprint* WidgetBlueprint)
{
#if WITH_EDITOR
	if (!WidgetBlueprint) { return -1; }

	const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
	Tree = TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
	UVerticalBox* List = Tree ? Cast<UVerticalBox>(Tree->FindWidget(TEXT("List_Settings"))) : nullptr;
	UWidget* MicRow = Tree ? Tree->FindWidget(TEXT("Size_CheckBox_MicEnabled")) : nullptr;
	if (!List || !MicRow)
	{
		Tree = nullptr;
		return -1;
	}
	if (Tree->FindWidget(TEXT("CheckBox_MicAlwaysOn")))
	{
		Tree = nullptr;
		return 0; // 이미 있음
	}
	WidgetCount = 0;
	BlueprintName = TEXT("WBP_CTSettingsWidget");

	UVerticalBoxSlot* BoxSlot = Cast<UVerticalBoxSlot>(List->InsertChildAt(List->GetChildIndex(MicRow) + 1,
		MakeCheckRow(TEXT("Size_CheckBox_MicAlwaysOn"), TEXT("CheckBox_MicAlwaysOn"), TEXT("Label_CheckBox_MicAlwaysOn"), TEXT("버튼 없이 말하기"))));
	BoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
	BoxSlot->SetHorizontalAlignment(HAlign_Fill);
	BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	// 기존 체크박스 글자 모양을 따라 함
	UTextBlock* NewText = Cast<UTextBlock>(Tree->FindWidget(TEXT("Label_CheckBox_MicAlwaysOn")));
	const UTextBlock* OldText = Cast<UTextBlock>(Tree->FindWidget(TEXT("Label_CheckBox_Mute")));
	if (NewText && OldText)
	{
		NewText->SetFont(OldText->GetFont());
		NewText->SetColorAndOpacity(OldText->GetColorAndOpacity());
	}

	return CloseTree(WidgetBlueprint);
#else
	return -1;
#endif
}

int32 UCTUIBuildTools::AddHudMicIndicator(UBlueprint* WidgetBlueprint)
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
	if (Tree->FindWidget(TEXT("Image_MicPanel")))
	{
		Tree = nullptr;
		return 0; // 이미 있음
	}
	WidgetCount = 0;
	BlueprintName = TEXT("WBP_CTHUDWidget");

	// 예전에 만든 32x32 아이콘만 있는 버전은 지우고 다시 만듦. 키 글자(Text_MicKey)와 배치는 유지
	UVerticalBox* Layout = Cast<UVerticalBox>(Tree->FindWidget(TEXT("Layout_Mic")));
	if (UWidget* OldIconBox = Tree->FindWidget(TEXT("Size_MicIcon")))
	{
		Tree->RemoveWidget(OldIconBox);
		if (UWidget* OldIcon = Tree->FindWidget(TEXT("Image_MicIcon"))) { Tree->RemoveWidget(OldIcon); }
	}
	if (!Layout)
	{
		Layout = Make<UVerticalBox>(TEXT("Layout_Mic"));
		Layout->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UTextBlock* KeyText = MakeHudText(TEXT("Text_MicKey"), TEXT("V"), 18, FLinearColor::White, ETextJustify::Center, 2, FLinearColor(0.f, 0.f, 0.f, 0.85f));
		AddV(Layout, KeyText, 0.f)->SetHorizontalAlignment(HAlign_Center);
		PlaceHud(Canvas, Layout, FVector2D(-40.f, 0.f), FVector2D::ZeroVector, FVector2D(1.f, 0.5f), FVector2D(1.f, 0.5f));
	}

	// 패널 124x65 (74x39 그림의 약 1.7배) 위에 아이콘과 파장 막대
	const FVector2D PanelSize(124.f, 65.f);
	UCanvasPanel* Group = Make<UCanvasPanel>(TEXT("Group_Mic"));
	Group->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	PlaceHud(Group, MakeHudImage(TEXT("Image_MicPanel"), TEXT("UI_Mic"), PanelSize), FVector2D::ZeroVector, PanelSize, FVector2D(0.f, 0.f), FVector2D(0.f, 0.f));
	PlaceHud(Group, MakeHudImage(TEXT("Image_MicIcon"), TEXT("ico/Ico_Mic"), FVector2D(40.f, 40.f)), FVector2D(14.f, 10.f), FVector2D(40.f, 40.f), FVector2D(0.f, 0.f), FVector2D(0.f, 0.f));

	// 파장 막대 3개: 높이가 오르내리는 가운데 정렬 막대
	UHorizontalBox* Waves = Make<UHorizontalBox>(TEXT("Row_MicWave"));
	Waves->SetVisibility(ESlateVisibility::HitTestInvisible);
	for (int32 Index = 1; Index <= 3; ++Index)
	{
		UImage* Bar = Make<UImage>(*FString::Printf(TEXT("Image_MicWave%d"), Index));
		Bar->SetVisibility(ESlateVisibility::HitTestInvisible);
		Bar->SetColorAndOpacity(FLinearColor::FromSRGBColor(FColor(0xFF, 0x27, 0x83)));
		USizeBox* BarBox = Sized(*FString::Printf(TEXT("Size_MicWave%d"), Index), Bar, 8.f, Index == 1 ? 14.f : (Index == 2 ? 24.f : 34.f));
		UHorizontalBoxSlot* BarSlot = Waves->AddChildToHorizontalBox(BarBox);
		BarSlot->SetPadding(FMargin(0.f, 0.f, Index < 3 ? 7.f : 0.f, 0.f));
		BarSlot->SetVerticalAlignment(VAlign_Center);
	}
	PlaceHud(Group, Waves, FVector2D(66.f, 8.f), FVector2D(40.f, 42.f), FVector2D(0.f, 0.f), FVector2D(0.f, 0.f));

	// 키 글자 위에 패널을 넣음
	UVerticalBoxSlot* GroupSlot = Cast<UVerticalBoxSlot>(Layout->InsertChildAt(0, Sized(TEXT("Size_MicGroup"), Group, PanelSize.X, PanelSize.Y)));
	GroupSlot->SetHorizontalAlignment(HAlign_Center);
	GroupSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));

	return CloseTree(WidgetBlueprint);
#else
	return -1;
#endif
}

int32 UCTUIBuildTools::SyncWidgetGuids(UBlueprint* WidgetBlueprint)
{
#if WITH_EDITOR
	if (!WidgetBlueprint) { return -1; }

	const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
	Tree = TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
	const FMapProperty* GuidMapProperty = CastField<FMapProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetVariableNameToGuidMap")));
	if (!Tree || !GuidMapProperty)
	{
		Tree = nullptr;
		return -1;
	}

	FScriptMapHelper GuidMap(GuidMapProperty, GuidMapProperty->ContainerPtrToValuePtr<void>(WidgetBlueprint));
	const int32 Before = GuidMap.Num();
	RegisterWidgetGuids(WidgetBlueprint);
	const int32 After = GuidMap.Num();

	WidgetBlueprint->Modify();
	WidgetBlueprint->MarkPackageDirty();
	Tree = nullptr;
	return FMath::Abs(After - Before);
#else
	return -1;
#endif
}

int32 UCTUIBuildTools::KeyUnlocalizedTexts(UBlueprint* WidgetBlueprint)
{
#if WITH_EDITOR
	if (!WidgetBlueprint) { return -1; }

	const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
	UWidgetTree* ExistingTree = TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
	if (!ExistingTree) { return -1; }

	int32 Changed = 0;
	TArray<UWidget*> All;
	ExistingTree->GetAllWidgets(All);
	for (UWidget* Widget : All)
	{
		UTextBlock* TextBlock = Cast<UTextBlock>(Widget);
		if (!TextBlock) { continue; }

		const FText Current = TextBlock->GetText();
		const FString Source = Current.ToString();
		bool bHasHangul = false;
		for (const TCHAR Char : Source) { if (Char >= 0xAC00 && Char <= 0xD7A3) { bHasHangul = true; break; } }
		const TOptional<FString> Namespace = FTextInspector::GetNamespace(Current);
		if (!bHasHangul || (Namespace.IsSet() && !Namespace->IsEmpty())) { continue; }

		const FString Key = FString::Printf(TEXT("%s.%s"), *WidgetBlueprint->GetName(), *Widget->GetName());
		TextBlock->SetText(FText::ChangeKey(FTextKey(TEXT("CT_UI")), FTextKey(Key), FText::FromString(Source)));
		++Changed;
	}

	if (Changed > 0)
	{
		WidgetBlueprint->Modify();
		WidgetBlueprint->MarkPackageDirty();
	}
	return Changed;
#else
	return -1;
#endif
}

int32 UCTUIBuildTools::BuildHudScoreEntry(UBlueprint* WidgetBlueprint)
{
#if WITH_EDITOR
	if (!WidgetBlueprint || !OpenTree(WidgetBlueprint, TEXT("WBP_CTHudScoreEntryWidget"))) { return -1; }

	const FLinearColor Yellow(0.99f, 0.85f, 0.21f, 1.f);

	UOverlay* Root = Make<UOverlay>(TEXT("Root_HudScoreEntry"));
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Tree->RootWidget = Root;

	// 내 줄 표시: 연한 분홍 바탕
	UImage* Highlight = Make<UImage>(TEXT("Widget_LocalHighlight"));
	Highlight->SetColorAndOpacity(FLinearColor(1.f, 0.31f, 0.64f, 0.22f));
	Highlight->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* HighlightSlot = Root->AddChildToOverlay(Highlight);
	HighlightSlot->SetHorizontalAlignment(HAlign_Fill);
	HighlightSlot->SetVerticalAlignment(VAlign_Fill);

	UHorizontalBox* Row = Make<UHorizontalBox>(TEXT("Row_HudScoreEntry"));
	Row->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UOverlaySlot* RowSlot = Root->AddChildToOverlay(Sized(TEXT("Size_HudScoreEntry"), Row, 0.f, 34.f));
	RowSlot->SetHorizontalAlignment(HAlign_Fill);
	RowSlot->SetVerticalAlignment(VAlign_Fill);

	// 순위 배지
	UTextBlock* Rank = MakeHudText(TEXT("Text_Rank"), TEXT("1"), 18, HudDarkText, ETextJustify::Center, 0);
	UBorder* Badge = MakeCard(TEXT("Card_HudRank"), Rank, Yellow, FMargin(0.f, 0.f, 0.f, 0.f));
	UHorizontalBoxSlot* BadgeSlot = Row->AddChildToHorizontalBox(Sized(TEXT("Size_HudRank"), Badge, 34.f, 26.f));
	BadgeSlot->SetPadding(FMargin(8.f, 0.f, 10.f, 0.f));
	BadgeSlot->SetVerticalAlignment(VAlign_Center);

	// 색 점
	UImage* Dot = Make<UImage>(TEXT("Image_ColorIcon"));
	Dot->SetVisibility(ESlateVisibility::HitTestInvisible);
	UHorizontalBoxSlot* DotSlot = Row->AddChildToHorizontalBox(Sized(TEXT("Size_HudColorIcon"), Dot, 18.f, 18.f));
	DotSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	DotSlot->SetVerticalAlignment(VAlign_Center);

	UTextBlock* Name = MakeHudText(TEXT("Text_PlayerName"), TEXT("PLAYER"), 17, HudDarkText, ETextJustify::Left, 0);
	Name->SetAutoWrapText(false);
	UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(Name);
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);

	UImage* Crown = MakeHudImage(TEXT("Image_Crown"), TEXT("ico/ico_crown"), FVector2D(22.f, 22.f));
	Crown->SetVisibility(ESlateVisibility::Collapsed);
	UHorizontalBoxSlot* CrownSlot = Row->AddChildToHorizontalBox(Sized(TEXT("Size_HudCrown"), Crown, 22.f, 22.f));
	CrownSlot->SetPadding(FMargin(4.f, 0.f, 4.f, 0.f));
	CrownSlot->SetVerticalAlignment(VAlign_Center);

	UTextBlock* Ghost = MakeHudText(TEXT("Widget_GhostMark"), TEXT("유령"), 13, FLinearColor(0.45f, 0.35f, 0.5f, 1.f), ETextJustify::Center, 0);
	Ghost->SetVisibility(ESlateVisibility::Collapsed);
	UHorizontalBoxSlot* GhostSlot = Row->AddChildToHorizontalBox(Ghost);
	GhostSlot->SetPadding(FMargin(0.f, 0.f, 4.f, 0.f));
	GhostSlot->SetVerticalAlignment(VAlign_Center);

	UTextBlock* Tails = MakeHudText(TEXT("Text_TailCount"), TEXT("0"), 18, HudDarkText, ETextJustify::Right, 0);
	UHorizontalBoxSlot* TailSlot = Row->AddChildToHorizontalBox(Sized(TEXT("Size_HudTailCount"), Tails, 50.f, 0.f));
	TailSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	TailSlot->SetVerticalAlignment(VAlign_Center);

	return CloseTree(WidgetBlueprint);
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
			Result += FString::Printf(TEXT(" pos=(%.1f,%.1f) size=(%.1f,%.1f) anchors=(%.2f,%.2f)-(%.2f,%.2f) align=(%.2f,%.2f)"),
				Offsets.Left, Offsets.Top, Offsets.Right, Offsets.Bottom,
				CanvasSlot->GetAnchors().Minimum.X, CanvasSlot->GetAnchors().Minimum.Y,
				CanvasSlot->GetAnchors().Maximum.X, CanvasSlot->GetAnchors().Maximum.Y,
				CanvasSlot->GetAlignment().X, CanvasSlot->GetAlignment().Y);
		}
		if (const USizeBox* Box = Cast<USizeBox>(Widget))
		{
			Result += FString::Printf(TEXT(" sizebox w=%s%.0f h=%s%.0f"), Box->bOverride_WidthOverride ? TEXT("") : TEXT("(off)"), Box->WidthOverride, Box->bOverride_HeightOverride ? TEXT("") : TEXT("(off)"), Box->HeightOverride);
		}
		if (const UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
		{
			Result += FString::Printf(TEXT(" text=\"%s\" font=%.0f wrap=%d"), *TextBlock->GetText().ToString(), TextBlock->GetFont().Size, TextBlock->GetAutoWrapText() ? 1 : 0);
		}
		if (const UImage* Img = Cast<UImage>(Widget))
		{
			const FSlateBrush& Brush = Img->GetBrush();
			const FLinearColor Tint = Brush.TintColor.GetSpecifiedColor();
			const FLinearColor Color = Img->GetColorAndOpacity();
			Result += FString::Printf(TEXT(" img=%s tint=(%.2f,%.2f,%.2f,%.2f) color=(%.2f,%.2f,%.2f,%.2f)"),
				Brush.GetResourceObject() ? *Brush.GetResourceObject()->GetName() : TEXT("none"), Tint.R, Tint.G, Tint.B, Tint.A, Color.R, Color.G, Color.B, Color.A);
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

	// 어두운 배경 + 가운데 카드
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

	// 제목: 세 언어 병기 (번역 제외)
	UTextBlock* Title = MakeText(TEXT("Label_LanguageTitle"), TEXT(""), TEXT("Bold"), 40, TextMain, ETextJustify::Center);
	Title->SetText(FText::FromString(TEXT("Language  언어  言語")));
	AddV(Layout, Title, 12.f);

	UTextBlock* Subtitle = MakeText(TEXT("Label_LanguageSubtitle"), TEXT(""), TEXT("Regular"), 20, TextDim, ETextJustify::Center);
	Subtitle->SetText(FText::FromString(TEXT("Select your language  /  언어를 선택하세요  /  言語を選択")));
	AddV(Layout, Subtitle, 36.f);

	// 언어 버튼: 각 언어 이름 (번역 제외)
	struct FLanguageButton { const TCHAR* Button; const TCHAR* Label; const TCHAR* Name; };
	const FLanguageButton LanguageButtons[] = {
		{ TEXT("Button_LanguageKo"), TEXT("Label_Button_LanguageKo"), TEXT("한국어") },
		{ TEXT("Button_LanguageEn"), TEXT("Label_Button_LanguageEn"), TEXT("English") },
		{ TEXT("Button_LanguageJa"), TEXT("Label_Button_LanguageJa"), TEXT("日本語") },
	};
	for (const FLanguageButton& Entry : LanguageButtons)
	{
		UButton* Button = MakeButton(Entry.Button, Entry.Label, TEXT(""), TEXT("UI_Old_Btn_Red"), ButtonMarginB, FLinearColor::White);
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
	UButton* Confirm = MakeButton(TEXT("Button_Confirm"), TEXT("Label_Button_Confirm"), TEXT("확인"), TEXT("UI_Old_Btn_Gray"), ButtonMarginB, FLinearColor::White);
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

	// 언어 콤보박스 -> 버튼 3개
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
			UButton* Button = MakeButton(Entry.Button, Entry.Label, TEXT(""), TEXT("UI_Old_Btn_Red"), ButtonMarginB, FLinearColor::White);
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

	// 조작 방법: 고정 안내
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

	// 버튼/체크박스 글자 줄바꿈 끔
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

	// 변수 위젯 이름-GUID 등록
	if (const FMapProperty* GuidMapProperty = CastField<FMapProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetVariableNameToGuidMap"))))
	{
		FScriptMapHelper GuidMap(GuidMapProperty, GuidMapProperty->ContainerPtrToValuePtr<void>(WidgetBlueprint));
		TArray<UWidget*> Current;

		// 지운 위젯 GUID 정리
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
		UButton* OpenButton = MakeButton(TEXT("Button_ChangeNameOpen"), TEXT("Label_Button_ChangeNameOpen"), TEXT("이름 변경"), TEXT("UI_Old_Btn_Yellow"), ButtonMarginA, FLinearColor(0.055f, 0.035f, 0.01f, 1.f));
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
		// 창 오른쪽 확장 (왼쪽에 설정 칸 자리)
		constexpr float Extra = 640.f;
		// Cap_MapDialog 는 축소 한도라 축소 비율만큼만 넓힘
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

		// 맵 목록: 오른쪽 이동, 위에 소제목 자리
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
				UButton* Arrow = MakeButton(*FString::Printf(TEXT("Button_Setting_%s_%s"), *Key, Suffix), *FString::Printf(TEXT("Label_Button_Setting_%s_%s"), *Key, Suffix), TEXT(""), TEXT("UI_Old_Btn_Red"), ArrowMargin, FLinearColor::White);
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

int32 UCTUIBuildTools::BuildLoadingScreen(UBlueprint* WidgetBlueprint, UTexture2D* BackgroundTexture)
{
#if WITH_EDITOR
	if (!WidgetBlueprint) { return 0; }

	const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
	Tree = TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
	if (!Tree) { return 0; }
	WidgetCount = 0;
	BlueprintName = TEXT("WBP_CTLoadingWidget");

	TArray<UWidget*> OldWidgets;
	Tree->GetAllWidgets(OldWidgets);
	for (UWidget* Old : OldWidgets) { Tree->RemoveWidget(Old); }
	Tree->RootWidget = nullptr;

	UOverlay* Root = Make<UOverlay>(TEXT("Root_Loading"));
	Root->SetVisibility(ESlateVisibility::Visible);
	Tree->RootWidget = Root;

	// 배경: 화면을 꽉 채우되 비율은 유지
	UImage* Background = Make<UImage>(TEXT("Image_LoadingBackground"));
	FSlateBrush BackgroundBrush;
	BackgroundBrush.SetResourceObject(BackgroundTexture);
	BackgroundBrush.DrawAs = ESlateBrushDrawType::Image;
	if (BackgroundTexture)
	{
		BackgroundBrush.ImageSize = FVector2D(BackgroundTexture->GetSizeX(), BackgroundTexture->GetSizeY());
	}
	Background->SetBrush(BackgroundBrush);
	Background->SetColorAndOpacity(BackgroundTexture ? FLinearColor::White : FLinearColor(0.012f, 0.02f, 0.04f, 1.f));
	Background->SetVisibility(ESlateVisibility::Visible);

	UScaleBox* BackgroundScale = Make<UScaleBox>(TEXT("Scale_LoadingBackground"));
	BackgroundScale->SetStretch(EStretch::ScaleToFill);
	BackgroundScale->SetVisibility(ESlateVisibility::Visible);
	UScaleBoxSlot* BackgroundScaleSlot = Cast<UScaleBoxSlot>(BackgroundScale->AddChild(Background));
	BackgroundScaleSlot->SetHorizontalAlignment(HAlign_Center);
	BackgroundScaleSlot->SetVerticalAlignment(VAlign_Center);
	UOverlaySlot* BackgroundSlot = Root->AddChildToOverlay(BackgroundScale);
	BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
	BackgroundSlot->SetVerticalAlignment(VAlign_Fill);

	// 글자가 잘 보이게 어둡게 덮음. 이 이미지가 클릭도 막음
	UImage* Dim = Make<UImage>(TEXT("Image_LoadingDim"));
	Dim->SetColorAndOpacity(FLinearColor(0.003f, 0.006f, 0.015f, 0.6f));
	Dim->SetVisibility(ESlateVisibility::Visible);
	UOverlaySlot* DimSlot = Root->AddChildToOverlay(Dim);
	DimSlot->SetHorizontalAlignment(HAlign_Fill);
	DimSlot->SetVerticalAlignment(VAlign_Fill);

	// 오른쪽 위: 참가 인원
	UTextBlock* PlayerCount = MakeText(TEXT("Text_PlayerCount"), TEXT("참가 인원 0/8"), TEXT("Bold"), 30, TextMain, ETextJustify::Center);
	UBorder* PlayerCountCard = MakeCard(TEXT("Card_PlayerCount"), PlayerCount, CardColor, FMargin(28.f, 14.f, 28.f, 14.f));
	UOverlaySlot* PlayerCountSlot = Root->AddChildToOverlay(PlayerCountCard);
	PlayerCountSlot->SetHorizontalAlignment(HAlign_Right);
	PlayerCountSlot->SetVerticalAlignment(VAlign_Top);
	PlayerCountSlot->SetPadding(FMargin(0.f, 48.f, 56.f, 0.f));

	// 아래 가운데: 로딩 문구, 진행 막대, 게임 설명
	UVerticalBox* Bottom = Make<UVerticalBox>(TEXT("Layout_LoadingBottom"));
	Bottom->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	AddV(Bottom, MakeText(TEXT("Text_Loading"), TEXT("로딩 중"), TEXT("Bold"), 48, TextMain, ETextJustify::Center), 20.f);

	UProgressBar* Progress = Make<UProgressBar>(TEXT("ProgressBar_Loading"));
	Progress->SetPercent(0.f);
	Progress->SetFillColorAndOpacity(TextTitle);
	Progress->SetVisibility(ESlateVisibility::HitTestInvisible);
	AddV(Bottom, Sized(TEXT("Size_ProgressBar_Loading"), Progress, 0.f, 14.f), 32.f);

	UVerticalBox* TipBox = Make<UVerticalBox>(TEXT("Layout_LoadingTip"));
	TipBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	AddV(TipBox, MakeText(TEXT("Label_LoadingTip"), TEXT("TIP"), TEXT("Bold"), 20, TextTitle, ETextJustify::Center), 6.f);
	AddV(TipBox, MakeText(TEXT("Text_Tip"), TEXT("목표 색의 플레이어에게서 꼬리를 빼앗으세요."), TEXT("Regular"), 26, TextMain, ETextJustify::Center), 0.f);
	AddV(Bottom, MakeCard(TEXT("Card_LoadingTip"), TipBox, CardColor, FMargin(32.f, 18.f, 32.f, 20.f)), 0.f);

	USizeBox* BottomSize = Sized(TEXT("Size_LoadingBottom"), Bottom, 1100.f, 0.f);
	UOverlaySlot* BottomSlot = Root->AddChildToOverlay(BottomSize);
	BottomSlot->SetHorizontalAlignment(HAlign_Center);
	BottomSlot->SetVerticalAlignment(VAlign_Bottom);
	BottomSlot->SetPadding(FMargin(40.f, 0.f, 40.f, 90.f));

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

int32 UCTUIBuildTools::BuildPauseMenu(UBlueprint* WidgetBlueprint)
{
#if WITH_EDITOR
	if (!WidgetBlueprint) { return 0; }

	const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
	Tree = TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
	if (!Tree) { return 0; }
	WidgetCount = 0;
	BlueprintName = TEXT("WBP_CTPauseMenuWidget");

	TArray<UWidget*> OldWidgets;
	Tree->GetAllWidgets(OldWidgets);
	for (UWidget* Old : OldWidgets) { Tree->RemoveWidget(Old); }
	Tree->RootWidget = nullptr;

	const FMargin ButtonMargin(0.101266f, 0.192771f, 0.101266f, 0.192771f);

	UOverlay* Root = Make<UOverlay>(TEXT("Root_Pause"));
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Tree->RootWidget = Root;

	auto AddFill = [Root](UWidget* Child)
	{
		UOverlaySlot* OverlaySlot = Root->AddChildToOverlay(Child);
		OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		OverlaySlot->SetVerticalAlignment(VAlign_Fill);
	};

	// 어두운 배경. 클릭을 막아 뒤의 게임 화면을 누르지 못하게 함
	UImage* Dim = Make<UImage>(TEXT("Image_PauseDim"));
	Dim->SetColorAndOpacity(FLinearColor(0.003f, 0.006f, 0.015f, 0.7f));
	Dim->SetVisibility(ESlateVisibility::Visible);
	AddFill(Dim);

	// 화면에 맞춰 줄어드는 가운데 정렬 상자
	auto MakeCentered = [](const TCHAR* ScaleName, UWidget* Design)
	{
		UScaleBox* Scale = Make<UScaleBox>(ScaleName);
		Scale->SetStretch(EStretch::ScaleToFit);
		Scale->SetStretchDirection(EStretchDirection::DownOnly);
		Scale->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UScaleBoxSlot* ScaleSlot = Cast<UScaleBoxSlot>(Scale->AddChild(Design));
		ScaleSlot->SetHorizontalAlignment(HAlign_Center);
		ScaleSlot->SetVerticalAlignment(VAlign_Center);
		return Scale;
	};

	// 메인 패널
	UVerticalBox* MainLayout = Make<UVerticalBox>(TEXT("Layout_PauseMain"));
	MainLayout->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	AddV(MainLayout, MakeText(TEXT("Label_PauseTitle"), TEXT("메뉴"), TEXT("Bold"), 44, TextMain, ETextJustify::Center), 8.f);
	AddV(MainLayout, MakeText(TEXT("Label_PauseSubtitle"), TEXT("경기는 계속 진행 중이에요"), TEXT("Regular"), 20, TextDim, ETextJustify::Center), 32.f);

	struct FMainButton { const TCHAR* Button; const TCHAR* Label; const TCHAR* Text; const TCHAR* Texture; float Gap; };
	const FMainButton MainButtons[] = {
		{ TEXT("Button_Resume"),   TEXT("Label_Button_Resume"),   TEXT("계속하기"), TEXT("UI_Old_Btn_Gray"), 16.f },
		{ TEXT("Button_Settings"), TEXT("Label_Button_Settings"), TEXT("설정"),     TEXT("UI_Old_Btn_Red"), 16.f },
		{ TEXT("Button_Forfeit"),  TEXT("Label_Button_Forfeit"),  TEXT("기권"),     TEXT("UI_Old_Btn_Red"), 6.f },
	};
	for (const FMainButton& Entry : MainButtons)
	{
		UButton* Button = MakeButton(Entry.Button, Entry.Label, Entry.Text, Entry.Texture, ButtonMargin, FLinearColor::White);
		AddV(MainLayout, Sized(*FString::Printf(TEXT("Size_%s"), Entry.Button), Button, 0.f, 68.f), Entry.Gap);
	}
	AddV(MainLayout, MakeText(TEXT("Text_ForfeitInfo"), TEXT("경기 시간이 절반 지나면 기권할 수 있어요."), TEXT("Regular"), 17, TextWarn, ETextJustify::Center), 16.f);
	UButton* QuitButton = MakeButton(TEXT("Button_Quit"), TEXT("Label_Button_Quit"), TEXT("게임 종료"), TEXT("UI_Old_Btn_Red"), ButtonMargin, FLinearColor::White);
	AddV(MainLayout, Sized(TEXT("Size_Button_Quit"), QuitButton, 0.f, 68.f), 0.f);

	UBorder* MainFrame = MakeCard(TEXT("Frame_PauseMain"), MainLayout, FLinearColor(0.012f, 0.02f, 0.04f, 1.f), FMargin(48.f, 40.f, 48.f, 44.f));
	UOverlay* MainPanel = Make<UOverlay>(TEXT("Panel_Main"));
	MainPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	{
		UOverlaySlot* Inner = MainPanel->AddChildToOverlay(MakeCentered(TEXT("Scale_PauseMain"), Sized(TEXT("Size_PauseMain"), MainFrame, 620.f, 0.f)));
		Inner->SetHorizontalAlignment(HAlign_Fill);
		Inner->SetVerticalAlignment(VAlign_Fill);
		Inner->SetPadding(FMargin(24.f));
	}
	AddFill(MainPanel);

	// 설정 패널: 실행 중에 설정 화면이 이 안에 들어옴
	UOverlay* SettingsPanel = Make<UOverlay>(TEXT("Panel_Settings"));
	SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	AddFill(SettingsPanel);

	// 확인 팝업
	UOverlay* ConfirmPanel = Make<UOverlay>(TEXT("Panel_Confirm"));
	ConfirmPanel->SetVisibility(ESlateVisibility::Collapsed);

	UImage* ConfirmDim = Make<UImage>(TEXT("Image_ConfirmDim"));
	ConfirmDim->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.55f));
	ConfirmDim->SetVisibility(ESlateVisibility::Visible);
	{
		UOverlaySlot* DimSlot = ConfirmPanel->AddChildToOverlay(ConfirmDim);
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* ConfirmLayout = Make<UVerticalBox>(TEXT("Layout_Confirm"));
	ConfirmLayout->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	AddV(ConfirmLayout, MakeText(TEXT("Text_ConfirmTitle"), TEXT("기권하시겠어요?"), TEXT("Bold"), 36, TextWarn, ETextJustify::Center), 20.f);
	AddV(ConfirmLayout, MakeText(TEXT("Text_ConfirmBody"), TEXT("기권하면 꼬리를 모두 잃고 이번 경기의 순위에서 제외돼요."), TEXT("Regular"), 22, TextMain, ETextJustify::Center), 36.f);

	UHorizontalBox* ConfirmButtons = Make<UHorizontalBox>(TEXT("Row_ConfirmButtons"));
	ConfirmButtons->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UButton* NoButton = MakeButton(TEXT("Button_ConfirmNo"), TEXT("Label_Button_ConfirmNo"), TEXT("취소"), TEXT("UI_Old_Btn_Red"), ButtonMargin, FLinearColor::White);
	UButton* YesButton = MakeButton(TEXT("Button_ConfirmYes"), TEXT("Label_Button_ConfirmYes"), TEXT("확인"), TEXT("UI_Old_Btn_Gray"), ButtonMargin, FLinearColor::White);
	AddH(ConfirmButtons, Sized(TEXT("Size_Button_ConfirmNo"), NoButton, 0.f, 64.f), 16.f, true);
	AddH(ConfirmButtons, Sized(TEXT("Size_Button_ConfirmYes"), YesButton, 0.f, 64.f), 0.f, true);
	AddV(ConfirmLayout, ConfirmButtons, 0.f);

	UBorder* ConfirmFrame = MakeCard(TEXT("Frame_Confirm"), ConfirmLayout, FLinearColor(0.012f, 0.02f, 0.04f, 1.f), FMargin(48.f, 40.f, 48.f, 40.f));
	{
		UOverlaySlot* Inner = ConfirmPanel->AddChildToOverlay(MakeCentered(TEXT("Scale_Confirm"), Sized(TEXT("Size_Confirm"), ConfirmFrame, 820.f, 0.f)));
		Inner->SetHorizontalAlignment(HAlign_Fill);
		Inner->SetVerticalAlignment(VAlign_Fill);
		Inner->SetPadding(FMargin(24.f));
	}
	AddFill(ConfirmPanel);

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

// 이모트 선택창 뼈대 (처음 한 번). 만든 뒤의 모양은 WBP 에서 직접 바꿈

#if WITH_EDITOR
namespace
{
	FSlateBrush StyleTextureBrush(const TCHAR* Name, const FMargin& Margin, ESlateBrushDrawType::Type DrawAs)
	{
		FSlateBrush Brush;
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/2D/UI/Style/%s.%s"), Name, Name));
		Brush.SetResourceObject(Texture);
		if (Texture) { Brush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY()); }
		Brush.DrawAs = DrawAs;
		Brush.Margin = Margin;
		return Brush;
	}

	// 투명 눌림 버튼 (눌림만 받고 모양은 위에 겹친 그림으로)
	void StyleClearButton(UButton* Button)
	{
		FButtonStyle Style = Button->GetStyle();
		Style.Normal = FSlateNoResource();
		FSlateBrush Hover;
		Hover.DrawAs = ESlateBrushDrawType::RoundedBox;
		Hover.TintColor = FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.25f));
		Hover.OutlineSettings.CornerRadii = FVector4(14.f, 14.f, 14.f, 14.f);
		Style.Hovered = Hover;
		FSlateBrush Press = Hover;
		Press.TintColor = FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.15f));
		Style.Pressed = Press;
		Button->SetStyle(Style);
	}
}
#endif

int32 UCTUIBuildTools::BuildEmoteCellDesign(UBlueprint* WidgetBlueprint)
{
#if WITH_EDITOR
	if (!WidgetBlueprint || !OpenTree(WidgetBlueprint, TEXT("WBP_CTEmoteCellWidget"))) { return -1; }

	const FLinearColor Magenta = FLinearColor::FromSRGBColor(FColor(0xFF, 0x27, 0x83));

	// 칸 사이 간격은 바깥 여백으로
	UBorder* Root = Make<UBorder>(TEXT("Border_CellRoot"));
	Root->SetBrushColor(FLinearColor::Transparent);
	Root->SetPadding(FMargin(4.f));
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Tree->RootWidget = Root;

	USizeBox* Size = Make<USizeBox>(TEXT("Size_Cell"));
	Size->SetWidthOverride(104.f);
	Size->SetHeightOverride(104.f);
	Size->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Root->SetContent(Size);

	UOverlay* Overlay = Make<UOverlay>(TEXT("Overlay_Cell"));
	Overlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Size->SetContent(Overlay);

	auto AddFill = [Overlay](UWidget* Child, const FMargin& Padding)
	{
		UOverlaySlot* OverlaySlot = Overlay->AddChildToOverlay(Child);
		OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		OverlaySlot->SetVerticalAlignment(VAlign_Fill);
		OverlaySlot->SetPadding(Padding);
	};

	// 배경: 테두리 없는 흰 상자
	UImage* Back = Make<UImage>(TEXT("Image_CellBack"));
	Back->SetBrush(StyleTextureBrush(TEXT("UI_Row_Flat"), FMargin(0.14f, 0.4f, 0.14f, 0.4f), ESlateBrushDrawType::Box));
	Back->SetVisibility(ESlateVisibility::HitTestInvisible);
	AddFill(Back, FMargin(0.f));

	// 이모지 그림: 그림은 코드가 채움 (크기와 여백은 여기서 정함)
	UImage* Icon = Make<UImage>(TEXT("Image_Icon"));
	Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
	AddFill(Icon, FMargin(6.f));

	// 고른 칸 표시 (분홍 테두리). 평소에는 숨김
	UImage* Selected = Make<UImage>(TEXT("Widget_Selected"));
	FSlateBrush SelectedBrush;
	SelectedBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	SelectedBrush.TintColor = FSlateColor(FLinearColor(1.f, 0.39f, 0.64f, 0.18f));
	SelectedBrush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(14.f, 14.f, 14.f, 14.f), FSlateColor(Magenta), 5.f);
	Selected->SetBrush(SelectedBrush);
	Selected->SetVisibility(ESlateVisibility::Collapsed);
	AddFill(Selected, FMargin(0.f));

	// 키 번호 (숫자 키 자리일 때만 코드가 보이게 함)
	UTextBlock* Key = Make<UTextBlock>(TEXT("Text_Key"));
	Key->SetText(Loc(TEXT("Text_Key"), TEXT("1")));
	Key->SetFont(MakeFont(TEXT("Bold"), 22));
	Key->SetColorAndOpacity(FSlateColor(Magenta));
	Key->SetVisibility(ESlateVisibility::HitTestInvisible);
	UOverlaySlot* KeySlot = Overlay->AddChildToOverlay(Key);
	KeySlot->SetHorizontalAlignment(HAlign_Left);
	KeySlot->SetVerticalAlignment(VAlign_Top);
	KeySlot->SetPadding(FMargin(8.f, 2.f, 0.f, 0.f));

	// 눌림만 받는 투명 버튼
	UButton* Button = Make<UButton>(TEXT("Button_Cell"));
	StyleClearButton(Button);
	AddFill(Button, FMargin(0.f));

	return CloseTree(WidgetBlueprint);
#else
	return 0;
#endif
}

int32 UCTUIBuildTools::BuildEmoteSelectDesign(UBlueprint* WidgetBlueprint)
{
#if WITH_EDITOR
	if (!WidgetBlueprint || !OpenTree(WidgetBlueprint, TEXT("WBP_CTEmoteSelectWidget"))) { return -1; }

	const FLinearColor DarkPurple(0.058f, 0.0006f, 0.032f, 1.f);

	UCanvasPanel* Canvas = Make<UCanvasPanel>(TEXT("Canvas_EmoteSelect"));
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Tree->RootWidget = Canvas;

	// 뒤를 어둡게 (뒤 화면 클릭 막음)
	UImage* Dim = Make<UImage>(TEXT("Image_EmoteDim"));
	Dim->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.45f));
	Dim->SetVisibility(ESlateVisibility::Visible);
	UCanvasPanelSlot* DimSlot = Cast<UCanvasPanelSlot>(Canvas->AddChild(Dim));
	DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	DimSlot->SetOffsets(FMargin(0.f));

	// 가운데 패널: 틀 그림 위에 내용
	UOverlay* Panel = Make<UOverlay>(TEXT("Overlay_EmotePanel"));
	Panel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(Canvas->AddChild(Panel));
	PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PanelSlot->SetAutoSize(true);

	UImage* Frame = Make<UImage>(TEXT("Image_EmotePanel"));
	Frame->SetBrush(StyleTextureBrush(TEXT("UI_Frame_Pink_L"), FMargin(0.018f, 0.12f, 0.018f, 0.026f), ESlateBrushDrawType::Box));
	Frame->SetVisibility(ESlateVisibility::Visible);
	UOverlaySlot* FrameSlot = Panel->AddChildToOverlay(Frame);
	FrameSlot->SetHorizontalAlignment(HAlign_Fill);
	FrameSlot->SetVerticalAlignment(VAlign_Fill);

	UVerticalBox* Column = Make<UVerticalBox>(TEXT("Column_Emote"));
	Column->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UOverlaySlot* ColumnSlot = Panel->AddChildToOverlay(Column);
	ColumnSlot->SetHorizontalAlignment(HAlign_Center);
	ColumnSlot->SetVerticalAlignment(VAlign_Fill);
	ColumnSlot->SetPadding(FMargin(40.f, 0.f, 40.f, 40.f));

	// 제목은 틀의 분홍 머리 부분에
	UTextBlock* Title = Make<UTextBlock>(TEXT("Text_EmoteTitle"));
	Title->SetText(Loc(TEXT("Text_EmoteTitle"), TEXT("이모트 설정")));
	FSlateFontInfo TitleFont = MakeFont(TEXT("Bold"), 44);
	TitleFont.OutlineSettings.OutlineSize = 3;
	TitleFont.OutlineSettings.OutlineColor = DarkPurple;
	Title->SetFont(TitleFont);
	Title->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Title->SetJustification(ETextJustify::Center);
	Title->SetVisibility(ESlateVisibility::HitTestInvisible);
	USizeBox* TitleBox = Sized(TEXT("Size_EmoteTitle"), Title, 0.f, 92.f);
	UVerticalBoxSlot* TitleSlot = Column->AddChildToVerticalBox(TitleBox);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetVerticalAlignment(VAlign_Center);

	AddV(Column, MakeText(TEXT("Text_EmoteHint"), TEXT("숫자 키 자리를 고른 뒤, 아래에서 넣을 이모트를 누르세요"), TEXT("Regular"), 20, DarkPurple, ETextJustify::Center), 14.f);

	// 숫자 키 자리 줄 (코드가 칸을 채움)
	UHorizontalBox* SlotRow = Make<UHorizontalBox>(TEXT("Row_EmoteSlots"));
	UVerticalBoxSlot* SlotRowSlot = Column->AddChildToVerticalBox(SlotRow);
	SlotRowSlot->SetHorizontalAlignment(HAlign_Center);
	SlotRowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));

	// 전체 이모트 격자 (코드가 칸을 채움)
	UUniformGridPanel* Grid = Make<UUniformGridPanel>(TEXT("Grid_EmoteLibrary"));
	UVerticalBoxSlot* GridSlot = Column->AddChildToVerticalBox(Grid);
	GridSlot->SetHorizontalAlignment(HAlign_Center);

	// 닫기 버튼
	UButton* Close = Make<UButton>(TEXT("Button_Close"));
	FButtonStyle CloseStyle = Close->GetStyle();
	const FSlateBrush CloseBrush = StyleTextureBrush(TEXT("UI_Btn_Pink_L"), FMargin(0.1f, 0.3f, 0.1f, 0.35f), ESlateBrushDrawType::Box);
	CloseStyle.Normal = CloseBrush;
	CloseStyle.Hovered = CloseBrush;
	CloseStyle.Hovered.TintColor = FSlateColor(FLinearColor(0.82f, 0.82f, 0.82f, 1.f));
	CloseStyle.Pressed = CloseBrush;
	CloseStyle.Pressed.TintColor = FSlateColor(FLinearColor(0.66f, 0.66f, 0.66f, 1.f));
	Close->SetStyle(CloseStyle);
	UTextBlock* CloseText = MakeText(TEXT("Label_Button_Close"), TEXT("닫기 (R)"), TEXT("Bold"), 24, FLinearColor::White, ETextJustify::Center);
	Close->SetContent(CloseText);
	USizeBox* CloseBox = Sized(TEXT("Size_Button_Close"), Close, 300.f, 80.f);
	UVerticalBoxSlot* CloseSlot = Column->AddChildToVerticalBox(CloseBox);
	CloseSlot->SetHorizontalAlignment(HAlign_Center);
	CloseSlot->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));

	return CloseTree(WidgetBlueprint);
#else
	return 0;
#endif
}

// 전적 팝업 뼈대 (처음 한 번). 만든 뒤의 모양은 WBP 에서 직접 바꿈

int32 UCTUIBuildTools::BuildMatchHistoryEntryDesign(UBlueprint* WidgetBlueprint)
{
#if WITH_EDITOR
	if (!WidgetBlueprint || !OpenTree(WidgetBlueprint, TEXT("WBP_CTMatchHistoryEntryWidget"))) { return -1; }

	const FLinearColor Magenta = FLinearColor::FromSRGBColor(FColor(0xFF, 0x27, 0x83));
	const FLinearColor DarkPurple(0.058f, 0.0006f, 0.032f, 1.f);

	// 줄 사이 간격은 바깥 여백으로
	UBorder* Root = Make<UBorder>(TEXT("Border_EntryRoot"));
	Root->SetBrushColor(FLinearColor::Transparent);
	Root->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Tree->RootWidget = Root;

	UOverlay* Overlay = Make<UOverlay>(TEXT("Overlay_Entry"));
	Overlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Root->SetContent(Overlay);

	// 배경: 테두리 없는 분홍 줄
	UImage* Back = Make<UImage>(TEXT("Image_EntryBack"));
	Back->SetBrush(StyleTextureBrush(TEXT("UI_Row_Flat"), FMargin(0.14f, 0.4f, 0.14f, 0.4f), ESlateBrushDrawType::Box));
	Back->SetColorAndOpacity(FLinearColor::FromSRGBColor(FColor(0xFF, 0xC2, 0xDC)));
	Back->SetVisibility(ESlateVisibility::HitTestInvisible);
	UOverlaySlot* BackSlot = Overlay->AddChildToOverlay(Back);
	BackSlot->SetHorizontalAlignment(HAlign_Fill);
	BackSlot->SetVerticalAlignment(VAlign_Fill);

	UHorizontalBox* Row = Make<UHorizontalBox>(TEXT("Row_Entry"));
	Row->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UOverlaySlot* RowSlot = Overlay->AddChildToOverlay(Row);
	RowSlot->SetHorizontalAlignment(HAlign_Fill);
	RowSlot->SetVerticalAlignment(VAlign_Fill);
	RowSlot->SetPadding(FMargin(24.f, 12.f, 24.f, 12.f));

	// 판 번호 + 맵 이름
	UTextBlock* Index = MakeText(TEXT("Text_Index"), TEXT("1"), TEXT("Bold"), 36, Magenta, ETextJustify::Center);
	Index->SetAutoWrapText(false);
	AddH(Row, Sized(TEXT("Size_Index"), Index, 56.f, 0.f), 10.f, false);

	UTextBlock* MapName = MakeText(TEXT("Text_MapName"), TEXT("시골맵"), TEXT("Bold"), 26, DarkPurple, ETextJustify::Left);
	AddH(Row, Sized(TEXT("Size_MapName"), MapName, 190.f, 0.f), 14.f, false);

	// 1~3등 칸
	for (int32 Slot = 1; Slot <= 3; ++Slot)
	{
		const FString Suffix = FString::FromInt(Slot);
		UOverlay* Cell = Make<UOverlay>(*(TEXT("Overlay_Cell") + Suffix));
		Cell->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UImage* CellBack = Make<UImage>(*(TEXT("Image_Cell") + Suffix));
		FSlateBrush CellBrush;
		CellBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
		CellBrush.TintColor = FSlateColor(FLinearColor::White);
		CellBrush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(12.f, 12.f, 12.f, 12.f), FSlateColor(Magenta), 3.f);
		CellBack->SetBrush(CellBrush);
		CellBack->SetVisibility(ESlateVisibility::HitTestInvisible);
		UOverlaySlot* CellBackSlot = Cell->AddChildToOverlay(CellBack);
		CellBackSlot->SetHorizontalAlignment(HAlign_Fill);
		CellBackSlot->SetVerticalAlignment(VAlign_Fill);

		UVerticalBox* Column = Make<UVerticalBox>(*(TEXT("Column_Cell") + Suffix));
		Column->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UOverlaySlot* ColumnSlot = Cell->AddChildToOverlay(Column);
		ColumnSlot->SetHorizontalAlignment(HAlign_Fill);
		ColumnSlot->SetVerticalAlignment(VAlign_Center);
		ColumnSlot->SetPadding(FMargin(6.f, 4.f, 6.f, 4.f));

		UTextBlock* Place = MakeText(*(TEXT("Text_Place") + Suffix), *Suffix, TEXT("Bold"), 18, Magenta, ETextJustify::Center);
		AddV(Column, Place, 0.f);
		UTextBlock* Name = MakeText(*(TEXT("Text_Name") + Suffix), TEXT("이름"), TEXT("Bold"), 22, DarkPurple, ETextJustify::Center);
		Name->SetAutoWrapText(false);
		AddV(Column, Name, 0.f);
		UTextBlock* Tail = MakeText(*(TEXT("Text_Tail") + Suffix), TEXT("0"), TEXT("Regular"), 17, DarkPurple, ETextJustify::Center);
		AddV(Column, Tail, 0.f);

		USizeBox* CellSize = Sized(*(TEXT("Size_Cell") + Suffix), Cell, 150.f, 0.f);
		CellSize->SetMinDesiredHeight(92.f);
		AddH(Row, CellSize, Slot < 3 ? 14.f : 0.f, false, VAlign_Fill);
	}

	return CloseTree(WidgetBlueprint);
#else
	return 0;
#endif
}

int32 UCTUIBuildTools::BuildMatchHistoryDesign(UBlueprint* WidgetBlueprint)
{
#if WITH_EDITOR
	if (!WidgetBlueprint || !OpenTree(WidgetBlueprint, TEXT("WBP_CTMatchHistoryWidget"))) { return -1; }

	const FLinearColor DarkPurple(0.058f, 0.0006f, 0.032f, 1.f);

	UCanvasPanel* Canvas = Make<UCanvasPanel>(TEXT("Canvas_History"));
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Tree->RootWidget = Canvas;

	// 뒤를 어둡게 (뒤 화면 클릭 막음)
	UImage* Dim = Make<UImage>(TEXT("Image_HistoryDim"));
	Dim->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.55f));
	Dim->SetVisibility(ESlateVisibility::Visible);
	UCanvasPanelSlot* DimSlot = Cast<UCanvasPanelSlot>(Canvas->AddChild(Dim));
	DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	DimSlot->SetOffsets(FMargin(0.f));

	UOverlay* Panel = Make<UOverlay>(TEXT("Overlay_HistoryPanel"));
	Panel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(Canvas->AddChild(Panel));
	PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PanelSlot->SetAutoSize(true);

	UImage* Frame = Make<UImage>(TEXT("Image_HistoryPanel"));
	Frame->SetBrush(StyleTextureBrush(TEXT("UI_Frame_Pink_L"), FMargin(0.018f, 0.12f, 0.018f, 0.026f), ESlateBrushDrawType::Box));
	Frame->SetVisibility(ESlateVisibility::Visible);
	UOverlaySlot* FrameSlot = Panel->AddChildToOverlay(Frame);
	FrameSlot->SetHorizontalAlignment(HAlign_Fill);
	FrameSlot->SetVerticalAlignment(VAlign_Fill);

	UVerticalBox* Column = Make<UVerticalBox>(TEXT("Column_History"));
	Column->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UOverlaySlot* ColumnSlot = Panel->AddChildToOverlay(Column);
	ColumnSlot->SetHorizontalAlignment(HAlign_Center);
	ColumnSlot->SetVerticalAlignment(VAlign_Fill);
	ColumnSlot->SetPadding(FMargin(40.f, 0.f, 40.f, 40.f));

	// 제목은 틀의 분홍 머리 부분에
	UTextBlock* Title = Make<UTextBlock>(TEXT("Text_HistoryTitle"));
	Title->SetText(Loc(TEXT("Text_HistoryTitle"), TEXT("방 전적")));
	FSlateFontInfo TitleFont = MakeFont(TEXT("Bold"), 44);
	TitleFont.OutlineSettings.OutlineSize = 3;
	TitleFont.OutlineSettings.OutlineColor = DarkPurple;
	Title->SetFont(TitleFont);
	Title->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Title->SetJustification(ETextJustify::Center);
	Title->SetVisibility(ESlateVisibility::HitTestInvisible);
	USizeBox* TitleBox = Sized(TEXT("Size_HistoryTitle"), Title, 0.f, 92.f);
	UVerticalBoxSlot* TitleSlot = Column->AddChildToVerticalBox(TitleBox);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetVerticalAlignment(VAlign_Center);

	AddV(Column, MakeText(TEXT("Text_Total"), TEXT("총 0판 / 1등 0번"), TEXT("Bold"), 24, DarkPurple, ETextJustify::Center), 14.f);

	// 목록 (코드가 줄을 채움). 없을 때는 안내 글자가 가운데에
	UOverlay* ListOverlay = Make<UOverlay>(TEXT("Overlay_HistoryList"));
	ListOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	UScrollBox* Scroll = Make<UScrollBox>(TEXT("Scroll_History"));
	UOverlaySlot* ScrollSlot = ListOverlay->AddChildToOverlay(Scroll);
	ScrollSlot->SetHorizontalAlignment(HAlign_Fill);
	ScrollSlot->SetVerticalAlignment(VAlign_Fill);

	UTextBlock* Empty = MakeText(TEXT("Text_Empty"), TEXT("아직 전적이 없어요. 한 판 해 보세요!"), TEXT("Bold"), 26, DarkPurple, ETextJustify::Center);
	UOverlaySlot* EmptySlot = ListOverlay->AddChildToOverlay(Empty);
	EmptySlot->SetHorizontalAlignment(HAlign_Center);
	EmptySlot->SetVerticalAlignment(VAlign_Center);

	USizeBox* ListBox = Sized(TEXT("Size_HistoryList"), ListOverlay, 900.f, 460.f);
	UVerticalBoxSlot* ListSlot = Column->AddChildToVerticalBox(ListBox);
	ListSlot->SetHorizontalAlignment(HAlign_Center);

	// 닫기 버튼
	UButton* Close = Make<UButton>(TEXT("Button_Close"));
	FButtonStyle CloseStyle = Close->GetStyle();
	const FSlateBrush CloseBrush = StyleTextureBrush(TEXT("UI_Btn_Pink_L"), FMargin(0.1f, 0.3f, 0.1f, 0.35f), ESlateBrushDrawType::Box);
	CloseStyle.Normal = CloseBrush;
	CloseStyle.Hovered = CloseBrush;
	CloseStyle.Hovered.TintColor = FSlateColor(FLinearColor(0.82f, 0.82f, 0.82f, 1.f));
	CloseStyle.Pressed = CloseBrush;
	CloseStyle.Pressed.TintColor = FSlateColor(FLinearColor(0.66f, 0.66f, 0.66f, 1.f));
	Close->SetStyle(CloseStyle);
	UTextBlock* CloseText = MakeText(TEXT("Label_Button_Close"), TEXT("닫기"), TEXT("Bold"), 24, FLinearColor::White, ETextJustify::Center);
	Close->SetContent(CloseText);
	USizeBox* CloseBox = Sized(TEXT("Size_Button_Close"), Close, 300.f, 80.f);
	UVerticalBoxSlot* CloseSlot = Column->AddChildToVerticalBox(CloseBox);
	CloseSlot->SetHorizontalAlignment(HAlign_Center);
	CloseSlot->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));

	return CloseTree(WidgetBlueprint);
#else
	return 0;
#endif
}

// 로비 가운데 위에 전적 버튼 하나 추가 (이미 있으면 건너뜀)
bool UCTUIBuildTools::AddLobbyHistoryButton(UBlueprint* WidgetBlueprint)
{
#if WITH_EDITOR
	// 주의: OpenTree 는 기존 위젯을 모두 지우므로 쓰지 않고 기존 트리에 그대로 덧붙임
	Tree = WidgetBlueprint ? GetBlueprintTree(WidgetBlueprint) : nullptr;
	if (!Tree) { return false; }
	WidgetCount = 0;
	BlueprintName = TEXT("WBP_CTLobbyWidget");

	if (Tree->FindWidget(TEXT("Button_History")))
	{
		Tree = nullptr;
		return true;
	}
	UCanvasPanel* Canvas = Cast<UCanvasPanel>(Tree->FindWidget(TEXT("Canvas_Lobby")));
	if (!Canvas) { Tree = nullptr; return false; }

	UButton* Button = Make<UButton>(TEXT("Button_History"));
	FButtonStyle Style = Button->GetStyle();
	const FSlateBrush Brush = StyleTextureBrush(TEXT("UI_Btn_Pink_M"), FMargin(0.1f, 0.3f, 0.1f, 0.35f), ESlateBrushDrawType::Box);
	Style.Normal = Brush;
	Style.Hovered = Brush;
	Style.Hovered.TintColor = FSlateColor(FLinearColor(0.82f, 0.82f, 0.82f, 1.f));
	Style.Pressed = Brush;
	Style.Pressed.TintColor = FSlateColor(FLinearColor(0.66f, 0.66f, 0.66f, 1.f));
	Button->SetStyle(Style);
	Button->SetContent(MakeText(TEXT("Label_Button_History"), TEXT("전적"), TEXT("Bold"), 26, FLinearColor::White, ETextJustify::Center));

	UCanvasPanelSlot* ButtonSlot = Cast<UCanvasPanelSlot>(Canvas->AddChild(Button));
	ButtonSlot->SetAnchors(FAnchors(0.5f, 0.f));
	ButtonSlot->SetAlignment(FVector2D(0.5f, 0.f));
	ButtonSlot->SetPosition(FVector2D(0.f, 24.f));
	ButtonSlot->SetSize(FVector2D(240.f, 72.f));
	ButtonSlot->SetZOrder(5);

	return CloseTree(WidgetBlueprint) >= 0;
#else
	return false;
#endif
}

// 로비 플레이어 한 줄 뼈대 (처음 한 번). 만든 뒤의 모양은 WBP 에서 직접 바꿈
int32 UCTUIBuildTools::BuildLobbyPlayerEntryDesign(UBlueprint* WidgetBlueprint)
{
#if WITH_EDITOR
	if (!WidgetBlueprint || !OpenTree(WidgetBlueprint, TEXT("WBP_CTLobbyPlayerEntryWidget"))) { return -1; }

	const FLinearColor Magenta = FLinearColor::FromSRGBColor(FColor(0xFF, 0x27, 0x83));
	const FLinearColor DarkPurple(0.058f, 0.0006f, 0.032f, 1.f);

	// 줄 사이 간격은 바깥 여백으로
	UBorder* Root = Make<UBorder>(TEXT("Border_PlayerRoot"));
	Root->SetBrushColor(FLinearColor::Transparent);
	Root->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Tree->RootWidget = Root;

	UOverlay* Overlay = Make<UOverlay>(TEXT("Overlay_Player"));
	Overlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Root->SetContent(Overlay);

	// 배경: 테두리 없는 분홍 줄
	UImage* Back = Make<UImage>(TEXT("Image_PlayerBack"));
	Back->SetBrush(StyleTextureBrush(TEXT("UI_Row_Flat"), FMargin(0.14f, 0.4f, 0.14f, 0.4f), ESlateBrushDrawType::Box));
	Back->SetColorAndOpacity(FLinearColor::FromSRGBColor(FColor(0xFF, 0xC2, 0xDC)));
	Back->SetVisibility(ESlateVisibility::HitTestInvisible);
	UOverlaySlot* BackSlot = Overlay->AddChildToOverlay(Back);
	BackSlot->SetHorizontalAlignment(HAlign_Fill);
	BackSlot->SetVerticalAlignment(VAlign_Fill);

	// 내 줄 강조 (평소 숨김)
	UImage* Me = Make<UImage>(TEXT("Image_Me"));
	FSlateBrush MeBrush;
	MeBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	MeBrush.TintColor = FSlateColor(FLinearColor(1.f, 0.39f, 0.64f, 0.18f));
	MeBrush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(14.f, 14.f, 14.f, 14.f), FSlateColor(Magenta), 3.f);
	Me->SetBrush(MeBrush);
	Me->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* MeSlot = Overlay->AddChildToOverlay(Me);
	MeSlot->SetHorizontalAlignment(HAlign_Fill);
	MeSlot->SetVerticalAlignment(VAlign_Fill);

	UHorizontalBox* Row = Make<UHorizontalBox>(TEXT("Row_Player"));
	Row->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UOverlaySlot* RowSlot = Overlay->AddChildToOverlay(Row);
	RowSlot->SetHorizontalAlignment(HAlign_Fill);
	RowSlot->SetVerticalAlignment(VAlign_Center);
	RowSlot->SetPadding(FMargin(18.f, 10.f, 18.f, 10.f));

	// 사람 아이콘
	UImage* Icon = Make<UImage>(TEXT("Image_PlayerIcon"));
	if (UTexture2D* PersonTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/2D/UI/ico/Ico_Person.Ico_Person")))
	{
		Icon->SetBrushFromTexture(PersonTexture);
	}
	Icon->SetColorAndOpacity(Magenta);
	Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
	AddH(Row, Sized(TEXT("Size_PlayerIcon"), Icon, 32.f, 32.f), 14.f, false);

	// 이름
	UTextBlock* Name = MakeText(TEXT("Text_Name"), TEXT("플레이어"), TEXT("Bold"), 26, DarkPurple, ETextJustify::Left);
	Name->SetAutoWrapText(false);
	AddH(Row, Name, 12.f, true);

	// 방장 왕관 (방장일 때만 보임)
	UImage* Crown = Make<UImage>(TEXT("Image_Host"));
	if (UTexture2D* CrownTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/2D/UI/ico/ico_crown.ico_crown")))
	{
		Crown->SetBrushFromTexture(CrownTexture);
	}
	Crown->SetVisibility(ESlateVisibility::Collapsed);
	AddH(Row, Sized(TEXT("Size_Host"), Crown, 30.f, 30.f), 12.f, false);

	// 상태 알약 (색은 코드가 상태에 따라 바꿈)
	UOverlay* Pill = Make<UOverlay>(TEXT("Overlay_StatePill"));
	Pill->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UImage* PillBack = Make<UImage>(TEXT("Image_StatePill"));
	FSlateBrush PillBrush;
	PillBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	PillBrush.TintColor = FSlateColor(FLinearColor::White);
	PillBrush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(17.f, 17.f, 17.f, 17.f), FSlateColor(FLinearColor::White), 0.f);
	PillBack->SetBrush(PillBrush);
	PillBack->SetVisibility(ESlateVisibility::HitTestInvisible);
	UOverlaySlot* PillBackSlot = Pill->AddChildToOverlay(PillBack);
	PillBackSlot->SetHorizontalAlignment(HAlign_Fill);
	PillBackSlot->SetVerticalAlignment(VAlign_Fill);
	UTextBlock* State = MakeText(TEXT("Text_State"), TEXT("대기"), TEXT("Bold"), 19, FLinearColor::White, ETextJustify::Center);
	State->SetAutoWrapText(false);
	UOverlaySlot* StateSlot = Pill->AddChildToOverlay(State);
	StateSlot->SetHorizontalAlignment(HAlign_Center);
	StateSlot->SetVerticalAlignment(VAlign_Center);
	AddH(Row, Sized(TEXT("Size_StatePill"), Pill, 92.f, 34.f), 0.f, false);

	return CloseTree(WidgetBlueprint);
#else
	return 0;
#endif
}

// 로비 플레이어 패널 머리에 "현재 / 최대" 인원 글자 추가 (기존 위젯은 건드리지 않고 덧붙임)
bool UCTUIBuildTools::AddLobbyPlayerCountText(UBlueprint* WidgetBlueprint)
{
#if WITH_EDITOR
	Tree = WidgetBlueprint ? GetBlueprintTree(WidgetBlueprint) : nullptr;
	if (!Tree) { return false; }
	WidgetCount = 0;
	BlueprintName = TEXT("WBP_CTLobbyWidget");

	if (Tree->FindWidget(TEXT("Text_PlayerCount"))) { Tree = nullptr; return true; }
	UCanvasPanel* Canvas = Cast<UCanvasPanel>(Tree->FindWidget(TEXT("Canvas_CompactPlayers")));
	UTextBlock* Label = Cast<UTextBlock>(Tree->FindWidget(TEXT("Label_LobbyPlayers")));
	if (!Canvas || !Label) { Tree = nullptr; return false; }

	// 제목 글자와 같은 글꼴과 색을 써서 어울리게
	UTextBlock* Count = Make<UTextBlock>(TEXT("Text_PlayerCount"));
	Count->SetText(Loc(TEXT("Text_PlayerCount"), TEXT("0 / 8")));
	Count->SetFont(Label->GetFont());
	Count->SetColorAndOpacity(Label->GetColorAndOpacity());
	Count->SetJustification(ETextJustify::Right);
	Count->SetVisibility(ESlateVisibility::HitTestInvisible);

	UCanvasPanelSlot* CountSlot = Cast<UCanvasPanelSlot>(Canvas->AddChild(Count));
	CountSlot->SetAnchors(FAnchors(0.f, 0.f));
	CountSlot->SetAlignment(FVector2D(0.f, 0.f));
	CountSlot->SetPosition(FVector2D(440.f, 2.f));
	CountSlot->SetSize(FVector2D(200.f, 44.f));
	CountSlot->SetZOrder(5);

	return CloseTree(WidgetBlueprint) >= 0;
#else
	return false;
#endif
}
