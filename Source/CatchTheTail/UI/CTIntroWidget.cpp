// Copyright Moon Party Games. All Rights Reserved.
#include "CTIntroWidget.h"
#include "Components/TextBlock.h"

void UCTIntroWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 키 입력을 받으려면 포커스가 가능해야 합니다.
	SetIsFocusable(true);

	ElapsedSeconds = 0.f;
	bFinished = false;
	SetRenderOpacity(FadeInSeconds > 0.f ? 0.f : 1.f);

	if (Text_Skip)
	{
		Text_Skip->SetVisibility(bAllowSkip ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	BP_OnIntroStarted();
}

void UCTIntroWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bFinished) { return; }

	ElapsedSeconds += InDeltaTime;

	const float FadeOutStart = FadeInSeconds + HoldSeconds;
	const float TotalSeconds = FadeOutStart + FadeOutSeconds;

	float Opacity = 1.f;
	if (ElapsedSeconds < FadeInSeconds)
	{
		Opacity = ElapsedSeconds / FadeInSeconds;
	}
	else if (ElapsedSeconds > FadeOutStart && FadeOutSeconds > 0.f)
	{
		Opacity = 1.f - (ElapsedSeconds - FadeOutStart) / FadeOutSeconds;
	}
	SetRenderOpacity(FMath::Clamp(Opacity, 0.f, 1.f));

	if (ElapsedSeconds >= TotalSeconds)
	{
		FinishIntro();
	}
}

FReply UCTIntroWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (bAllowSkip)
	{
		FinishIntro();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UCTIntroWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bAllowSkip)
	{
		FinishIntro();
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UCTIntroWidget::FinishIntro()
{
	if (bFinished) { return; }
	bFinished = true;

	OnIntroFinished.Broadcast();
}
