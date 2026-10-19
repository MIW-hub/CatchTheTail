// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CTMainMenuWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;
class UWidget;

/** 메인 메뉴가 보여 주는 화면(패널) */
UENUM(BlueprintType)
enum class ECTMenuPanel : uint8
{
	Main		UMETA(DisplayName = "Main"),
	Lobby		UMETA(DisplayName = "Lobby"),
	Settings	UMETA(DisplayName = "Settings"),
	HowTo		UMETA(DisplayName = "HowTo"),
	QuitConfirm	UMETA(DisplayName = "QuitConfirm"),
};

/**
 * 메인 메뉴 화면. (WBP_MainMenu의 부모. L_MainMenu 맵에서 ACTMenuPlayerController가 띄웁니다)
 *
 * 화면 구성 (기획 시안: 메인 / 로비 / 설정 / 게임 방법 / 종료 확인)
 *  - 아래 이름의 패널 위젯 중 만든 것만 쓰입니다. 한 번에 하나만 보이고, 메인 패널이 처음에 보입니다.
 *    Panel_Main, Panel_Lobby, Panel_Settings, Panel_HowTo, Panel_QuitConfirm
 *  - 패널이 없는 항목은 건너뜁니다. (예: Panel_QuitConfirm이 없으면 종료 버튼이 바로 종료)
 *  - 종료 확인(Panel_QuitConfirm)은 메인 화면 위에 겹쳐 뜨는 팝업입니다.
 *
 * WBP에 만들 수 있는 버튼 (이름을 똑같이, 모두 선택 사항)
 *  메인 패널
 *   - Button_Start      : 게임 시작 (방을 만들고 바로 시작)
 *   - Button_FindLobby  : 로비 찾기 (로비 패널로 이동)
 *   - Button_Settings   : 설정
 *   - Button_HowTo      : 게임 방법
 *   - Button_Quit       : 게임 종료 (종료 확인 팝업을 띄움)
 *  로비 패널
 *   - Button_Host              : 방 만들기
 *   - Button_Join              : 방 참가
 *   - EditableTextBox_Address  : 참가할 방장의 주소 (예: 192.168.0.10 또는 192.168.0.10:7777)
 *   - Button_LobbyBack         : 뒤로가기
 *  설정 / 게임 방법 패널
 *   - Button_SettingsBack, Button_HowToBack : 뒤로가기
 *  종료 확인 팝업
 *   - Button_QuitCancel  : 취소
 *   - Button_QuitConfirm : 게임 종료
 *  기타
 *   - Text_Status : 접속 실패, 연결 끊김 등의 안내 문구
 *
 * 방 만들기/참가/종료는 UCTGameInstance가 처리합니다. 이 위젯은 연결과 화면 전환만 합니다.
 */
UCLASS(Abstract)
class CATCHTHETAIL_API UCTMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 보여 줄 패널을 바꿉니다. (만들지 않은 패널은 무시) */
	UFUNCTION(BlueprintCallable, Category = "CT|Menu")
	void ShowPanel(ECTMenuPanel Panel);

	UFUNCTION(BlueprintPure, Category = "CT|Menu")
	ECTMenuPanel GetCurrentPanel() const { return CurrentPanel; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 안내 문구가 표시될 때 호출 (연출, 사운드용) */
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Menu")
	void BP_OnMessageShown(const FText& Message);

	/** 패널이 바뀔 때 호출 (전환 애니메이션용) */
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Menu")
	void BP_OnPanelChanged(ECTMenuPanel NewPanel);

	// ----- 패널 -----
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

	// ----- 메인 패널 -----
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

	// ----- 로비 패널 -----
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Host;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Join;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_Address;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_LobbyBack;

	// ----- 설정 / 게임 방법 / 종료 확인 -----
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

private:
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
	void HandleJoinClicked();

	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleQuitConfirmClicked();

	UFUNCTION()
	void HandleMessage(FText Message);

	void ShowMessage(const FText& Message);

	/** 패널 하나를 켜고 끕니다. (nullptr이면 무시) */
	static void SetPanelVisible(UWidget* Panel, bool bVisible);

	ECTMenuPanel CurrentPanel = ECTMenuPanel::Main;
};
