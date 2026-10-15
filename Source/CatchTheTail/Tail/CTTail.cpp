// Copyright Moon Party Games. All Rights Reserved.
#include "CTTail.h"
#include "../Core/CTGameMode.h"
#include "../Player/CTCharacter.h"
#include "../Player/CTPlayerState.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"

ACTTail::ACTTail()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // 불이 서서히 바뀌는 동안에만 켭니다.

	bReplicates = true;
	SetReplicatingMovement(false); // 위치는 캐릭터에 붙어서 따라갑니다.

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	TailMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TailMesh"));
	TailMesh->SetupAttachment(Root);
	TailMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 이동을 방해하지 않게
	TailMesh->SetGenerateOverlapEvents(false);

	TouchSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TouchSphere"));
	TouchSphere->SetupAttachment(Root);
	TouchSphere->SetSphereRadius(TouchRadius);
	TouchSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TouchSphere->SetHiddenInGame(true);
}

void ACTTail::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACTTail, bCaught);
}

void ACTTail::BeginPlay()
{
	Super::BeginPlay();

	TouchSphere->SetSphereRadius(TouchRadius);
	LightAmount = bCaught ? 0.f : 1.f;
	RefreshVisual();
}

void ACTTail::OnRep_AttachmentReplication()
{
	Super::OnRep_AttachmentReplication();

	// 클라이언트에서 캐릭터에 붙은 뒤에야 주인의 색을 알 수 있습니다.
	RefreshVisual();
}

ACTCharacter* ACTTail::GetTailOwnerCharacter() const
{
	return Cast<ACTCharacter>(GetAttachParentActor());
}

FVector ACTTail::GetTouchLocation() const
{
	return TouchSphere->GetComponentLocation();
}

// ============================================================
// 잡힘 상태
// ============================================================

void ACTTail::SetCaught(bool bNewCaught)
{
	if (!HasAuthority()) { return; }
	if (bCaught == bNewCaught) { return; }

	bCaught = bNewCaught;
	ForceNetUpdate();
	OnRep_Caught(); // 서버(방장) 화면에도 같은 연출
}

void ACTTail::OnRep_Caught()
{
	// 켜짐 ↔ 꺼짐 전환을 서서히 보여 줍니다.
	if (LightFadeSeconds > 0.f)
	{
		SetActorTickEnabled(true);
	}
	else
	{
		LightAmount = bCaught ? 0.f : 1.f;
	}

	RefreshVisual();
	BP_OnCaughtChanged(bCaught);
}

void ACTTail::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Target = bCaught ? 0.f : 1.f;
	LightAmount = FMath::FInterpConstantTo(LightAmount, Target, DeltaSeconds, 1.f / FMath::Max(LightFadeSeconds, 0.01f));
	ApplyLook();

	if (FMath::IsNearlyEqual(LightAmount, Target))
	{
		LightAmount = Target;
		ApplyLook();
		SetActorTickEnabled(false);
	}
}

// ============================================================
// 닿음 판정 (서버)
// ============================================================

bool ACTTail::TryTouch(ACTCharacter* Toucher)
{
	if (!HasAuthority() || !Toucher || bCaught) { return false; }

	ACTCharacter* OwnerCharacter = GetTailOwnerCharacter();
	if (!OwnerCharacter || OwnerCharacter == Toucher) { return false; }

	// 잡기 동작 중일 때만 닿을 수 있습니다.
	if (!Toucher->IsGrabActive()) { return false; }

	// 닿았는지: 꼬리 중심과 손을 뻗은 캐릭터 사이 거리
	const float Reach = TouchRadius + Toucher->GetCapsuleComponent()->GetScaledCapsuleRadius();
	if (FVector::Dist(Toucher->GetActorLocation(), GetTouchLocation()) > Reach) { return false; }

	ACTPlayerState* GrabberState = Toucher->GetPlayerState<ACTPlayerState>();
	ACTPlayerState* VictimState = OwnerCharacter->GetPlayerState<ACTPlayerState>();
	ACTGameMode* CTGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACTGameMode>() : nullptr;
	if (!GrabberState || !VictimState || !CTGameMode) { return false; }

	// 목표 색인지, 유령인지, 게임 중인지는 GameMode가 확인합니다.
	// 성공하면 주인이 유령이 되고, 캐릭터가 이 꼬리의 SetCaught(true)를 호출합니다.
	return CTGameMode->HandleTailGrabbed(GrabberState, VictimState);
}

// ============================================================
// 겉모습
// ============================================================

void ACTTail::EnsureMaterials()
{
	const int32 SlotCount = TailMesh->GetNumMaterials();
	if (Materials.Num() == SlotCount) { return; }

	Materials.Reset();
	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		Materials.Add(TailMesh->GetMaterial(SlotIndex)
			? TailMesh->CreateAndSetMaterialInstanceDynamic(SlotIndex)
			: nullptr);
	}
}

void ACTTail::RefreshVisual()
{
	// 주인의 색을 가져옵니다.
	LitColor = NoOwnerColor;
	if (const ACTCharacter* OwnerCharacter = GetTailOwnerCharacter())
	{
		if (const ACTPlayerState* OwnerState = OwnerCharacter->GetPlayerState<ACTPlayerState>())
		{
			if (OwnerState->GetAssignedColor() != ECTColor::None)
			{
				LitColor = ACTPlayerState::GetLinearColorOf(OwnerState->GetAssignedColor());
			}
		}
	}

	ApplyLook();
}

void ACTTail::ApplyLook()
{
	EnsureMaterials();

	// LightAmount 1 = 주인 색 + 불빛, 0 = 회색 + 불빛 없음
	const FLinearColor Color = FMath::Lerp(CaughtColor, LitColor, LightAmount);
	const float Emissive = LitEmissiveStrength * LightAmount;

	for (UMaterialInstanceDynamic* Material : Materials)
	{
		if (Material)
		{
			Material->SetVectorParameterValue(ColorParameterName, Color);
			Material->SetScalarParameterValue(EmissiveParameterName, Emissive);
		}
	}
}
