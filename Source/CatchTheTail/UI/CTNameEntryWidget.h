// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CTNameEntryWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;

/** 이름이 검사를 통과해서 저장되었을 때 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCTNameConfirmedSignature);

/**
 * 이름 입력 화면. (WBP_NameEntry의 부모. 메인 메뉴 전에 ACTMenuPlayerController가 띄웁니다)
 *
 * 흐름: 인트로 → (저장된 이름이 없으면) 이름 입력 → 메인 메뉴
 *  - 이름은 처음 한 번만 정할 수 있습니다. 저장하면 다음 실행부터는 이 화면을 건너뛰고, 바꿀 수 없습니다.
 *  - 저장한 이름은 방을 만들거나 참가할 때 서버에 전달되어 점수판과 결과 화면에 표시됩니다.
 *
 * WBP에 만들 수 있는 위젯 (이름을 똑같이, 모두 선택 사항)
 *  - EditableTextBox_Name : 이름 입력칸 (Enter를 눌러도 확인됨)
 *  - Button_Confirm       : 확인 버튼
 *  - Text_Error           : 오류 문구 (평소에는 숨김)
 *  - Text_Hint            : 안내 문구 (예: "2~12자, 한글/영문/숫자/_/-")
 *  - 이름은 한 번만 정할 수 있으니, 확인 전에 "이름은 나중에 바꿀 수 없습니다" 같은 안내를 WBP에 적어 두세요.
 *
 * 이름 규칙은 UCTGameInstance가 검사합니다. 이 위젯은 입력을 전달하고 결과를 보여 줍니다.
 */
UCLASS(Abstract)
class CATCHTHETAIL_API UCTNameEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 이름이 저장되면 호출됩니다. */
	UPROPERTY(BlueprintAssignable, Category = "CT|Name")
	FCTNameConfirmedSignature OnNameConfirmed;

	/** 입력칸의 이름을 검사하고 저장합니다. (확인 버튼, Enter가 호출) */
	UFUNCTION(BlueprintCallable, Category = "CT|Name")
	void ConfirmName();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 이름이 규칙에 맞지 않을 때 (흔들림 연출, 사운드용) */
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Name")
	void BP_OnNameError(const FText& Reason);

	/** 이름이 저장되었을 때 (연출용) */
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Name")
	void BP_OnNameSaved(const FString& SavedName);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_Name;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Confirm;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Error;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Hint;

private:
	UFUNCTION()
	void HandleConfirmClicked();

	UFUNCTION()
	void HandleTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandleTextChanged(const FText& Text);

	void ShowError(const FText& Reason);
	void ClearError();
};
