// Copyright Moon Party Games. All Rights Reserved.
#include "CTEmoteBubbleComponent.h"
#include "CTEmoteBubbleWidget.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UCTEmoteBubbleComponent::UCTEmoteBubbleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// 3D 월드에 떠 있는 2D 그림. 아랫부분(말풍선 꼬리쪽)이 기준점
	SetWidgetSpace(EWidgetSpace::World);
	SetWidgetClass(UCTEmoteBubbleWidget::StaticClass());
	SetDrawSize(UCTEmoteBubbleWidget::GetDrawSize());
	SetPivot(FVector2D(0.5f, 0.f));
	SetTwoSided(true);
	SetBlendMode(EWidgetBlendMode::Transparent);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetCastShadow(false);
	SetCanEverAffectNavigation(false);
	SetVisibility(false);
}

void UCTEmoteBubbleComponent::ShowEmote(UTexture2D* Icon, FVector2D UVMin, FVector2D UVMax)
{
	// 전용 서버는 그릴 필요 없음
	if (GetNetMode() == NM_DedicatedServer) { return; }

	if (!GetUserWidgetObject()) { InitWidget(); }
	if (UCTEmoteBubbleWidget* Bubble = Cast<UCTEmoteBubbleWidget>(GetUserWidgetObject()))
	{
		Bubble->SetEmoteIcon(Icon, UVMin, UVMax);
	}

	Phase = EPhase::Rise;
	PhaseTime = 0.f;
	ElapsedTotal = 0.f;

	ApplyState(0.f, 0.f, PopSeconds > 0.f ? 0.6f : 1.f);
	SetVisibility(true);
	SetComponentTickEnabled(true);
	FaceCamera();
}

void UCTEmoteBubbleComponent::HideEmoteNow()
{
	Phase = EPhase::Idle;
	SetVisibility(false);
	SetComponentTickEnabled(false);
}

void UCTEmoteBubbleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (Phase == EPhase::Idle) { return; }

	PhaseTime += DeltaTime;
	ElapsedTotal += DeltaTime;

	// 나타날 때 살짝 커짐
	const float Pop = PopSeconds > 0.f ? FMath::InterpEaseOut(0.6f, 1.f, FMath::Clamp(ElapsedTotal / PopSeconds, 0.f, 1.f), 2.f) : 1.f;

	switch (Phase)
	{
	case EPhase::Rise:
	{
		const float T = FMath::Clamp(PhaseTime / RiseSeconds, 0.f, 1.f);
		// 천천히 -> 빠르게
		const float Alpha = FMath::InterpEaseOut(0.f, 1.f, T, EaseExponent);
		// 처음 0.25초 안에 나타남
		const float Opacity = FMath::Clamp(ElapsedTotal / 0.25f, 0.f, 1.f);
		ApplyState(Alpha, Opacity, Pop);
		if (T >= 1.f) { Phase = EPhase::Hold; PhaseTime = 0.f; }
		break;
	}
	case EPhase::Hold:
		ApplyState(1.f, 1.f, Pop);
		if (PhaseTime >= HoldSeconds) { Phase = EPhase::Fade; PhaseTime = 0.f; }
		break;
	case EPhase::Fade:
	{
		const float T = FMath::Clamp(PhaseTime / FadeSeconds, 0.f, 1.f);
		// 천천히 -> 빠르게 사라짐
		const float Opacity = 1.f - FMath::InterpEaseIn(0.f, 1.f, T, EaseExponent);
		ApplyState(1.f, Opacity, Pop);
		if (T >= 1.f) { HideEmoteNow(); return; }
		break;
	}
	default:
		break;
	}

	FaceCamera();
}

void UCTEmoteBubbleComponent::ApplyState(float Alpha, float Opacity, float Scale)
{
	SetRelativeLocation(StartOffset + FVector(0.f, 0.f, RiseDistance * Alpha));

	const float BaseScale = WorldWidth / FMath::Max(1.f, UCTEmoteBubbleWidget::GetDrawSize().X);
	SetRelativeScale3D(FVector(BaseScale * Scale));

	if (UCTEmoteBubbleWidget* Bubble = Cast<UCTEmoteBubbleWidget>(GetUserWidgetObject()))
	{
		Bubble->SetBubbleOpacity(Opacity);
	}
}

void UCTEmoteBubbleComponent::FaceCamera()
{
	// 보는 카메라와 나란히 세움 (그림이 기울어 보이지 않게)
	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Camera) { return; }

	const FQuat Facing = Camera->GetCameraRotation().Quaternion() * FRotator(0.f, 180.f, 0.f).Quaternion();
	SetWorldRotation(Facing);

	// 화면 오른쪽(카메라 기준)으로 비켜서 머리 오른쪽에 뜨게 함. 위치는 매번 ApplyState가 먼저 잡고 옴
	const FVector CameraRight = Camera->GetCameraRotation().RotateVector(FVector::RightVector);
	SetWorldLocation(GetComponentLocation() + CameraRight * ScreenOffsetRight);
}
