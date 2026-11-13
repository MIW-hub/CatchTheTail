// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CTUIBuildTools.generated.h"

class UBlueprint;
class UMaterialInterface;
class UTexture2D;

// 위젯 트리 구성 에디터 도구
UCLASS()
class CATCHTHETAIL_API UCTUIBuildTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 설정 화면 위젯 트리 재구성
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 BuildSettingsWidgetDesign(UBlueprint* WidgetBlueprint);

	// 언어 선택 화면 위젯 트리 구성
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 BuildLanguageSelectDesign(UBlueprint* WidgetBlueprint);

	// 설정 화면 다듬기
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool RefineSettingsWidget(UBlueprint* WidgetBlueprint);

	// 이름 변경 영역 교체
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool RefineNameChange(UBlueprint* WidgetBlueprint);

	// 로비 맵 설정 창 구성
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool BuildMatchSettingsPanel(UBlueprint* WidgetBlueprint);

	// 로딩 화면(WBP_CTLoadingWidget) 위젯 트리 구성: 배경, 참가 인원, 로딩 문구, 진행 막대, 게임 설명
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 BuildLoadingScreen(UBlueprint* WidgetBlueprint, UTexture2D* BackgroundTexture);

	// ESC 메뉴(WBP_CTPauseMenuWidget) 위젯 트리 구성: 메인(계속하기/설정/기권/게임 종료), 설정 자리, 확인 팝업
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 BuildPauseMenu(UBlueprint* WidgetBlueprint);

	// 이모트 선택창 WBP 처음 뼈대 만들기 (한 번만. 이후 모양은 WBP 에서 직접 꾸밈)
	// 칸(WBP_CTEmoteCellWidget): 배경, 이모지 그림, 선택 표시, 키 번호, 버튼
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 BuildEmoteCellDesign(UBlueprint* WidgetBlueprint);

	// 창(WBP_CTEmoteSelectWidget): 어두운 배경, 틀, 제목, 숫자 키 자리 줄, 전체 목록 격자, 닫기 버튼
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 BuildEmoteSelectDesign(UBlueprint* WidgetBlueprint);

	// 위젯 트리를 글자로 설명 (이름, 종류, 캔버스 위치)
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static FString DescribeWidgetTree(UBlueprint* WidgetBlueprint);

	// 점수판 한 줄(WBP_CTScoreEntryWidget)에 1등 왕관 이미지(Image_Crown) 추가
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool AddScoreEntryCrown(UBlueprint* WidgetBlueprint, UTexture2D* CrownTexture);

	// HUD 스킬 칸 정리
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 BuildHudSkillSlots(UBlueprint* WidgetBlueprint, UMaterialInterface* CooldownMaterial);

	// HUD(WBP_CTHUDWidget) 전체 재구성: 타겟/시간/순위표 카드, 내 꼬리, 키 안내
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 BuildHudLayout(UBlueprint* WidgetBlueprint, UMaterialInterface* CooldownMaterial);

	// 설정 화면(WBP_CTSettingsWidget)에 음성 채팅 항목(마이크 사용, 음성 볼륨, 마이크 감도)을 소리 항목 아래에 추가. 이미 있으면 0. 기존 위젯은 건드리지 않음
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 AddVoiceSettings(UBlueprint* WidgetBlueprint);

	// 로비 맵 설정 창의 경기 설정 칸에 항목 한 줄([이름 < 값 >])을 맨 아래에 추가. 이미 있으면 0. 기존 위젯은 건드리지 않음
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 AddMatchSettingRow(UBlueprint* WidgetBlueprint, int32 SettingIndex);

	// 캔버스 위에 놓인 위젯의 앵커, 정렬, 위치와 크기(앵커가 늘어나는 형태면 오른쪽, 아래 여백)를 정함
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool SetCanvasSlotPlacement(UBlueprint* WidgetBlueprint, FName WidgetName, FVector2D AnchorMin, FVector2D AnchorMax, FVector2D Alignment, FVector2D Position, FVector2D SizeOrRightBottom);

	// 크기 칸(SizeBox)의 가로/세로 지정값을 바꿈. 0 이하는 해제
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool SetSizeBoxOverride(UBlueprint* WidgetBlueprint, FName WidgetName, float Width, float Height);

	// 위젯 기본 표시 상태를 바꿈 (ESlateVisibility 번호: 0 보임, 1 접힘, 2 숨김, 3 클릭 통과, 4 자신만 통과)
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool SetWidgetVisibilityByName(UBlueprint* WidgetBlueprint, FName WidgetName, int32 Visibility);

	// HUD 키 칸(Overlay)의 글자(TextName)를 숨기고 그 위에 아이콘 이미지(IconName)를 올림. 이미 있으면 이미지만 교체
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool SetHudKeyIcon(UBlueprint* WidgetBlueprint, FName OverlayName, FName TextName, FName IconName, UTexture2D* IconTexture, float IconSize);

	// 설정 화면에 "버튼 없이 말하기" 체크박스(CheckBox_MicAlwaysOn)를 마이크 사용 아래에 추가. 이미 있으면 0. 기존 위젯은 건드리지 않음
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 AddMicAlwaysOnSetting(UBlueprint* WidgetBlueprint);

	// HUD(WBP_CTHUDWidget) 오른쪽에 마이크 상태 아이콘(32x32, Image_MicIcon)과 아래 키 이름(Text_MicKey)을 추가. 이미 있으면 0. 기존 위젯은 건드리지 않음
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 AddHudMicIndicator(UBlueprint* WidgetBlueprint);

	// 캔버스 안에 이미지를 만들거나(없으면) 갱신(있으면). DrawAs 0=Image 1=Box(9-slice). Margin은 텍스처 대비 비율(L,T,R,B)
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool PlaceCanvasImage(UBlueprint* WidgetBlueprint, FName ParentName, FName ImageName, UTexture2D* Texture, FVector2D AnchorMin, FVector2D AnchorMax, FVector2D Alignment, FVector2D Position, FVector2D SizeOrRightBottom, int32 ZOrder, int32 DrawAs, FVector4 Margin, FLinearColor Tint);

	// 버튼 모양을 이미지로 바꿈. 올림/눌림은 같은 이미지에 색만 어둡게(HoverFactor, PressFactor)
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool SetButtonImage(UBlueprint* WidgetBlueprint, FName ButtonName, UTexture2D* Texture, int32 DrawAs, FVector4 Margin, float HoverFactor, float PressFactor);

	// 글자 색, 크기(0이면 그대로), 외곽선 설정
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool SetTextAppearance(UBlueprint* WidgetBlueprint, FName TextName, FLinearColor Color, int32 FontSize, FLinearColor OutlineColor, int32 OutlineSize);

	// Border 배경을 이미지로 바꾸고 안쪽 여백(L,T,R,B)을 정함
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool SetBorderImage(UBlueprint* WidgetBlueprint, FName BorderName, UTexture2D* Texture, int32 DrawAs, FVector4 Margin, FVector4 Padding);

	// 위젯(글자 등)을 이미지 배경의 Border로 감쌈. 이름은 Pill_<원래 이름>. 이미 감쌌으면 배경만 갱신
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool WrapInBorder(UBlueprint* WidgetBlueprint, FName WidgetName, UTexture2D* Texture, int32 DrawAs, FVector4 Margin, FVector4 Padding);

	// 슬라이더를 둥근 막대 + 둥근 손잡이(외곽선)로 바꿈
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool StyleSlider(UBlueprint* WidgetBlueprint, FName SliderName, FLinearColor BarColor, FLinearColor HandleColor, FLinearColor OutlineColor);

	// 체크박스를 둥근 흰 상자 / 체크 시 색 상자(외곽선)로 바꿈
	// 전적 팝업 줄 / 팝업 뼈대 (처음 한 번)
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 BuildMatchHistoryEntryDesign(UBlueprint* WidgetBlueprint);

	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 BuildMatchHistoryDesign(UBlueprint* WidgetBlueprint);

	// 로비 플레이어 한 줄 뼈대 (처음 한 번)
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 BuildLobbyPlayerEntryDesign(UBlueprint* WidgetBlueprint);

	// 로비 플레이어 패널에 "현재 / 최대" 인원 글자 추가
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool AddLobbyPlayerCountText(UBlueprint* WidgetBlueprint);

	// 로비 위젯에 전적 버튼 추가
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool AddLobbyHistoryButton(UBlueprint* WidgetBlueprint);

	// 입력창 배경/글자 색 (어두운 배경 + 밝은 글자)
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool StyleEditableTextBox(UBlueprint* WidgetBlueprint, FName BoxName, FLinearColor Fill, FLinearColor FocusedFill, FLinearColor OutlineColor, FLinearColor TextColor, float Radius);

	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool StyleCheckBox(UBlueprint* WidgetBlueprint, FName CheckBoxName, FLinearColor UncheckedFill, FLinearColor CheckedFill, FLinearColor OutlineColor, float BoxSize);

	// 이미지를 둥근 상자(채움 + 외곽선)로 바꿈. 선택 표시 테두리 등에 씀
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool SetImageRoundedBox(UBlueprint* WidgetBlueprint, FName ImageName, FLinearColor Fill, FLinearColor Outline, float Radius, float OutlineWidth);

	// 진행 막대를 둥근 배경(외곽선) + 둥근 채움으로 바꿈
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool StyleProgressBar(UBlueprint* WidgetBlueprint, FName BarName, FLinearColor Background, FLinearColor Fill, FLinearColor Outline);

	// 새 이미지(Style, ico)나 둥근 상자를 쓰는 이미지, 버튼, Border에 남은 예전 색 배율을 흰색으로 되돌림. 고친 개수
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 FixImageTints(UBlueprint* WidgetBlueprint);

	// 버튼 안 글자(내용)를 가운데로 두고 아래 여백을 줌. 이미지 아래 그림자만큼 올려서 얼굴 가운데에 맞춤
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool SetButtonLabelPadding(UBlueprint* WidgetBlueprint, FName ButtonName, float BottomPadding);

	// 버튼 안 글자 앞에 아이콘을 넣어 [아이콘 글자]를 한 묶음으로 가운데 정렬 (버튼과 함께 움직이고 커짐). 이미 넣었으면 아이콘만 갱신
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool AddButtonIcon(UBlueprint* WidgetBlueprint, FName ButtonName, UTexture2D* Icon, float IconSize, float Gap);

	// 위젯을 지움 (GUID 표도 정리)
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool RemoveWidgetByName(UBlueprint* WidgetBlueprint, FName WidgetName);

	// 글자 크기를 Delta만큼 키우거나 줄임 (최소 8). 색과 외곽선은 그대로
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool AdjustFontSize(UBlueprint* WidgetBlueprint, FName TextName, int32 Delta);

	// 이미지 위젯의 색 배율(Color and Opacity)을 정함. 흰색이면 원본 색 그대로
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool SetImageColorByName(UBlueprint* WidgetBlueprint, FName ImageName, FLinearColor Color);

	// 캔버스 슬롯의 겹침 순서(큰 값이 위)와 글자 정렬(0 왼쪽, 1 가운데, 2 오른쪽)
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool SetCanvasSlotZOrder(UBlueprint* WidgetBlueprint, FName WidgetName, int32 ZOrder);

	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool SetTextJustificationByName(UBlueprint* WidgetBlueprint, FName TextName, int32 Justification);

	// 위젯 변수 이름-GUID 표를 위젯 트리에 맞춤 (없는 것은 등록, 지워진 위젯 것은 제거). 바뀐 개수를 돌려줌
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 SyncWidgetGuids(UBlueprint* WidgetBlueprint);

	// 번역 키 없는 한글 글자(TextBlock)에 키를 붙임. 바꾼 개수를 돌려줌
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 KeyUnlocalizedTexts(UBlueprint* WidgetBlueprint);

	// HUD 순위표 한 줄(흰 카드 안에 들어가는 작은 줄) 구성
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 BuildHudScoreEntry(UBlueprint* WidgetBlueprint);
};
