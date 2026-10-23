// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CTIntroWidget.generated.h"

class UTextBlock;

// 인트로 종료
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCTIntroFinishedSignature);

UCLASS(Abstract)
class CATCHTHETAIL_API UCTIntroWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "CT|Intro")
	FCTIntroFinishedSignature OnIntroFinished;

	// 인트로 즉시 종료
	UFUNCTION(BlueprintCallable, Category = "CT|Intro")
	void FinishIntro();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 인트로 시작
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Intro")
	void BP_OnIntroStarted();

	// 페이드 인(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Intro", meta = (ClampMin = "0"))
	float FadeInSeconds = 0.8f;

	// 유지 시간(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Intro", meta = (ClampMin = "0"))
	float HoldSeconds = 2.0f;

	// 페이드 아웃(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Intro", meta = (ClampMin = "0"))
	float FadeOutSeconds = 0.6f;

	// 입력으로 건너뛰기
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Intro")
	bool bAllowSkip = true;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Skip;

private:
	float ElapsedSeconds = 0.f;
	bool bFinished = false;
};
