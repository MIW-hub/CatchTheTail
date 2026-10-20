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

/** 사용자가 새 키를 골랐을 때 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCTKeyReboundSignature, FName, ActionId, FKey, NewKey);

/**
 * 설정 창의 "조작 방법" 한 줄. (WBP_KeyBindEntry의 부모)
 *
 * WBP에 만들 수 있는 위젯 (이름을 똑같이, 모두 선택 사항)
 *  - Text_ActionName : 행동 이름 (이동, 점프, 대시 ...)
 *  - KeySelector     : Input Key Selector 위젯. 키를 바꿀 수 있는 행동에서 보입니다. (눌러서 새 키를 입력)
 *  - Text_Keys       : 키 안내 글자. 키를 바꿀 수 없는 행동(이동 등)에서 W, A, S, D처럼 보여 줍니다.
 */
UCLASS(Abstract)
class CATCHTHETAIL_API UCTKeyBindEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 한 줄의 내용을 채웁니다. (설정 창이 호출) */
	void Setup(const FCTControlRow& Row);

	/** 키 선택기를 지정한 키로 되돌립니다. (키 변경이 거절됐을 때) */
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
