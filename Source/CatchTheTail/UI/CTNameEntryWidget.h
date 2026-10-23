// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CTNameEntryWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;

// 이름 저장됨
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCTNameConfirmedSignature);

UCLASS(Abstract)
class CATCHTHETAIL_API UCTNameEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 저장 시 호출
	UPROPERTY(BlueprintAssignable, Category = "CT|Name")
	FCTNameConfirmedSignature OnNameConfirmed;

	// 이름 검사 후 저장
	UFUNCTION(BlueprintCallable, Category = "CT|Name")
	void ConfirmName();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 규칙 위반 (연출용)
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Name")
	void BP_OnNameError(const FText& Reason);

	// 저장 완료 (연출용)
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
