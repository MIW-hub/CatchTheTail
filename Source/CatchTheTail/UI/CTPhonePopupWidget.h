// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CTUserWidget.h"
#include "CTPhonePopupWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCTPhoneQuerySelectedSignature, ECTPhoneQuery, Query);

/**
 * 전화부스 팝업. 팝업 어디서든 마우스 좌클릭은 목표, 우클릭은 헌터를 알아봄 (입력 자체로 선택).
 * 버튼 위젯은 안내용 모양일 뿐이고 클릭을 받지 않음.
 * WBP를 따로 만들지 않아도 코드가 기본 모양을 만들고, WBP 자식에서 같은 이름의 위젯을 두면 그것을 씀.
 */
UCLASS()
class CATCHTHETAIL_API UCTPhonePopupWidget : public UCTUserWidget
{
	GENERATED_BODY()

public:
	// 알아볼 대상을 골랐을 때 (쿨타임 중에는 호출되지 않음)
	UPROPERTY(BlueprintAssignable, Category = "CT|UI")
	FCTPhoneQuerySelectedSignature OnQuerySelected;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Target;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Hunter;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Status;

private:
	// WBP에서 루트를 만들지 않았을 때만 기본 모양을 만듦
	void BuildDefaultLayout();

	void RefreshStatus();
	void Select(ECTPhoneQuery Query);

	// 지금 걸 수 있는가 (쿨타임이 끝남)
	bool bCanCall = true;
};
