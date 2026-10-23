// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "Framework/Commands/InputChord.h"
#include "CTKeyBindEntryWidget.generated.h"

class UInputKeySelector;
class UTextBlock;
struct FCTControlRow;

// 새 키 선택
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCTKeyReboundSignature, FName, ActionId, FKey, NewKey);

UCLASS(Abstract)
class CATCHTHETAIL_API UCTKeyBindEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 한 줄 채우기
	void Setup(const FCTControlRow& Row);

	// 선택기 키 되돌리기
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void ShowKey(FKey Key);

	UPROPERTY(BlueprintAssignable, Category = "CT|Settings")
	FCTKeyReboundSignature OnKeyRebound;

	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	FName GetActionId() const { return ActionId; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ActionName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UInputKeySelector> KeySelector;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Keys;

private:
	UFUNCTION()
	void HandleKeySelected(FInputChord SelectedKey);

	FName ActionId = NAME_None;
	bool bRebindable = false;
	FKey CurrentKey;
};
