// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "CTSettingsWidget.generated.h"

class UButton;
class UCheckBox;
class UCTKeyBindEntryWidget;
class UCTSettingsSubsystem;
class UEditableTextBox;
class USlider;
class UTextBlock;
class UVerticalBox;
class UWidget;

/** "닫기" 버튼을 눌렀을 때 (메뉴로 돌아가거나 창을 닫는 쪽에서 받음) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCTSettingsCloseSignature);

/**
 * 설정 창. (WBP_Settings의 부모)
 *
 * 구성 (WBP에서 이름을 똑같이 만들면 자동 연결됩니다. 모두 선택 사항)
 *
 *  [소리]   Slider_Music, Slider_Sfx (0~1), Text_MusicValue, Text_SfxValue (퍼센트 글자), CheckBox_Mute
 *  [조작]   VerticalBox_Controls : 줄이 들어갈 곳 (+ 아래 KeyBindEntryClass 지정)
 *           Text_ControlsMessage : 키 변경이 거절된 이유 (이미 쓰는 키 등)
 *  [화면]   CheckBox_Fullscreen (전체 화면), CheckBox_ScreenShake (화면 흔들림)
 *  [접근성] CheckBox_TeamColors (색 구분 보조), CheckBox_TeamMarker (캐릭터 위 팀 표시)
 *  [개인]   Text_PlayerName : 내 이름
 *           Panel_ChangeName : 이름 변경 영역 (프로젝트 설정의 Allow Name Change가 꺼져 있으면 숨김)
 *           EditableTextBox_NewName, Button_ChangeName, Text_NameMessage
 *  [기타]   Button_Reset (설정 초기화), Button_Close (닫기), Text_Message (안내 문구)
 *
 * 값을 바꾸면 바로 적용되고 저장됩니다. (UCTSettingsSubsystem)
 * 사용 방법: 메인 메뉴의 Panel_Settings 안에 이 WBP를 놓고, OnCloseRequested에서 메인 패널로 돌아가게 하면 됩니다.
 */
UCLASS(Abstract)
class CATCHTHETAIL_API UCTSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 닫기 버튼을 누르면 호출됩니다. */
	UPROPERTY(BlueprintAssignable, Category = "CT|Settings")
	FCTSettingsCloseSignature OnCloseRequested;

	/** 저장된 값으로 화면을 모두 다시 채웁니다. */
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void RefreshAll();

	/**
	 * 모든 설정을 기본값으로 되돌립니다.
	 * Button_Reset을 누르면 바로 실행됩니다. 확인 창을 넣고 싶으면 Button_Reset은 만들지 말고,
	 * 확인 창에서 이 함수를 직접 부르세요.
	 */
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void ResetSettings();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 안내 문구가 표시될 때 (연출용) */
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Settings")
	void BP_OnMessage(const FText& Message);

	/** RefreshAll이 끝난 뒤 (추가 표시용) */
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Settings")
	void BP_OnRefreshed();

	/** 조작 방법 한 줄로 쓸 위젯 (WBP_KeyBindEntry, UCTKeyBindEntryWidget을 부모로 만드세요) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Settings")
	TSubclassOf<UCTKeyBindEntryWidget> KeyBindEntryClass;

	// ----- 소리 -----
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> Slider_Music;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> Slider_Sfx;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_MusicValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SfxValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_Mute;

	// ----- 조작 -----
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VerticalBox_Controls;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ControlsMessage;

	// ----- 화면 -----
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_Fullscreen;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_ScreenShake;

	// ----- 접근성 -----
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_TeamColors;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_TeamMarker;

	// ----- 개인 설정 -----
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_PlayerName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Panel_ChangeName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_NewName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_ChangeName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_NameMessage;

	// ----- 기타 -----
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Reset;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Close;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Message;

private:
	UCTSettingsSubsystem* GetSettings() const;

	void BuildControlRows();
	void ShowMessage(const FText& Message);
	void ShowNameMessage(const FText& Message);
	static void SetPercentText(UTextBlock* Text, float Value);

	UFUNCTION()
	void HandleMusicChanged(float Value);

	UFUNCTION()
	void HandleSfxChanged(float Value);

	UFUNCTION()
	void HandleMuteChanged(bool bIsChecked);

	UFUNCTION()
	void HandleFullscreenChanged(bool bIsChecked);

	UFUNCTION()
	void HandleScreenShakeChanged(bool bIsChecked);

	UFUNCTION()
	void HandleTeamColorsChanged(bool bIsChecked);

	UFUNCTION()
	void HandleTeamMarkerChanged(bool bIsChecked);

	UFUNCTION()
	void HandleKeyRebound(FName ActionId, FKey NewKey);

	UFUNCTION()
	void HandleChangeNameClicked();

	UFUNCTION()
	void HandleResetClicked();

	UFUNCTION()
	void HandleCloseClicked();

	/** RefreshAll이 위젯 값을 채우는 중에는 변경 알림을 무시합니다. */
	bool bRefreshing = false;
};
