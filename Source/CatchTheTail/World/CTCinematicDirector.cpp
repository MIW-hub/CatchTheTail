// Copyright Moon Party Games. All Rights Reserved.
#include "CTCinematicDirector.h"
#include "../Player/CTCharacter.h"
#include "../Type/CTTypes.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

ACTCinematicDirector::ACTCinematicDirector()
{
	PrimaryActorTick.bCanEverTick = true;
}

ACTCharacter* ACTCinematicDirector::SpawnChicken(const FLinearColor& Tint)
{
	if (!CharacterClass) { return nullptr; }
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACTCharacter* Chicken = GetWorld()->SpawnActor<ACTCharacter>(CharacterClass, LaneStart, FRotator::ZeroRotator, Params);
	if (Chicken)
	{
		Chicken->SetCinematicTint(Tint);
		Chicken->GetCharacterMovement()->bRunPhysicsWithNoController = true; // 조종하는 컨트롤러가 없어도 움직임 계산
		Chicken->bUseControllerRotationYaw = false;
		Chicken->SetActorEnableCollision(false);
		SetChickenVisible(Chicken, false);
	}
	return Chicken;
}

ACameraActor* ACTCinematicDirector::SpawnCamera(float Fov)
{
	ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (Cam && Cam->GetCameraComponent())
	{
		Cam->GetCameraComponent()->SetFieldOfView(Fov);
		Cam->GetCameraComponent()->bConstrainAspectRatio = false;
	}
	return Cam;
}

void ACTCinematicDirector::SetChickenVisible(ACTCharacter* Chicken, bool bVisible)
{
	if (!Chicken) { return; }
	Chicken->SetActorHiddenInGame(!bVisible);
	TArray<AActor*> Attached;
	Chicken->GetAttachedActors(Attached);
	for (AActor* Child : Attached) { Child->SetActorHiddenInGame(!bVisible); }
}

void ACTCinematicDirector::Place(ACTCharacter* Chicken, const FVector& Location, float Yaw)
{
	if (!Chicken) { return; }
	Chicken->GetCharacterMovement()->StopMovementImmediately();
	bool bMoved = Chicken->SetActorLocationAndRotation(Location, FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	if (!bMoved) { bMoved = Chicken->TeleportTo(Location, FRotator(0.f, Yaw, 0.f)); }
	if (!bMoved && Chicken->GetRootComponent()) { Chicken->GetRootComponent()->SetWorldLocationAndRotation(Location, FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics); bMoved = true; }
	UE_LOG(LogTemp, Log, TEXT("[CT-Cine] Place %s -> %s 결과=%d 지금=%s 루트이동성=%d"), *Chicken->GetName(), *Location.ToString(), bMoved ? 1 : 0, *Chicken->GetActorLocation().ToString(), Chicken->GetRootComponent() ? static_cast<int32>(Chicken->GetRootComponent()->Mobility) : -1);
	Chicken->SetActorEnableCollision(true);
	SetChickenVisible(Chicken, true);
}

void ACTCinematicDirector::Run(ACTCharacter* Chicken, const FVector& Direction, float Scale)
{
	if (!Chicken) { return; }
	// 이동 계산에 기대지 않고 직접 옮기고, 속도값만 넣어 달리기 애니메이션이 나오게 함
	const float Speed = RunSpeed * Scale;
	const float Dt = GetWorld()->GetDeltaSeconds();
	Chicken->SetCinematicRun(true);
	Chicken->SetActorLocationAndRotation(Chicken->GetActorLocation() + Direction * Speed * Dt, FRotator(0.f, Direction.Rotation().Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	Chicken->GetCharacterMovement()->Velocity = Direction * Speed;
}

void ACTCinematicDirector::BeginPlay()
{
	Super::BeginPlay();
	White = SpawnChicken(FLinearColor(1.f, 1.f, 1.f, 1.f));
	Green = SpawnChicken(FLinearColor(0.056f, 0.351f, 0.063f, 1.f));
	Cams[0] = SpawnCamera(80.f);
	Cams[1] = SpawnCamera(62.f);
	Cams[2] = SpawnCamera(62.f);
	Cams[3] = SpawnCamera(70.f);
}

void ACTCinematicDirector::StartShot(int32 Shot)
{
	CurrentShot = Shot;
	const float Z = LaneStart.Z;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->SetViewTargetWithBlend(Cams[Shot], 0.f);
	}

	switch (Shot)
	{
	case 0:
		break; // 아무도 없음
	case 1: // 흰 닭이 +X 로 달림
		Place(White, FVector(LaneStart.X + 300.f, LaneStart.Y, Z), 0.f);
		break;
	case 2: // 흰 닭이 앞서 달리고 초록 닭이 뒤에서 쫓아감
		Place(White, FVector(LaneStart.X + 680.f, LaneStart.Y, Z), 0.f);
		Place(Green, FVector(LaneStart.X + 300.f, LaneStart.Y, Z), 0.f);
		break;
	case 3: // 정면: 닭들이 카메라(-X 쪽) 로 달려옴
		Place(White, FVector(LaneStart.X + 2350.f, LaneStart.Y, Z), 180.f);
		Place(Green, FVector(LaneStart.X + 2600.f, LaneStart.Y, Z), 180.f);
		Phase3 = 0;
		break;
	}
}

void ACTCinematicDirector::UpdateShot(float T)
{
	const float Z = LaneStart.Z;
	const FVector Side(0.f, SideSign * SideDistance, 0.f);
	const float SideYaw = SideSign < 0.f ? 90.f : -90.f;

	switch (CurrentShot)
	{
	case 0: // 전경: 높은 곳에서 천천히 다가감
	{
		const float Alpha = T / FMath::Max(ShotSeconds[0], 0.1f);
		const FVector Pos(LaneStart.X + 1800.f, LaneStart.Y + SideSign * FMath::Lerp(2600.f, 2100.f, Alpha), GroundZ + FMath::Lerp(650.f, 600.f, Alpha));
		Cams[0]->SetActorLocationAndRotation(Pos, FRotator(-13.f, SideYaw, 0.f));
		break;
	}
	case 1: // 흰 닭 옆면 따라가기
		Run(White, FVector::ForwardVector, 1.f);
		Cams[1]->SetActorLocationAndRotation(White->GetActorLocation() + Side + FVector(120.f, 0.f, 10.f), FRotator(0.f, SideYaw, 0.f));
		break;
	case 2: // 초록 닭 옆면 따라가기 (오른쪽에 흰 닭)
		Run(White, FVector::ForwardVector, 0.9f);
		Run(Green, FVector::ForwardVector, 1.f);
		Cams[2]->SetActorLocationAndRotation(Green->GetActorLocation() + Side + FVector(260.f, 0.f, 10.f), FRotator(0.f, SideYaw, 0.f));
		break;
	case 3: // 쿼터뷰에서 시작해 정면으로 돌아오며: 달려옴 → 잡힘/발버둥 → 흰 닭이 이겨 다시 달림, 초록 닭은 넘어졌다 일어나 쫓아감
	{
		const float StruggleEnd = CatchAtSeconds + StruggleSeconds;
		const FVector Fwd = FVector::ForwardVector; // +X (길 방향)

		// --- 닭 움직임 ---
		if (Phase3 == 0)
		{
			if (T < CatchAtSeconds)
			{
				Run(White, -Fwd, 0.92f);
				Run(Green, -Fwd, 1.f);
			}
			else
			{
				// 초록 닭이 흰 닭 바로 뒤(같은 줄)에서 꼬리를 잡음 -> 발버둥
				Phase3 = 1;
				CatchPoint = White->GetActorLocation();
				White->SetCinematicRun(false);
				Green->SetCinematicRun(false);
				White->GetCharacterMovement()->StopMovementImmediately();
				Green->GetCharacterMovement()->StopMovementImmediately();
				Place(White, CatchPoint, 180.f);
				Place(Green, CatchPoint + FVector(105.f, 0.f, 0.f), 180.f);
				if (PullMontage && Green->GetMesh() && Green->GetMesh()->GetAnimInstance()) { Green->GetMesh()->GetAnimInstance()->Montage_Play(PullMontage, 1.f); }
				if (FleeMontage && White->GetMesh() && White->GetMesh()->GetAnimInstance()) { White->GetMesh()->GetAnimInstance()->Montage_Play(FleeMontage, 1.f); }
			}
		}
		else if (Phase3 == 1 && T < StruggleEnd)
		{
			// 발버둥: 동작을 끊기지 않게 반복하고, 흰 닭은 뿌리치려 크게, 초록 닭은 버티며 작게 흔들림
			const float W = T - CatchAtSeconds;
			UAnimInstance* WA = White->GetMesh() ? White->GetMesh()->GetAnimInstance() : nullptr;
			UAnimInstance* GA = Green->GetMesh() ? Green->GetMesh()->GetAnimInstance() : nullptr;
			if (WA && FleeMontage && !WA->Montage_IsPlaying(FleeMontage)) { WA->Montage_Play(FleeMontage, 1.f); }
			if (GA && PullMontage && !GA->Montage_IsPlaying(PullMontage)) { GA->Montage_Play(PullMontage, 1.f); }
			White->SetActorLocationAndRotation(CatchPoint + FVector(-26.f * FMath::Abs(FMath::Sin(W * 11.f)), 10.f * FMath::Sin(W * 23.f), 0.f), FRotator(0.f, 180.f + 14.f * FMath::Sin(W * 19.f), 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			Green->SetActorLocationAndRotation(CatchPoint + FVector(105.f - 12.f * FMath::Abs(FMath::Sin(W * 11.f)), 5.f * FMath::Sin(W * 21.f), 0.f), FRotator(0.f, 180.f + 6.f * FMath::Sin(W * 17.f), 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		}
		else if (Phase3 == 1 && T >= StruggleEnd)
		{
			// 흰 닭이 이김: 뿌리치고 다시 달려감(카메라 반대쪽), 초록 닭은 뒤로 넘어짐
			Phase3 = 2;
			Phase3Start = T;
			if (UAnimInstance* WA = White->GetMesh() ? White->GetMesh()->GetAnimInstance() : nullptr) { WA->Montage_Stop(0.15f); }
			if (UAnimInstance* GA = Green->GetMesh() ? Green->GetMesh()->GetAnimInstance() : nullptr)
			{
				GA->Montage_Stop(0.1f);
				FallLen = FallMontage ? GA->Montage_Play(FallMontage, 1.f) : 0.f;
			}
		}
		else if (Phase3 == 2 && T >= Phase3Start + FallLen)
		{
			Phase3 = 3;
			Phase3Start = T;
			if (UAnimInstance* GA = Green->GetMesh() ? Green->GetMesh()->GetAnimInstance() : nullptr)
			{
				GetUpLen = GetUpMontage ? GA->Montage_Play(GetUpMontage, 1.f) : 0.f;
			}
		}
		else if (Phase3 == 3 && T >= Phase3Start + GetUpLen)
		{
			Phase3 = 4; // 일어나서 쫓아감
		}

		if (Phase3 >= 2) { Run(White, Fwd, 1.f); }
		if (Phase3 >= 4) { Run(Green, Fwd, 1.f); }

		// --- 카메라: 비스듬한 위쪽(쿼터뷰)에서 시작해 정면(낮은 시점)으로 돌아옴 ---
		const FVector Target = (Phase3 == 0 ? (White->GetActorLocation() + Green->GetActorLocation()) * 0.5f : CatchPoint) + FVector(0.f, 0.f, 55.f);
		const float Orbit = FMath::Clamp(T / FMath::Max(CatchAtSeconds + 0.4f, 0.1f), 0.f, 1.f);
		const float Ease = Orbit * Orbit * (3.f - 2.f * Orbit);
		const float Back = FMath::Clamp((T - StruggleEnd) / 1.2f, 0.f, 1.f); // 발버둥이 끝난 뒤에 정면으로
		const float YawOff = FMath::DegreesToRadians(FMath::Lerp(FMath::Lerp(58.f, 24.f, Ease), 0.f, Back)); // 길 축에서 옆으로 벌어진 각도 (발버둥 동안은 초록 닭이 보이게 비스듬히)
		const float Elev = FMath::DegreesToRadians(FMath::Lerp(28.f, 3.f, Ease));     // 올려다보는 각도
		const float Radius = FMath::Lerp(950.f, 560.f, Ease);
		const FVector Dir(-FMath::Cos(YawOff) * FMath::Cos(Elev), SideSign * FMath::Sin(YawOff) * FMath::Cos(Elev), FMath::Sin(Elev));
		const FVector CamPos = Target + Dir * Radius;
		Cams[3]->SetActorLocationAndRotation(CamPos, (Target - CamPos).Rotation());
		break;
	}
	}
}

void ACTCinematicDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!White || !Green) { return; }

	Elapsed += DeltaSeconds;
	float Start = 0.f;
	int32 Shot = 0;
	for (; Shot < 4; ++Shot)
	{
		if (Elapsed < Start + ShotSeconds[Shot]) { break; }
		Start += ShotSeconds[Shot];
	}
	Shot = FMath::Min(Shot, 3);
	if (Shot != CurrentShot) { StartShot(Shot); }
	UpdateShot(Elapsed - Start);

	// 매 프레임 시점을 현재 컷의 카메라로 (플레이어 컨트롤러가 늦게 만들어져도 맞춤)
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->SetViewTargetWithBlend(Cams[Shot], 0.f);
	}

	LogTimer += DeltaSeconds;
	if (LogTimer >= 0.5f)
	{
		LogTimer = 0.f;
		UE_LOG(LogTemp, Log, TEXT("[CT-Cine] t=%.1f 컷%d 흰닭=%s 초록닭=%s 카메라=%s"), Elapsed, Shot + 1, *White->GetActorLocation().ToString(), *Green->GetActorLocation().ToString(), *Cams[Shot]->GetActorLocation().ToString());
	}
}
