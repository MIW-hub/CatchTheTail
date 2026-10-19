// Copyright Moon Party Games. All Rights Reserved.
#include "CTTail.h"
#include "../Core/CTGameMode.h"
#include "../Player/CTCharacter.h"
#include "../Player/CTPlayerState.h"
#include "Components/CapsuleComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** 꼬리 조각은 이동을 방해하거나 다른 것과 부딪히지 않는 연출용입니다. */
	void SetupPartComponent(USkeletalMeshComponent* Part)
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->bCastDynamicShadow = true;
	}
}

ACTTail::ACTTail()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // 불이 바뀌거나 몸통이 붙는 동안에만 켭니다.

	bReplicates = true;
	SetReplicatingMovement(false); // 위치는 캐릭터에 붙어서 따라갑니다.

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BaseMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(Root);
	SetupPartComponent(BaseMesh);

	CapMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CapMesh"));
	CapMesh->SetupAttachment(Root);
	SetupPartComponent(CapMesh);
	CapMesh->SetVisibility(false); // 몸통이 생기면 보임

	// 기본 메시. (블루프린트에서 다른 메시로 바꿀 수 있습니다)
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BaseFinder(TEXT("/Game/Character/Socket/tail_03.tail_03"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BodyFinder(TEXT("/Game/Character/Socket/tail_02.tail_02"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CapFinder(TEXT("/Game/Character/Socket/tail_01.tail_01"));
	if (BaseFinder.Succeeded()) { BaseAsset = BaseFinder.Object; }
	if (BodyFinder.Succeeded()) { BodyAsset = BodyFinder.Object; }
	if (CapFinder.Succeeded()) { CapAsset = CapFinder.Object; }
}

void ACTTail::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACTTail, bCaught);
}

void ACTTail::ApplyMeshAssets()
{
	if (bMeshAssetsApplied) { return; }
	bMeshAssetsApplied = true;

	// 블루프린트에서 바꾼 메시를 반영합니다. (생성자 시점에는 아직 BP 값이 적용되기 전)
	BaseMesh->SetSkeletalMeshAsset(BaseAsset);
	CapMesh->SetSkeletalMeshAsset(CapAsset);
}

void ACTTail::BeginPlay()
{
	Super::BeginPlay();

	ApplyMeshAssets();
	LightAmount = bCaught ? 0.f : 1.f;
	RefreshVisual();
}

void ACTTail::OnRep_AttachmentReplication()
{
	Super::OnRep_AttachmentReplication();

	// 클라이언트에서 캐릭터에 붙은 뒤에야 주인의 색과 꼬리 수를 알 수 있습니다.
	RefreshVisual();
}

ACTCharacter* ACTTail::GetTailOwnerCharacter() const
{
	return Cast<ACTCharacter>(GetAttachParentActor());
}

FVector ACTTail::GetTouchLocation() const
{
	// 몸통이 있으면 맨 끝 캡, 없으면 받침이 "꼬리 끝"입니다.
	const USkeletalMeshComponent* Tip = BodyMeshes.Num() > 0 ? CapMesh.Get() : BaseMesh.Get();
	return Tip->Bounds.Origin;
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

	bool bBusy = false;

	// 불 켜짐 / 꺼짐
	const float LightTarget = bCaught ? 0.f : 1.f;
	if (!FMath::IsNearlyEqual(LightAmount, LightTarget))
	{
		LightAmount = FMath::FInterpConstantTo(LightAmount, LightTarget, DeltaSeconds, 1.f / FMath::Max(LightFadeSeconds, 0.01f));
		ApplyLook();
		bBusy = true;
	}

	// 새 몸통이 작게 시작해서 커지는 연출
	for (int32 Index = 0; Index < BodyMeshes.Num(); ++Index)
	{
		if (BodyPop[Index] >= 1.f) { continue; }

		BodyPop[Index] = FMath::Min(1.f, BodyPop[Index] + DeltaSeconds / FMath::Max(SegmentPopSeconds, 0.01f));
		BodyMeshes[Index]->SetRelativeScale3D(FVector(FMath::Lerp(0.3f, 1.f, BodyPop[Index])));
		bBusy = true;
	}

	if (!bBusy)
	{
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

	// 닿았는지: 꼬리 끝과 손을 뻗은 캐릭터 사이 거리
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
// 몸통 쌓기
// ============================================================

USkeletalMeshComponent* ACTTail::AddBodySegment()
{
	USkeletalMeshComponent* Segment = NewObject<USkeletalMeshComponent>(this);
	SetupPartComponent(Segment);
	Segment->SetSkeletalMeshAsset(BodyAsset);
	Segment->SetupAttachment(Root);
	Segment->RegisterComponent();
	return Segment;
}

void ACTTail::RefreshSegments()
{
	// 주인의 꼬리 수 (복제된 PlayerState 값)
	int32 TargetCount = 0;
	if (const ACTCharacter* OwnerCharacter = GetTailOwnerCharacter())
	{
		if (const ACTPlayerState* OwnerState = OwnerCharacter->GetPlayerState<ACTPlayerState>())
		{
			TargetCount = FMath::Clamp(OwnerState->GetTailCount(), 0, MaxBodySegments);
		}
	}

	const int32 OldCount = BodyMeshes.Num();
	if (TargetCount == OldCount) { return; }

	// 늘리기: 새 몸통은 작게 시작해서 커집니다.
	while (BodyMeshes.Num() < TargetCount)
	{
		USkeletalMeshComponent* Segment = AddBodySegment();
		BodyMeshes.Add(Segment);
		if (SegmentPopSeconds > 0.f)
		{
			BodyPop.Add(0.f);
			Segment->SetRelativeScale3D(FVector(0.3f));
			SetActorTickEnabled(true);
		}
		else
		{
			BodyPop.Add(1.f);
		}
	}

	// 줄이기: 맨 끝 몸통부터 없앱니다.
	while (BodyMeshes.Num() > TargetCount)
	{
		if (USkeletalMeshComponent* Last = BodyMeshes.Pop())
		{
			Last->DestroyComponent();
		}
		BodyPop.Pop();
	}

	// 몸통은 꼬리 방향(+Z)으로 SegmentSpacing씩 이어 붙입니다.
	for (int32 Index = 0; Index < BodyMeshes.Num(); ++Index)
	{
		BodyMeshes[Index]->SetRelativeLocation(FVector(0.f, 0.f, Index * SegmentSpacing));
	}

	// 캡은 맨 끝 몸통 위에. (몸통이 없으면 숨김. 캡 메시는 첫 몸통의 윗면에 맞게 만들어져 있음)
	const bool bHasBody = BodyMeshes.Num() > 0;
	CapMesh->SetVisibility(bHasBody);
	if (bHasBody)
	{
		CapMesh->SetRelativeLocation(FVector(0.f, 0.f, (BodyMeshes.Num() - 1) * SegmentSpacing));
	}

	RebuildMaterials();
	BP_OnSegmentCountChanged(BodyMeshes.Num());
}

// ============================================================
// 겉모습
// ============================================================

void ACTTail::RebuildMaterials()
{
	Materials.Reset();

	auto Collect = [this](UMeshComponent* Part)
	{
		if (!Part) { return; }

		for (int32 SlotIndex = 0; SlotIndex < Part->GetNumMaterials(); ++SlotIndex)
		{
			UMaterialInterface* Material = Part->GetMaterial(SlotIndex);
			if (!Material) { continue; }

			UMaterialInstanceDynamic* Dynamic = Cast<UMaterialInstanceDynamic>(Material);
			if (!Dynamic)
			{
				Dynamic = Part->CreateAndSetMaterialInstanceDynamic(SlotIndex);
			}
			if (Dynamic)
			{
				Materials.Add(Dynamic);
			}
		}
	};

	// 이 꼬리 액터의 모든 메시(받침, 몸통, 캡, 블루프린트에서 직접 추가한 메시)에 같은 색을 적용합니다.
	TInlineComponentArray<UMeshComponent*> MeshComponents;
	GetComponents(MeshComponents);
	for (UMeshComponent* Part : MeshComponents)
	{
		Collect(Part);
	}
}

void ACTTail::RefreshVisual()
{
	ApplyMeshAssets();

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

	RefreshSegments();
	if (Materials.IsEmpty())
	{
		RebuildMaterials();
	}
	ApplyLook();
}

void ACTTail::ApplyLook()
{
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
