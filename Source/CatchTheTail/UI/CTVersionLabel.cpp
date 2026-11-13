// Copyright Moon Party Games. All Rights Reserved.
#include "CTVersionLabel.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Fonts/SlateFontInfo.h"
#include "GeneralProjectSettings.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// "v0.1.0  ·  초기 버전" (언어에 맞춰 문구만 바뀜)
	FText MakeLabelText()
	{
		const FString Version = GetDefault<UGeneralProjectSettings>()->ProjectVersion;
		const FString Language = FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName();

		FString Early = TEXT("Early Version");
		if (Language == TEXT("ko")) { Early = TEXT("초기 버전"); }
		else if (Language == TEXT("ja")) { Early = TEXT("初期バージョン"); }

		return FText::FromString(FString::Printf(TEXT("v%s  ·  %s"), *Version, *Early));
	}
}

void CTVersionLabel::Show(const UWorld* World, TSharedPtr<SWidget>& Holder)
{
	UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;
	if (!Viewport || Holder.IsValid()) { return; }

	Holder = SNew(SBox)
		.Visibility(EVisibility::HitTestInvisible)
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.f, 0.f, 24.f, 16.f))
		[
			SNew(STextBlock)
			.Text(TAttribute<FText>::CreateLambda([]() { return MakeLabelText(); }))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
			.ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.85f))
			.ShadowOffset(FVector2D(1.f, 1.f))
			.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f))
		];

	// UMG 위젯 위에 그림
	Viewport->AddViewportWidgetContent(Holder.ToSharedRef(), 100);
}

void CTVersionLabel::Hide(const UWorld* World, TSharedPtr<SWidget>& Holder)
{
	UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;
	if (Holder.IsValid() && Viewport)
	{
		Viewport->RemoveViewportWidgetContent(Holder.ToSharedRef());
	}
	Holder.Reset();
}
