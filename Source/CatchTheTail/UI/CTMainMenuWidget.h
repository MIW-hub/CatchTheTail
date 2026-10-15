// Copyright Moon Party Games. All Rights Reserved.
// [보류 중] 켜는 방법은 CTGameInstance.h 맨 위 설명을 보세요.
#if 0
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CTMainMenuWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;

/**
 * 메인 메뉴 화면. (WBP_MainMenu의 부모. L_MainMenu 맵에서 ACTMenuPlayerController가 띄웁니다)
 *
 * WBP에 만들 수 있는 위젯 (이름을 똑같이, 모두 선택 사항)
 *  - Button_Host            : 방 만들기
 *  - Button_Join            : 방 참가
 *  - EditableTextBox_Address: 참가할 방장의 주소 (예: 192.168.0.10 또는 192.168.0.10:7777)
 *  - Button_Quit            : 게임 종료
 *  - Text_Status            : 접속 실패, 연결 끊김 등의 안내 문구
 *
 * 버튼 동작은 UCTGameInstance가 처리합니다. 이 위젯은 연결만 합니다.
 */
UCLASS(Abstract)
class CATCHTHETAIL_API UCTMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 안내 문구가 표시될 때 호출 (연출, 사운드용) */
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Menu")
	void BP_OnMessageShown(const FText& Message);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Host;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Join;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_Address;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Quit;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Status;

private:
	UFUNCTION()
	void HandleHostClicked();

	UFUNCTION()
	void HandleJoinClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleMessage(FText Message);

	void ShowMessage(const FText& Message);
};
#endif // 보류: 메인 메뉴 / 방 만들기
