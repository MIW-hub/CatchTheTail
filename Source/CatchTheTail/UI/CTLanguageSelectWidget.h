// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CTLanguageSelectWidget.generated.h"

class UButton;

// 언어를 골라 확인함
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCTLanguageConfirmedSignature);

// 언어 선택 화면
UCLASS(Abstract)
class CATCHTHETAIL_API UCTLanguageSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 확인 시 호출
	UPROPERTY(BlueprintAssignable, Category = "CT|Language")
	FCTLanguageConfirmedSignature OnLanguageConfirmed;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 언어가 바뀜 (연출용)
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Language")
	void BP_OnLanguageSelected(const FString& CultureCode);

	// 위젯 (WBP 같은 이름 자동 연결)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_LanguageKo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_LanguageEn;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_LanguageJa;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Confirm;

	// 고르지 않은 언어 버튼의 투명도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Language", meta = (ClampMin = "0", ClampMax = "1"))
	float UnselectedOpacity = 0.55f;

private:
	UFUNCTION()
	void HandleKoClicked();

	UFUNCTION()
	void HandleEnClicked();

	UFUNCTION()
	void HandleJaClicked();

	UFUNCTION()
	void HandleConfirmClicked();

	void SelectLanguage(const FString& CultureCode);
	void RefreshSelection();
};
