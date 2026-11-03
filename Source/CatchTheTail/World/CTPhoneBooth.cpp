// Copyright Moon Party Games. All Rights Reserved.
#include "CTPhoneBooth.h"
#include "../Core/CTGameMode.h"
#include "../Core/CTGameState.h"
#include "../Player/CTCharacter.h"
#include "../Player/CTPlayerController.h"
#include "../Player/CTPlayerState.h"
#include "../UI/CTHUDWidget.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// 통화 진행 확인 간격(초)
	constexpr float ServerCheckInterval = 0.1f;

	// 설정을 못 읽었을 때 쓰는 쿨타임(초)
	constexpr float FallbackCooldownSeconds = 30.f;
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

	// 임시 모양
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		Mesh->SetStaticMesh(CubeFinder.Object);
	}
}

void ACTPhoneBooth::BeginPlay()
{
	Super::BeginPlay();

	// 들어온 사람 화면에 사용법 안내 (모든 화면에서 겹침이 계산되므로 조종하는 쪽만 반응)
	Trigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &ACTPhoneBooth::HandleBeginOverlap);

	// 임시 모양일 때만 빨강
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
		ServerTick();
	}
}

bool ACTPhoneBooth::IsCharacterInside(const ACTCharacter* Character) const
{
	return Character && Trigger->IsOverlappingActor(Character);
}

ACTPhoneBooth* ACTPhoneBooth::FindBoothContaining(const ACTCharacter* Character)
{
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!World) { return nullptr; }

	for (TActorIterator<ACTPhoneBooth> It(World); It; ++It)
	{
		if (It->IsCharacterInside(Character)) { return *It; }
	}
	return nullptr;
}

bool ACTPhoneBooth::CanUsePhone(const ACTCharacter* Character)
{
	const UWorld* World = Character ? Character->GetWorld() : nullptr;
	const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	const ACTPlayerState* PlayerState = Character ? Character->GetPlayerState<ACTPlayerState>() : nullptr;

	return CTGameState && PlayerState
		&& CTGameState->GetPhase() == ECTGamePhase::Playing
		&& !PlayerState->IsGhost()
		&& !PlayerState->IsForfeited()
		&& PlayerState->GetAssignedColor() != ECTColor::None;
}

void ACTPhoneBooth::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const ACTCharacter* Character = Cast<ACTCharacter>(OtherActor);
	if (!Character || !Character->IsLocallyControlled() || !CanUsePhone(Character)) { return; }

	const ACTPlayerController* Controller = Cast<ACTPlayerController>(Character->GetController());
	if (UCTHUDWidget* HUD = Controller ? Controller->GetHUDWidget() : nullptr)
	{
		HUD->ShowNotice(NSLOCTEXT("CT", "Phone_EnterPrompt", "F키로 전화 걸기"), EnterPromptSeconds);
	}
}

void ACTPhoneBooth::ServerTick()
{
	ACTCharacter* Current = CurrentUser.Get();
	if (!Current)
	{
		CurrentUser.Reset();
		return;
	}

	// 나가거나 유령이 되면 취소, 끝나면 결과
	if (!IsCharacterInside(Current) || !CanUsePhone(Current)) { CancelUse(); }
	else if (GetWorld()->GetTimeSeconds() >= UseEndTime) { FinishUse(); }
}

void ACTPhoneBooth::ServerRequestCall(ACTCharacter* Character, ECTPhoneQuery Query)
{
	UWorld* World = GetWorld();
	const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	ACTPlayerState* PlayerState = Character ? Character->GetPlayerState<ACTPlayerState>() : nullptr;
	if (!HasAuthority() || !CTGameState || !PlayerState) { return; }

	// 서버 재검사: 부스 안, 경기 중, 이미 통화 중이 아님
	if (!IsCharacterInside(Character) || !CanUsePhone(Character) || Character->IsBoothLocked()) { return; }

	const ACTGameMode* GameMode = World->GetAuthGameMode<ACTGameMode>();
	const float CooldownSeconds = GameMode ? GameMode->GetPhoneCooldownSeconds() : FallbackCooldownSeconds;
	if (CooldownSeconds <= 0.f)
	{
		Character->ClientShowNotice(NSLOCTEXT("CT", "Phone_Disabled", "이번 경기에서는 전화를 쓸 수 없습니다."), NoticeSeconds);
		return;
	}

	if (CurrentUser.IsValid())
	{
		Character->ClientShowNotice(NSLOCTEXT("CT", "Phone_Busy", "다른 사람이 통화 중입니다."), NoticeSeconds);
		return;
	}

	const double CooldownRemaining = PlayerState->GetPhoneReadyServerTime() - CTGameState->GetServerWorldTimeSeconds();
	if (CooldownRemaining > 0.0)
	{
		Character->ClientShowNotice(FText::Format(NSLOCTEXT("CT", "Phone_Cooldown", "아직 전화를 걸 수 없습니다. ({0}초)"),
			FText::AsNumber(FMath::CeilToInt(CooldownRemaining))), NoticeSeconds);
		return;
	}

	CurrentUser = Character;
	CurrentQuery = Query;
	UseEndTime = World->GetTimeSeconds() + UseDurationSeconds;
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

void ACTPhoneBooth::FinishUse()
{
	ACTCharacter* Character = CurrentUser.Get();
	CurrentUser.Reset();
	if (!Character) { return; }

	Character->SetBoothLocked(false);

	ACTPlayerState* MyState = Character->GetPlayerState<ACTPlayerState>();
	const ACTGameState* CTGameState = GetWorld()->GetGameState<ACTGameState>();
	const bool bAskTarget = CurrentQuery == ECTPhoneQuery::Target;

	// 알아낼 사람: 목표 = 목표 색을 가진 사람, 헌터 = 내 색을 목표로 가진 사람 (유령 제외)
	ACTCharacter* Found = nullptr;
	if (MyState && CTGameState)
	{
		for (APlayerState* OtherState : CTGameState->PlayerArray)
		{
			const ACTPlayerState* Other = Cast<ACTPlayerState>(OtherState);
			if (!Other || Other == MyState || Other->IsGhost()) { continue; }

			const bool bMatches = bAskTarget
				? Other->GetAssignedColor() == MyState->GetTargetColor()
				: Other->GetTargetColor() == MyState->GetAssignedColor();
			if (bMatches)
			{
				Found = Cast<ACTCharacter>(Other->GetPawn());
				if (Found) { break; }
			}
		}
	}

	if (!Found || !MyState)
	{
		// 쿨타임 소모 없음
		Character->ClientShowNotice(bAskTarget
			? NSLOCTEXT("CT", "Phone_NoTarget", "목표를 찾을 수 없습니다.")
			: NSLOCTEXT("CT", "Phone_NoHunter", "헌터를 찾을 수 없습니다."), NoticeSeconds);
		BP_OnUseFinished(Character, false);
		return;
	}

	const ACTGameMode* GameMode = GetWorld()->GetAuthGameMode<ACTGameMode>();
	const float CooldownSeconds = GameMode ? GameMode->GetPhoneCooldownSeconds() : FallbackCooldownSeconds;
	MyState->SetPhoneReadyServerTime(CTGameState->GetServerWorldTimeSeconds() + CooldownSeconds);

	// 거리는 구간으로만, 방향은 화살표로 알려줌 (목표, 헌터 모두)
	const float DistanceMeters = FVector::Dist(Character->GetActorLocation(), Found->GetActorLocation()) / 100.f;
	const int32 Step = FMath::Max(FMath::RoundToInt(DistanceStepMeters), 1);
	const int32 Rounded = FMath::RoundToInt(DistanceMeters / Step) * Step;
	const bool bNear = Rounded < Step;

	FText Result;
	if (bAskTarget)
	{
		Result = bNear
			? FText::Format(NSLOCTEXT("CT", "Phone_ResultNear", "목표가 {0}m 이내에 있습니다."), FText::AsNumber(Step))
			: FText::Format(NSLOCTEXT("CT", "Phone_Result", "목표가 약 {0}m 거리에 있습니다."), FText::AsNumber(Rounded));
	}
	else
	{
		Result = bNear
			? FText::Format(NSLOCTEXT("CT", "Phone_HunterNear", "헌터가 {0}m 이내에 있습니다."), FText::AsNumber(Step))
			: FText::Format(NSLOCTEXT("CT", "Phone_HunterResult", "헌터가 약 {0}m 거리에 있습니다."), FText::AsNumber(Rounded));
	}

	Character->ClientShowPhoneResult(Result, NoticeSeconds, /*bShowArrow=*/true, Found->GetActorLocation());
	if (bAskTarget)
	{
		Found->ClientShowNotice(NSLOCTEXT("CT", "Phone_Found", "누군가 당신을 찾았습니다!"), NoticeSeconds);
	}
	BP_OnUseFinished(Character, true);
}
