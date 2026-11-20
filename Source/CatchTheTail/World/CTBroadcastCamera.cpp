// Copyright Moon Party Games. All Rights Reserved.
#include "CTBroadcastCamera.h"
#include "../Player/CTCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "EngineUtils.h"
#include "UObject/UObjectIterator.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ACTBroadcastCamera::ACTBroadcastCamera()
{
	PrimaryActorTick.bCanEverTick = true;
	Cam = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	SetRootComponent(Cam);
	Cam->bConstrainAspectRatio = false;
	Cam->SetFieldOfView(70.f);
}

void ACTBroadcastCamera::HideWidgets()
{
	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		if (It->GetWorld() == GetWorld() && It->IsInViewport() && It->GetVisibility() != ESlateVisibility::Collapsed)
		{
			It->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void ACTBroadcastCamera::Apply(const FVector& Target, const FRotator& Rot, float Fov, bool bSnap, float PosSpeed, float RotSpeed)
{
	const float Dt = GetWorld()->GetDeltaSeconds();
	if (bSnap || !bPoseInit)
	{
		SetActorLocationAndRotation(Target, Rot);
	}
	else
	{
		SetActorLocationAndRotation(FMath::VInterpTo(GetActorLocation(), Target, Dt, PosSpeed), FMath::RInterpTo(GetActorRotation(), Rot, Dt, RotSpeed));
	}
	bPoseInit = true;
	Cam->SetFieldOfView(Fov);
}

FVector ACTBroadcastCamera::Unoccluded(const FVector& LookAt, const FVector& Wanted) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CTBroadcastTrace), false);
	for (ACTCharacter* C : Chars)
	{
		if (!C) { continue; }
		Params.AddIgnoredActor(C);
		TArray<AActor*> Attached;
		C->GetAttachedActors(Attached);
		for (AActor* A : Attached) { Params.AddIgnoredActor(A); } // 꼬리, 든 물건
	}
	// 시선이 큰 벽/천장에 막힐 때만 앞으로 당김. 주인공 근처의 작은 물체(바닥 아이템 등)는 무시
	FHitResult Hit;
	const FVector Dir = (Wanted - LookAt).GetSafeNormal();
	if (GetWorld()->LineTraceSingleByChannel(Hit, LookAt + Dir * 200.f, Wanted, ECC_Visibility, Params))
	{
		return Hit.Location - Dir * 40.f;
	}
	return Wanted;
}

void ACTBroadcastCamera::Tick(float Dt)
{
	Super::Tick(Dt);
	Elapsed += Dt;

	// 시점은 계속 이 카메라로 (관전자 폰이나 맵 이동이 가져가지 못하게)
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (PC->GetViewTarget() != this) { PC->SetViewTarget(this); }
	}

	HudTimer -= Dt;
	if (bHideHud && HudTimer <= 0.f) { HideWidgets(); HudTimer = 0.25f; }

	Chars.Reset();
	for (TActorIterator<ACTCharacter> It(GetWorld()); It; ++It)
	{
		if (!It->IsHidden()) { Chars.Add(*It); }
	}
	if (Chars.Num() == 0) { return; }
	Chars.Sort([](const ACTCharacter& A, const ACTCharacter& B) { return A.GetName() < B.GetName(); });

	if (!Hero || Hero->IsHidden() || Hero->IsActorBeingDestroyed()) { Hero = Chars[0]; bFocusInit = false; }

	FVector Sum = FVector::ZeroVector;
	for (ACTCharacter* C : Chars) { Sum += C->GetActorLocation(); }
	Centroid = Sum / Chars.Num();

	const FVector Vel = Hero->GetVelocity().GetSafeNormal2D();
	if (!Vel.IsNearlyZero()) { Heading = FMath::VInterpTo(Heading, Vel, Dt, 3.f).GetSafeNormal2D(); }

	if (Style == 1) { UpdateAerial(Dt); } else { UpdateGameplay(Dt); }

	LogTimer -= Dt;
	if (LogTimer <= 0.f)
	{
		LogTimer = 3.f;
		UE_LOG(LogTemp, Log, TEXT("[CT-Broadcast] t=%.1f 캐릭터 %d 주인공 %s 카메라 %s 모아본곳 %s"), Elapsed, Chars.Num(), *Hero->GetActorLocation().ToString(), *GetActorLocation().ToString(), *Centroid.ToString());
	}
}

void ACTBroadcastCamera::UpdateGameplay(float Dt)
{
	static const int32 Order[] = { 0, 4, 1, 2, 0, 3, 4, 1 };
	const float ShotLen = 5.5f;
	const int32 ShotIndex = static_cast<int32>(Elapsed / ShotLen);
	const int32 Shot = Order[ShotIndex % UE_ARRAY_COUNT(Order)];
	const bool bSnap = (ShotIndex != LastShot);
	LastShot = ShotIndex;

	const FVector H = Hero->GetActorLocation();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Heading).GetSafeNormal();
	FVector Pos, LookAt;
	float Fov = 70.f;
	float PosSpeed = 6.f;

	switch (Shot)
	{
	case 0: // 추격: 주인공 뒤에서 (게임 화면과 비슷)
		Pos = H - Heading * 480.f + FVector(0.f, 0.f, 260.f);
		LookAt = H + FVector(0.f, 0.f, 80.f);
		break;
	case 1: // 높은 쿼터뷰: 모인 곳을 내려다봄
	{
		const FVector Focus = FMath::Lerp(H, Centroid, 0.5f);
		Pos = Focus + FVector(-850.f, -850.f, 1250.f);
		LookAt = Focus;
		Fov = 68.f; PosSpeed = 3.f;
		break;
	}
	case 2: // 옆에서 따라감
		Pos = H + Right * (ShotIndex % 2 ? 650.f : -650.f) + FVector(0.f, 0.f, 150.f);
		LookAt = H + Heading * 160.f + FVector(0.f, 0.f, 60.f);
		Fov = 55.f;
		break;
	case 3: // 정면: 주인공이 카메라 쪽으로 달려옴
		Pos = H + Heading * 520.f + FVector(0.f, 0.f, 130.f);
		LookAt = H + FVector(0.f, 0.f, 60.f);
		Fov = 75.f;
		break;
	default: // 4: 가장 가까운 두 명 (몸싸움이 있는 곳)을 돌며 담음
	{
		float Best = TNumericLimits<float>::Max();
		FVector Mid = H;
		for (int32 i = 0; i < Chars.Num(); ++i)
		{
			for (int32 j = i + 1; j < Chars.Num(); ++j)
			{
				const float D = FVector::DistSquared(Chars[i]->GetActorLocation(), Chars[j]->GetActorLocation());
				if (D < Best) { Best = D; Mid = (Chars[i]->GetActorLocation() + Chars[j]->GetActorLocation()) * 0.5f; }
			}
		}
		const float A = Elapsed * 0.45f;
		Pos = Mid + FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * 620.f + FVector(0.f, 0.f, 330.f);
		LookAt = Mid + FVector(0.f, 0.f, 70.f);
		Fov = 62.f; PosSpeed = 4.f;
		break;
	}
	}

	Pos = Unoccluded(LookAt, Pos);
	Apply(Pos, (LookAt - Pos).Rotation(), Fov, bSnap, PosSpeed, 9.f);
}

void ACTBroadcastCamera::UpdateAerial(float Dt)
{
	const FVector Want = FMath::Lerp(Hero->GetActorLocation(), Centroid, 0.15f);
	if (!bFocusInit) { SmoothFocus = Want; bFocusInit = true; }
	SmoothFocus = FMath::VInterpTo(SmoothFocus, Want, Dt, 1.6f);

	// 처음 7초: 높은 곳에서 내려오며 진입, 이후 주인공 둘레를 비행기처럼 선회
	const float Intro = FMath::Clamp(1.f - Elapsed / 7.f, 0.f, 1.f);
	const float Ease = Intro * Intro;
	const float Altitude = (1050.f + 200.f * FMath::Sin(Elapsed * 0.35f) + Ease * 3200.f) * AerialScale;
	const float Radius = (950.f + 200.f * FMath::Sin(Elapsed * 0.2f) + Ease * 1200.f) * AerialScale;
	const float Angle = 0.6f + Elapsed * 0.22f;

	FVector Pos = SmoothFocus + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Radius + FVector(0.f, 0.f, Altitude);
	// 큰 상자에 시선이 막히면 당기지 않고 위로 올라감 (공중 촬영이라 고도를 높이면 됨)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(CTBroadcastAerial), false);
		for (ACTCharacter* C : Chars)
		{
			if (!C) { continue; }
			Params.AddIgnoredActor(C);
			TArray<AActor*> Attached;
			C->GetAttachedActors(Attached);
			for (AActor* A : Attached) { Params.AddIgnoredActor(A); }
		}
		const FVector Look = SmoothFocus + FVector(0.f, 0.f, 100.f);
		for (int32 Try = 0; Try < 14; ++Try)
		{
			FHitResult Hit;
			const FVector Dir = (Pos - Look).GetSafeNormal();
			const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Look + Dir * 200.f, Pos, ECC_Visibility, Params);
			FHitResult Up;
			const bool bUnder = GetWorld()->LineTraceSingleByChannel(Up, Pos + FVector(0.f, 0.f, 3000.f), Pos, ECC_Visibility, Params);
			if (!bBlocked && !(bUnder && Up.Location.Z + 350.f > Pos.Z)) { break; }
			Pos.Z += 250.f * AerialScale;
		}
	}
	FRotator Rot = (SmoothFocus - Pos).Rotation();
	Rot.Roll = 6.f * FMath::Sin(Elapsed * 0.6f); // 기체가 기울어 도는 느낌
	Apply(Pos, Rot, 62.f, false, 100.f, 100.f);
}
