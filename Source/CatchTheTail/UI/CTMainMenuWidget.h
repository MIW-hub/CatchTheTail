// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CTMainMenuWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;
class UCTSettingsWidget;
class UWidget;

// 메뉴 패널
UENUM(BlueprintType)
enum class ECTMenuPanel : uint8
{
	Main		UMETA(DisplayName = "Main"),
	Lobby		UMETA(DisplayName = "Lobby"),
	Settings	UMETA(DisplayName = "Settings"),
	HowTo		UMETA(DisplayName = "HowTo"),
	QuitConfirm	UMETA(DisplayName = "QuitConfirm"),
};

UCLASS(Abstract)
class CATCHTHETAIL_API UCTMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 패널 전환
	UFUNCTION(BlueprintCallable, Category = "CT|Menu")
	void ShowPanel(ECTMenuPanel Panel);

	UFUNCTION(BlueprintPure, Category = "CT|Menu")
	ECTMenuPanel GetCurrentPanel() const { return CurrentPanel; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 안내 문구 표시
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Menu")
	void BP_OnMessageShown(const FText& Message);

	// 패널 전환 연출
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Menu")
	void BP_OnPanelChanged(ECTMenuPanel NewPanel);

	// 설정 패널에 넣을 설정 화면
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Menu")
	TSubclassOf<UCTSettingsWidget> SettingsWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "CT|Menu")
	TObjectPtr<UCTSettingsWidget> SettingsWidget;

	// 패널
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Panel_Main;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Panel_Lobby;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Panel_Settings;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Panel_HowTo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Panel_QuitConfirm;

	// 메인 패널
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Start;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_FindLobby;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Settings;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_HowTo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Quit;

	// 방 코드 입장
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_RoomCode;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_JoinByCode;

	// 로비 패널
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Host;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_LobbyBack;

	// 설정 / 게임 방법 / 종료 확인
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_SettingsBack;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_HowToBack;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_QuitCancel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_QuitConfirm;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Status;

	// 저장된 이름
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_PlayerName;

private:
	// Panel_Settings에 설정 화면 생성
	void CreateSettingsWidget();

	// 온라인 불가 시 버튼 막고 사유 툴팁
	void ApplyOnlineAvailability();

	UFUNCTION()
	void HandleJoinByCodeClicked();

	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleFindLobbyClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleHowToClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleHostClicked();

	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleQuitConfirmClicked();

	UFUNCTION()
	void HandleMessage(FText Message);

	// 이름 표시 갱신
	UFUNCTION()
	void HandlePlayerNameChanged();

	void RefreshPlayerName();

	void ShowMessage(const FText& Message);

	// 패널 켜고 끄기
	static void SetPanelVisible(UWidget* Panel, bool bVisible);

	ECTMenuPanel CurrentPanel = ECTMenuPanel::Main;
};
