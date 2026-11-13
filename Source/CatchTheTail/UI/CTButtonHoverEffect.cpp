// Copyright Moon Party Games. All Rights Reserved.
#include "CTButtonHoverEffect.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Styling/SlateTypes.h"

namespace
{
	const FString NoHoverMarker(TEXT("NoHover"));
	const FName EffectObjectName(TEXT("CTButtonHoverEffect"));

	// 확대, 떠오름 효과를 빼고 색 변화만 쓰는 버튼 이름
	bool IsColorOnlyButton(const FString& Name)
	{
		return Name == TEXT("Button_SelectMap");
	}
}

void UCTButtonHoverEffect::Attach(UUserWidget* Owner)
{
	if (!Owner || !Owner->WidgetTree) { return; }

	// 같은 위젯이 다시 만들어져도 효과 객체는 하나만
	UCTButtonHoverEffect* Effect = FindObject<UCTButtonHoverEffect>(Owner, *EffectObjectName.ToString());
	if (!Effect)
	{
		Effect = NewObject<UCTButtonHoverEffect>(Owner, EffectObjectName);
	}

	Effect->OwnerWidget = Owner;
	Effect->Collect(Owner);

	if (!Effect->TickHandle.IsValid())
	{
		Effect->TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(Effect, &UCTButtonHoverEffect::Tick));
	}
}

void UCTButtonHoverEffect::BeginDestroy()
{
	if (TickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
		TickHandle.Reset();
	}
	Super::BeginDestroy();
}

void UCTButtonHoverEffect::Collect(UUserWidget* Owner)
{
	Entries.Reset();
	Owner->WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		UButton* Button = Cast<UButton>(Widget);
		if (Button && !Button->GetName().Contains(NoHoverMarker) && !IsColorOnlyButton(Button->GetName()))
		{
			FEntry Entry;
			Entry.Button = Button;
			Entries.Add(Entry);
		}
	});
}

bool UCTButtonHoverEffect::Tick(float DeltaTime)
{
	// 위젯이 사라졌으면 멈춤
	if (!OwnerWidget.IsValid())
	{
		TickHandle.Reset();
		return false;
	}

	for (FEntry& Entry : Entries)
	{
		UButton* Button = Entry.Button.Get();
		if (!Button) { continue; }

		const bool bActive = Button->GetIsEnabled() && Button->IsVisible();
		const bool bPressed = bActive && Button->IsPressed();
		const bool bHovered = bActive && Button->IsHovered();

		const float TargetScale = bPressed ? PressedScale : (bHovered ? HoverScale : 1.f);
		const float TargetLift = bHovered && !bPressed ? -HoverLift : 0.f;

		const float NewScale = FMath::FInterpTo(Entry.Scale, TargetScale, DeltaTime, InterpSpeed);
		const float NewLift = FMath::FInterpTo(Entry.Lift, TargetLift, DeltaTime, InterpSpeed);

		// 변화가 없으면 건드리지 않음
		if (FMath::IsNearlyEqual(NewScale, Entry.Scale, 0.0005f) && FMath::IsNearlyEqual(NewLift, Entry.Lift, 0.005f)
			&& FMath::IsNearlyEqual(NewScale, TargetScale, 0.0005f))
		{
			continue;
		}

		Entry.Scale = NewScale;
		Entry.Lift = NewLift;
		Button->SetRenderScale(FVector2D(NewScale, NewScale));
		Button->SetRenderTranslation(FVector2D(0.f, NewLift));
	}

	return true;
}

void UCTButtonHoverEffect::ApplyDarkenStyle(UButton* Button)
{
	if (!Button) { return; }

	auto Darken = [](FSlateBrush Brush, float Factor, float OverlayAlpha)
	{
		FLinearColor Tint = Brush.TintColor.GetSpecifiedColor();
		if (Brush.DrawAs == ESlateBrushDrawType::NoDrawType || Tint.A < 0.05f)
		{
			Brush.DrawAs = ESlateBrushDrawType::Box;
			Brush.SetResourceObject(nullptr);
			Brush.TintColor = FLinearColor(0.f, 0.f, 0.f, OverlayAlpha);
		}
		else
		{
			Tint.R *= Factor; Tint.G *= Factor; Tint.B *= Factor;
			Brush.TintColor = Tint;
		}
		return Brush;
	};

	FButtonStyle Style = Button->GetStyle();
	Style.SetHovered(Darken(Style.Normal, 0.68f, 0.28f));
	Style.SetPressed(Darken(Style.Normal, 0.56f, 0.40f));
	Button->SetStyle(Style);
}
