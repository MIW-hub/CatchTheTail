// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CTPauseMenuWidget.generated.h"

class UButton;
class UCTSettingsWidget;
class UTextBlock;
class UWidget;

// 메뉴를 닫아 달라는 요청 (계속하기, 기권 확정 등)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCTPauseCloseSignature);

/**
 * 경기 중 ESC로 여는 메뉴. 게임은 멈추지 않습니다.
 * 계속하기 / 설정(소리, 화면, 조작 키) / 기권 / 게임 종료가 있고,
 * 기권과 게임 종료는 불이익을 알리는 확인 창을 한 번 거칩니다.
 * 기권은 경기 시간의 절반이 지난 뒤부터 누를 수 있습니다.
 */
UCLASS(Abstract)
class CATCHTHETAIL_API UCTPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UCTPauseMenuWidget(const FObjectInitializer& ObjectInitializer);

	// 닫기 요청 시 호출
	UPROPERTY(BlueprintAssignable, Category = "CT|Pause")
	FCTPauseCloseSignature OnCloseRequested;

	// 메뉴를 열 때 첫 화면으로 되돌림
	UFUNCTION(BlueprintCallable, Category = "CT|Pause")
	void ResetToMain();

	// ESC로 한 단계 뒤로. 설정 화면이나 확인 창을 닫았으면 true, 더 닫을 게 없으면 false
	UFUNCTION(BlueprintCallable, Category = "CT|Pause")
	bool HandleBack();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 설정 화면 (기본값 WBP_CTSettingsWidget). 소리, 화면, 조작 키가 들어 있음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Pause")
	TSubclassOf<UCTSettingsWidget> SettingsWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "CT|Pause")
	TObjectPtr<UCTSettingsWidget> SettingsWidget;

	// 패널
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Panel_Main;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Panel_Settings;

	// 확인 창 (메인 위에 뜨는 팝업)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Panel_Confirm;

	// 메인 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Resume;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Settings;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Forfeit;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Quit;

	// 기권 버튼 아래 안내 (언제부터 가능한지)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ForfeitInfo;

	// 확인 창
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ConfirmTitle;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ConfirmBody;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_ConfirmYes;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_ConfirmNo;

private:
	enum class EPendingAction : uint8 { None, Forfeit, Quit };

	enum class EPanel : uint8 { Main, Settings };

	void CreateSettingsWidget();
	void ShowPanel(EPanel Panel);
	void OpenConfirm(EPendingAction Action);
	void CloseConfirm();

	// 기권 버튼 활성/안내 문구 갱신
	void RefreshForfeit();

	UFUNCTION()
	void HandleResumeClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleSettingsClosed();

	UFUNCTION()
	void HandleForfeitClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleConfirmYesClicked();

	UFUNCTION()
	void HandleConfirmNoClicked();

	EPendingAction PendingAction = EPendingAction::None;
	EPanel CurrentPanel = EPanel::Main;

	// 안내 문구를 매 프레임이 아니라 이 간격으로 갱신
	float RefreshTimer = 0.f;
};
