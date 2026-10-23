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
	// 연출 전용, 충돌/이동 영향 없음
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
	PrimaryActorTick.bStartWithTickEnabled = false; // 변화 중에만 켬

	bReplicates = true;
	SetReplicatingMovement(false); // 캐릭터에 붙어 따라감

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BaseMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(Root);
	SetupPartComponent(BaseMesh);

	CapMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CapMesh"));
	CapMesh->SetupAttachment(Root);
	SetupPartComponent(CapMesh);
	CapMesh->SetVisibility(false); // 몸통 생기면 표시

	// 기본 메시, BP에서 교체 가능
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

	// BP 값은 생성자 이후 적용
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

	// 복제 스케일이 소수 2자리로 양자화돼 0이 되므로 주인이 오프셋 재계산
	if (ACTCharacter* OwnerCharacter = GetTailOwnerCharacter())
	{
		OwnerCharacter->ApplyTailOffset();
	}

	// 부착 후에야 주인 색/꼬리 수 확인 가능
	RefreshVisual();
}

ACTCharacter* ACTTail::GetTailOwnerCharacter() const
{
	return Cast<ACTCharacter>(GetAttachParentActor());
}

FVector ACTTail::GetTouchLocation() const
{
	// 끝: 캡, 없으면 받침
	const USkeletalMeshComponent* Tip = BodyMeshes.Num() > 0 ? CapMesh.Get() : BaseMesh.Get();
	return Tip->Bounds.Origin;
}

// 잡힘 상태

void ACTTail::SetCaught(bool bNewCaught)
{
	if (!HasAuthority()) { return; }
	if (bCaught == bNewCaught) { return; }

	bCaught = bNewCaught;
	ForceNetUpdate();
	OnRep_Caught(); // 서버에도 같은 연출
}

void ACTTail::OnRep_Caught()
{
	// 켜짐↔꺼짐 서서히 전환
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

	// 새 몸통 성장 연출
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

// 닿음 판정 (서버)

bool ACTTail::TryTouch(ACTCharacter* Toucher)
{
	if (!HasAuthority() || !Toucher || bCaught) { return false; }

	ACTCharacter* OwnerCharacter = GetTailOwnerCharacter();
	if (!OwnerCharacter || OwnerCharacter == Toucher) { return false; }

	// 잡기 동작 중에만 접촉
	if (!Toucher->IsGrabActive()) { return false; }

	// 꼬리 끝~캐릭터 거리
	const float Reach = TouchRadius + Toucher->GetCapsuleComponent()->GetScaledCapsuleRadius();
	if (FVector::Dist(Toucher->GetActorLocation(), GetTouchLocation()) > Reach) { return false; }

	ACTPlayerState* GrabberState = Toucher->GetPlayerState<ACTPlayerState>();
	ACTPlayerState* VictimState = OwnerCharacter->GetPlayerState<ACTPlayerState>();
	ACTGameMode* CTGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACTGameMode>() : nullptr;
	if (!GrabberState || !VictimState || !CTGameMode) { return false; }

	// 목표 색/유령/게임 중은 GameMode 확인
	return CTGameMode->HandleTailGrabbed(GrabberState, VictimState);
}

// 몸통 쌓기

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
	// 주인 꼬리 수 (PlayerState 복제값)
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

	// 증가: 새 몸통 성장
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

	// 감소: 끝 몸통부터
	while (BodyMeshes.Num() > TargetCount)
	{
		if (USkeletalMeshComponent* Last = BodyMeshes.Pop())
		{
			Last->DestroyComponent();
		}
		BodyPop.Pop();
	}

	// 몸통은 +Z로 SegmentSpacing씩
	for (int32 Index = 0; Index < BodyMeshes.Num(); ++Index)
	{
		BodyMeshes[Index]->SetRelativeLocation(FVector(0.f, 0.f, Index * SegmentSpacing));
	}

	// 캡은 끝 몸통 위, 몸통 없으면 숨김 (캡 메시는 첫 몸통 윗면 기준)
	const bool bHasBody = BodyMeshes.Num() > 0;
	CapMesh->SetVisibility(bHasBody);
	if (bHasBody)
	{
		CapMesh->SetRelativeLocation(FVector(0.f, 0.f, (BodyMeshes.Num() - 1) * SegmentSpacing));
	}

	RebuildMaterials();
	BP_OnSegmentCountChanged(BodyMeshes.Num());
}

// 겉모습

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

	// 모든 메시에 같은 색 적용 (BP 추가분 포함)
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

	// 주인 색
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
	// LightAmount 1=주인색+불빛, 0=회색
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
