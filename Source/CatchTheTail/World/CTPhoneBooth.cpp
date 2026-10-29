// Copyright Moon Party Games. All Rights Reserved.
#include "CTPhoneBooth.h"
#include "../Core/CTGameState.h"
#include "../Player/CTCharacter.h"
#include "../Player/CTPlayerState.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// 서버가 부스를 확인하는 간격(초)
	constexpr float ServerCheckInterval = 0.1f;
}

ACTPhoneBooth::ACTPhoneBooth()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->SetBoxExtent(FVector(60.f, 60.f, 110.f));
	Trigger->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	Trigger->SetGenerateOverlapEvents(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Trigger);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(1.2f, 1.2f, 2.2f));

	// 임시 모양 (블루프린트에서 부스 메시로 교체)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		Mesh->SetStaticMesh(CubeFinder.Object);
	}
}

void ACTPhoneBooth::BeginPlay()
{
	Super::BeginPlay();

	// 임시 모양일 때만 빨간색
	if (Mesh->GetNumMaterials() > 0 && Mesh->GetMaterial(0) && Mesh->GetMaterial(0)->GetName().Contains(TEXT("BasicShapeMaterial")))
	{
		if (UMaterialInstanceDynamic* Material = Mesh->CreateAndSetMaterialInstanceDynamic(0))
		{
			Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.7f, 0.05f, 0.05f, 1.f));
		}
	}
}

void ACTPhoneBooth::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority()) { return; }

	TickAccumulator += DeltaSeconds;
	if (TickAccumulator >= ServerCheckInterval)
	{
		TickAccumulator = 0.f;
		ServerTick(GetWorld()->GetTimeSeconds());
	}
}

bool ACTPhoneBooth::IsCharacterInside(const ACTCharacter* Character) const
{
	return Character && Trigger->IsOverlappingActor(Character);
}

bool ACTPhoneBooth::CanUsePhone(const ACTCharacter* Character)
{
	const UWorld* World = Character ? Character->GetWorld() : nullptr;
	const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	const ACTPlayerState* PlayerState = Character ? Character->GetPlayerState<ACTPlayerState>() : nullptr;

	return CTGameState && PlayerState
		&& CTGameState->GetPhase() == ECTGamePhase::Playing
		&& !PlayerState->IsGhost()
		&& PlayerState->GetAssignedColor() != ECTColor::None;
}

void ACTPhoneBooth::ServerTick(double Now)
{
	// 쓰는 중: 나가거나 유령이 되면 취소, 시간이 되면 결과
	if (ACTCharacter* Current = CurrentUser.Get())
	{
		if (!IsCharacterInside(Current) || !CanUsePhone(Current)) { CancelUse(); }
		else if (Now >= UseEndTime) { FinishUse(Now); }
		return;
	}
	CurrentUser.Reset();

	// 나간 사람은 다시 쓸 수 있게 목록에서 뺌
	for (auto It = WaitForExit.CreateIterator(); It; ++It)
	{
		if (!It->IsValid() || !IsCharacterInside(It->Get())) { It.RemoveCurrent(); }
	}

	if (Now < CooldownEndTime) { return; }

	TArray<AActor*> Overlapping;
	Trigger->GetOverlappingActors(Overlapping, ACTCharacter::StaticClass());
	for (AActor* Actor : Overlapping)
	{
		ACTCharacter* Character = Cast<ACTCharacter>(Actor);
		if (!Character || !CanUsePhone(Character) || WaitForExit.Contains(Character)) { continue; }

		BeginUse(Character, Now);
		return;
	}
}

void ACTPhoneBooth::BeginUse(ACTCharacter* Character, double Now)
{
	ACTPlayerState* PlayerState = Character->GetPlayerState<ACTPlayerState>();
	if (!PlayerState) { return; }

	// 횟수를 다 썼으면 한 번만 알려 주고, 나갔다 들어올 때까지 조용히
	if (PlayerState->GetPhoneUsesLeft() <= 0)
	{
		WaitForExit.Add(Character);
		Character->ClientShowNotice(NSLOCTEXT("CT", "Phone_NoUses", "전화를 더 쓸 수 없습니다."), NoticeSeconds);
		return;
	}

	CurrentUser = Character;
	UseEndTime = Now + UseDurationSeconds;
	Character->SetBoothLocked(true);
	Character->ClientShowNotice(NSLOCTEXT("CT", "Phone_Calling", "전화 거는 중..."), UseDurationSeconds);
	BP_OnUseStarted(Character);
}

void ACTPhoneBooth::CancelUse()
{
	ACTCharacter* Character = CurrentUser.Get();
	CurrentUser.Reset();
	if (Character)
	{
		Character->SetBoothLocked(false);
		BP_OnUseFinished(Character, false);
	}
}

void ACTPhoneBooth::FinishUse(double Now)
{
	ACTCharacter* Character = CurrentUser.Get();
	CurrentUser.Reset();
	if (!Character) { return; }

	Character->SetBoothLocked(false);
	WaitForExit.Add(Character);
	CooldownEndTime = Now + BoothCooldownSeconds;

	ACTPlayerState* MyState = Character->GetPlayerState<ACTPlayerState>();
	const ACTGameState* CTGameState = GetWorld()->GetGameState<ACTGameState>();

	// 내 목표 = 내 목표 색을 배정받은 (유령 아닌) 사람
	ACTCharacter* Target = nullptr;
	if (MyState && CTGameState)
	{
		for (APlayerState* OtherState : CTGameState->PlayerArray)
		{
			const ACTPlayerState* Other = Cast<ACTPlayerState>(OtherState);
			if (Other && Other != MyState && !Other->IsGhost() && Other->GetAssignedColor() == MyState->GetTargetColor())
			{
				Target = Cast<ACTCharacter>(Other->GetPawn());
				break;
			}
		}
	}

	if (!Target || !MyState)
	{
		// 횟수는 쓰지 않음
		Character->ClientShowNotice(NSLOCTEXT("CT", "Phone_NoTarget", "목표를 찾을 수 없습니다."), NoticeSeconds);
		BP_OnUseFinished(Character, false);
		return;
	}

	MyState->ConsumePhoneUse();
	const int32 UsesLeft = MyState->GetPhoneUsesLeft();

	// 거리는 구간으로만 알려 줌 (방향 없음)
	const float DistanceMeters = FVector::Dist(Character->GetActorLocation(), Target->GetActorLocation()) / 100.f;
	const int32 Step = FMath::Max(FMath::RoundToInt(DistanceStepMeters), 1);
	const int32 Rounded = FMath::RoundToInt(DistanceMeters / Step) * Step;

	const FText Result = Rounded < Step
		? FText::Format(NSLOCTEXT("CT", "Phone_ResultNear", "목표가 {0}m 이내에 있습니다. (남은 횟수 {1})"), FText::AsNumber(Step), FText::AsNumber(UsesLeft))
		: FText::Format(NSLOCTEXT("CT", "Phone_Result", "목표가 약 {0}m 거리에 있습니다. (남은 횟수 {1})"), FText::AsNumber(Rounded), FText::AsNumber(UsesLeft));

	Character->ClientShowNotice(Result, NoticeSeconds);
	Target->ClientShowNotice(NSLOCTEXT("CT", "Phone_Found", "누군가 당신을 찾았습니다!"), NoticeSeconds);
	BP_OnUseFinished(Character, true);
}
