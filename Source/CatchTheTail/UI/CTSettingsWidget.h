// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "Types/SlateEnums.h"
#include "CTSettingsWidget.generated.h"

class UButton;
class UCheckBox;
class UCTKeyBindEntryWidget;
class UCTSettingsSubsystem;
class UEditableTextBox;
class USlider;
class UTextBlock;
class UVerticalBox;
class UComboBoxString;
class UWidget;

// 닫기 버튼
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCTSettingsCloseSignature);

UCLASS(Abstract)
class CATCHTHETAIL_API UCTSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 닫기 시 호출
	UPROPERTY(BlueprintAssignable, Category = "CT|Settings")
	FCTSettingsCloseSignature OnCloseRequested;

	// 저장값으로 전체 채우기
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void RefreshAll();

	// 전체 기본값 복원
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void ResetSettings();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 안내 문구 표시
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Settings")
	void BP_OnMessage(const FText& Message);

	// RefreshAll 후
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Settings")
	void BP_OnRefreshed();

	// 조작 한 줄 위젯 (UCTKeyBindEntryWidget 부모)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Settings")
	TSubclassOf<UCTKeyBindEntryWidget> KeyBindEntryClass;

	// 언어 선택 (한국어 / English / 日本語)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> ComboBox_Language;

	// 소리
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

	// 조작
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VerticalBox_Controls;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ControlsMessage;

	// 화면
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_Fullscreen;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_ScreenShake;

	// 접근성
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_TeamColors;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_TeamMarker;

	// 개인 설정
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

	// 기타
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
	void HandleLanguageChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	// 언어 목록을 채우고 현재 언어를 선택
	void RefreshLanguage();

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

	// 채우는 중 변경 알림 무시
	bool bRefreshing = false;
};
