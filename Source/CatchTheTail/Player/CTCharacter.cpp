// Copyright Moon Party Games. All Rights Reserved.
#include "CTCharacter.h"
#include "../Core/CTGameMode.h"
#include "../Core/CTGameState.h"
#include "CTPlayerState.h"
#include "../Tail/CTTail.h"
#include "../Tongue/CTTongueComponent.h"
#include "../Animation/CTAnimNotifyState_GrabWindow.h"
#include "EngineUtils.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

namespace CTBuffNames
{
	static const FName Run(TEXT("Run"));
	static const FName Chase(TEXT("Chase"));
	static const FName Ghost(TEXT("Ghost"));
}

ACTCharacter::ACTCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// 몸은 "움직이는 방향"으로 돌립니다. (탑다운 시점에서는 카메라가 고정이라 컨트롤러 회전을 쓰지 않음)
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 공중에서 또 점프하는 것을 막습니다. (점프 최대 횟수 1)
	JumpMaxCount = 1;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = true;
	MoveComp->RotationRate = FRotator(0.f, 540.f, 0.f);
	MoveComp->JumpZVelocity = 700.f;
	MoveComp->AirControl = 0.35f;
	MoveComp->MaxWalkSpeed = BaseWalkSpeed;
	MoveComp->BrakingDecelerationWalking = 2000.f;

	// 서버의 캐릭터 메시도 화면 밖에서 몽타주(잡기 노티파이)를 계속 진행해야 합니다.
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;

	// 탑다운 카메라 (브롤스타즈 스타일): 각도를 고정하고 캐릭터 위치만 따라갑니다.
	// 실제 각도, 거리, 시야각은 BeginPlay의 ApplyCameraSettings에서 CameraPitch 등으로 적용합니다.
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // 캐릭터가 돌아도 카메라는 돌지 않음
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bUsePawnControlRotation = false; // 마우스로 카메라를 돌리지 않음
	CameraBoom->bDoCollisionTest = false; // 벽 때문에 카메라가 당겨지지 않게
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// 혀: 메시의 Head 본에 붙입니다. (메시와 위치 보정은 컴포넌트가 처리)
	TongueMesh = CreateDefaultSubobject<UCTTongueComponent>(TEXT("TongueMesh"));
	TongueMesh->SetupAttachment(GetMesh(), TEXT("Head"));
}

void ACTCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACTCharacter, SpeedBuffs);
	DOREPLIFETIME(ACTCharacter, RunState);
	DOREPLIFETIME(ACTCharacter, RunStateEndTime);
	DOREPLIFETIME(ACTCharacter, OwnedTail);
}

void ACTCharacter::BeginPlay()
{
	Super::BeginPlay();

	ApplyCameraSettings();

	if (HasAuthority())
	{
		SpawnTail();
	}

	RefreshBodyColor();

	RecalculateMaxWalkSpeed();

	if (HasAuthority())
	{
		RefreshChaseBuff();
	}
}

void ACTCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	RefreshBodyColor();

	if (HasAuthority())
	{
		RefreshChaseBuff();
	}
}

bool ACTCharacter::CanJumpInternal_Implementation() const
{
	// 유령은 점프할 수 없습니다.
	if (IsGhost())
	{
		return false;
	}
	return Super::CanJumpInternal_Implementation();
}

void ACTCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	RefreshBodyColor();

	// PlayerState의 유령 알림이 캐릭터보다 먼저 와서 놓쳤을 수 있으므로 여기서 한 번 더 맞춥니다.
	if (IsGhost())
	{
		OnGhostStateChanged(true);
	}
}

// ============================================================
// 상태 읽기
// ============================================================

bool ACTCharacter::IsGhost() const
{
	const ACTPlayerState* CTPlayerState = GetPlayerState<ACTPlayerState>();
	return CTPlayerState && CTPlayerState->IsGhost();
}

float ACTCharacter::GetSpeedMultiplier() const
{
	// 버프 합산: 1.0(기본) + 달리기 0.5 + 추격 0.2 = 1.7
	float Multiplier = 1.f;
	for (const FCTSpeedBuff& Buff : SpeedBuffs)
	{
		Multiplier += Buff.Value;
	}
	return FMath::Max(0.f, Multiplier);
}

float ACTCharacter::GetRunStateRemainingSeconds() const
{
	if (RunState == ECTRunState::Idle)
	{
		return 0.f;
	}
	return static_cast<float>(FMath::Max(0.0, RunStateEndTime - GetServerTime()));
}

float ACTCharacter::GetRunGaugePercent() const
{
	switch (RunState)
	{
	case ECTRunState::Running:
		return RunDurationSeconds > 0.f ? FMath::Clamp(GetRunStateRemainingSeconds() / RunDurationSeconds, 0.f, 1.f) : 0.f;
	case ECTRunState::Cooldown:
		return RunCooldownSeconds > 0.f ? FMath::Clamp(1.f - GetRunStateRemainingSeconds() / RunCooldownSeconds, 0.f, 1.f) : 1.f;
	default:
		return 1.f;
	}
}

float ACTCharacter::GetPushReadyPercent() const
{
	if (PushCooldownSeconds <= 0.f || !GetWorld()) { return 1.f; }

	const double Elapsed = GetWorld()->GetTimeSeconds() - LocalLastPushTime;
	return FMath::Clamp(static_cast<float>(Elapsed / PushCooldownSeconds), 0.f, 1.f);
}

bool ACTCharacter::CanUseActions() const
{
	if (IsGhost()) { return false; }

	const ACTGameState* CTGameState = GetWorld() ? GetWorld()->GetGameState<ACTGameState>() : nullptr;
	return CTGameState && CTGameState->GetPhase() == ECTGamePhase::Playing;
}

UAnimMontage* ACTCharacter::GetActionMontage(ECTCharacterAction Action) const
{
	switch (Action)
	{
	case ECTCharacterAction::Grab:   return GrabMontage;
	case ECTCharacterAction::Push:   return PushMontage;
	case ECTCharacterAction::Caught: return CaughtMontage;
	case ECTCharacterAction::Pushed: return PushedMontage;
	default:                         return nullptr;
	}
}

// ============================================================
// 속도 버프
// ============================================================

void ACTCharacter::AddSpeedBuff(FName BuffId, float Value)
{
	if (!HasAuthority()) { return; }

	if (FCTSpeedBuff* Existing = SpeedBuffs.FindByPredicate(
		[BuffId](const FCTSpeedBuff& Buff) { return Buff.Id == BuffId; }))
	{
		Existing->Value = Value;
	}
	else
	{
		FCTSpeedBuff NewBuff;
		NewBuff.Id = BuffId;
		NewBuff.Value = Value;
		SpeedBuffs.Add(NewBuff);
	}

	RecalculateMaxWalkSpeed();
}

void ACTCharacter::RemoveSpeedBuff(FName BuffId)
{
	if (!HasAuthority()) { return; }

	const int32 RemovedCount = SpeedBuffs.RemoveAll(
		[BuffId](const FCTSpeedBuff& Buff) { return Buff.Id == BuffId; });

	if (RemovedCount > 0)
	{
		RecalculateMaxWalkSpeed();
	}
}

void ACTCharacter::RecalculateMaxWalkSpeed()
{
	// 참고: 서버와 클라이언트가 같은 값을 써야 끊김(보정)이 적습니다.
	// 그래서 버프 목록을 복제하고, 양쪽에서 같은 계산을 합니다.
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * GetSpeedMultiplier();
}

void ACTCharacter::OnRep_SpeedBuffs()
{
	RecalculateMaxWalkSpeed();
}

void ACTCharacter::RefreshChaseBuff()
{
	if (!HasAuthority()) { return; }

	const ACTGameState* CTGameState = GetWorld() ? GetWorld()->GetGameState<ACTGameState>() : nullptr;
	const bool bShouldHaveChaseBuff =
		CTGameState && CTGameState->GetPhase() == ECTGamePhase::Playing && !IsGhost();

	// 지금은 "게임 중이고 유령이 아니면 항상" 추격 버프를 켭니다.
	// (나중에 "목표가 가까울 때만" 같은 조건이 필요하면 이 함수만 바꾸면 됩니다)
	if (bShouldHaveChaseBuff)
	{
		AddSpeedBuff(CTBuffNames::Chase, ChaseBuffValue);
	}
	else
	{
		RemoveSpeedBuff(CTBuffNames::Chase);
	}
}

void ACTCharacter::OnGhostStateChanged(bool bNewIsGhost)
{
	if (HasAuthority())
	{
		if (bNewIsGhost)
		{
			ForceResetRun();
			AddSpeedBuff(CTBuffNames::Ghost, GhostBuffValue);
		}
		else
		{
			RemoveSpeedBuff(CTBuffNames::Ghost);
		}
		RefreshChaseBuff();

		// 잡혀서 유령이 되면 꼬리 불이 꺼지고 회색이 됩니다. 부활하면 다시 켜집니다.
		if (OwnedTail)
		{
			OwnedTail->SetCaught(bNewIsGhost);
		}
		if (bNewIsGhost)
		{
			EndGrabWindow();
		}
	}

	if (bNewIsGhost)
	{
		StopJumping();
		// 잡힘 몽타주는 그대로 두고, 하던 잡기/밀기 동작만 멈춥니다.
		StopAttackMontages();
	}

	BP_OnGhostStateChanged(bNewIsGhost);
}

// ============================================================
// 입력 연결
// ============================================================

void ACTCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CT] EnhancedInputComponent가 아닙니다. Project Settings > Input > Default Input Component Class를 확인하세요."));
		return;
	}

	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACTCharacter::Move);
	}
	if (JumpAction)
	{
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	}
	if (RunAction)
	{
		EnhancedInput->BindAction(RunAction, ETriggerEvent::Started, this, &ACTCharacter::OnRunPressed);
		EnhancedInput->BindAction(RunAction, ETriggerEvent::Completed, this, &ACTCharacter::OnRunReleased);
		EnhancedInput->BindAction(RunAction, ETriggerEvent::Canceled, this, &ACTCharacter::OnRunReleased);
	}
	if (GrabAction)
	{
		EnhancedInput->BindAction(GrabAction, ETriggerEvent::Started, this, &ACTCharacter::OnGrabPressed);
	}
	if (PushAction)
	{
		EnhancedInput->BindAction(PushAction, ETriggerEvent::Started, this, &ACTCharacter::OnPushPressed);
	}
}

void ACTCharacter::ApplyCameraSettings()
{
	if (!CameraBoom || !FollowCamera) { return; }

	// 각도는 월드 기준으로 고정. 위에서 비스듬히 내려다보는 시점
	CameraBoom->SetWorldRotation(FRotator(CameraPitch, CameraYaw, 0.f));
	CameraBoom->TargetArmLength = CameraDistance;
	CameraBoom->SocketOffset = FVector::ZeroVector;
	FollowCamera->SetFieldOfView(CameraFieldOfView);
}

void ACTCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MoveValue = Value.Get<FVector2D>();

	if (Controller)
	{
		// 고정 카메라의 방향 기준으로 위/오른쪽을 계산합니다. (화면 위쪽 = 앞, 유령도 이동 가능)
		const FRotator YawRotation(0.f, CameraYaw, 0.f);
		const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(Forward, MoveValue.Y);
		AddMovementInput(Right, MoveValue.X);
	}
}

void ACTCharacter::OnRunPressed()
{
	if (IsGhost()) { return; }
	ServerSetRunInput(true);
}

void ACTCharacter::OnRunReleased()
{
	ServerSetRunInput(false);
}

void ACTCharacter::OnGrabPressed()
{
	if (!CanUseActions()) { return; }

	// 서버 응답을 기다리지 않고 바로 손을 뻗어서 조작감이 늦지 않게 합니다.
	PlayActionMontage(ECTCharacterAction::Grab);
	ServerGrab();
}

void ACTCharacter::OnPushPressed()
{
	if (!CanUseActions()) { return; }

	// 쿨타임 중에는 서버도 거절하므로 동작을 재생하지 않습니다.
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LocalLastPushTime < PushCooldownSeconds) { return; }
	LocalLastPushTime = Now;

	PlayActionMontage(ECTCharacterAction::Push);
	ServerPush();
}

// ============================================================
// 달리기 (서버)
// ============================================================

void ACTCharacter::ServerSetRunInput_Implementation(bool bPressed)
{
	if (bPressed)
	{
		TryStartRun();
	}
	else
	{
		StopRun(/*bEndedNaturally=*/false);
	}
}

void ACTCharacter::TryStartRun()
{
	if (IsGhost() || RunState != ECTRunState::Idle)
	{
		return;
	}

	AddSpeedBuff(CTBuffNames::Run, RunBuffValue);

	RunState = ECTRunState::Running;
	RunStateEndTime = GetServerTime() + RunDurationSeconds;

	GetWorldTimerManager().SetTimer(
		RunTimerHandle, this, &ACTCharacter::OnRunDurationEnded,
		FMath::Max(RunDurationSeconds, 0.01f), false);
}

void ACTCharacter::StopRun(bool bEndedNaturally)
{
	if (RunState != ECTRunState::Running)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(RunTimerHandle);
	RemoveSpeedBuff(CTBuffNames::Run);

	if (bEndedNaturally || bCooldownOnEarlyRelease)
	{
		RunState = ECTRunState::Cooldown;
		RunStateEndTime = GetServerTime() + RunCooldownSeconds;

		GetWorldTimerManager().SetTimer(
			RunTimerHandle, this, &ACTCharacter::OnRunCooldownEnded,
			FMath::Max(RunCooldownSeconds, 0.01f), false);
	}
	else
	{
		RunState = ECTRunState::Idle;
		RunStateEndTime = 0.0;
	}
}

void ACTCharacter::ForceResetRun()
{
	GetWorldTimerManager().ClearTimer(RunTimerHandle);
	RemoveSpeedBuff(CTBuffNames::Run);
	RunState = ECTRunState::Idle;
	RunStateEndTime = 0.0;
}

void ACTCharacter::OnRunDurationEnded()
{
	StopRun(/*bEndedNaturally=*/true);
}

void ACTCharacter::OnRunCooldownEnded()
{
	RunState = ECTRunState::Idle;
	RunStateEndTime = 0.0;
}

// ============================================================
// 잡기 / 밀기 (서버)
// ============================================================

void ACTCharacter::ServerGrab_Implementation()
{
	UWorld* World = GetWorld();
	ACTGameMode* CTGameMode = World ? World->GetAuthGameMode<ACTGameMode>() : nullptr;
	const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	ACTPlayerState* MyPlayerState = GetPlayerState<ACTPlayerState>();

	if (!CTGameMode || !CTGameState || !MyPlayerState) { return; }
	if (MyPlayerState->IsGhost()) { return; }
	if (CTGameState->GetPhase() != ECTGamePhase::Playing) { return; }

	// 이미 잡기 동작 중이면 새로 시작하지 않습니다. (버튼 연타로 구간이 계속 늘어나는 것 방지)
	const double Now = World->GetTimeSeconds();
	if (bGrabActive || Now < GrabLockEndTime || GetWorldTimerManager().IsTimerActive(GrabWindowStartTimerHandle)) { return; }

	bool bSuccess = false;

	if (bRequireTailTouch)
	{
		// 꼬리 터치 방식: 잡기 동작 구간 동안 상대 꼬리에 닿으면 잡힙니다.
		// 성공 여부는 아직 모르므로 bSuccess는 false로 보내고, 잡힌 사람 쪽에 Caught 연출이 나옵니다.
		if (bUseMontageGrabWindow && HasGrabWindowNotify())
		{
			// 몽타주의 CT Grab Window 구간에서 판정이 켜지고 꺼집니다. (아래 MulticastPlayAction이 서버에서도 몽타주를 재생)
			GrabLockEndTime = Now + GrabMontage->GetPlayLength() / FMath::Max(ActionMontagePlayRate, 0.1f);
		}
		else if (GrabWindowDelaySeconds > 0.f)
		{
			GrabLockEndTime = Now + GrabWindowDelaySeconds + GrabActiveSeconds;
			GetWorldTimerManager().SetTimer(
				GrabWindowStartTimerHandle, this, &ACTCharacter::BeginGrabWindow, GrabWindowDelaySeconds, false);
		}
		else
		{
			BeginGrabWindow();
		}
	}
	else
	{
		bSuccess = TryGrabByCone();
	}

	// 손을 뻗는 연출은 잡기에 성공하든 못하든 재생합니다.
	MulticastPlayAction(ECTCharacterAction::Grab, bSuccess);
}

bool ACTCharacter::TryGrabByCone()
{
	UWorld* World = GetWorld();
	ACTGameMode* CTGameMode = World ? World->GetAuthGameMode<ACTGameMode>() : nullptr;
	const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	ACTPlayerState* MyPlayerState = GetPlayerState<ACTPlayerState>();
	if (!CTGameMode || !CTGameState || !MyPlayerState) { return false; }

	const ECTColor MyTargetColor = MyPlayerState->GetTargetColor();
	if (MyTargetColor == ECTColor::None) { return false; }

	// 정해진 목표 색을 가진 사람만 잡을 수 있습니다.
	for (APlayerState* OtherState : CTGameState->PlayerArray)
	{
		ACTPlayerState* OtherPlayerState = Cast<ACTPlayerState>(OtherState);
		if (!OtherPlayerState || OtherPlayerState == MyPlayerState) { continue; }
		if (OtherPlayerState->GetAssignedColor() != MyTargetColor) { continue; }
		if (OtherPlayerState->IsGhost()) { continue; }

		const ACTCharacter* OtherCharacter = Cast<ACTCharacter>(OtherPlayerState->GetPawn());
		if (!OtherCharacter) { continue; }

		if (IsInFrontCone(OtherCharacter, GrabDistance, GrabHalfAngleDegrees))
		{
			// 잡힌 사람의 반응 연출은 GameMode가 성공 처리와 함께 재생합니다.
			return CTGameMode->HandleTailGrabbed(MyPlayerState, OtherPlayerState);
		}
	}
	return false;
}

// ============================================================
// 꼬리 / 잡기 동작 구간 (서버)
// ============================================================

void ACTCharacter::SpawnTail()
{
	if (!HasAuthority() || !TailClass || OwnedTail) { return; }

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	OwnedTail = GetWorld()->SpawnActor<ACTTail>(TailClass, GetActorTransform(), SpawnParams);
	if (OwnedTail)
	{
		OwnedTail->AttachToComponent(
			GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TailSocketName);

		// 붙은 본의 기준 자세(컴포넌트 공간)를 구해서, 기본 자세에서 TailOffset 위치에 놓이도록 상대 변환을 정합니다.
		// 이후 본이 움직이면 꼬리가 그대로 따라갑니다.
		const USkeletalMesh* MeshAsset = GetMesh() ? GetMesh()->GetSkeletalMeshAsset() : nullptr;
		const int32 BoneIndex = MeshAsset ? MeshAsset->GetRefSkeleton().FindBoneIndex(TailSocketName) : INDEX_NONE;
		if (BoneIndex != INDEX_NONE)
		{
			const FReferenceSkeleton& RefSkeleton = MeshAsset->GetRefSkeleton();
			const TArray<FTransform>& RefPose = RefSkeleton.GetRefBonePose();
			FTransform BoneComponentTransform = FTransform::Identity;
			for (int32 Index = BoneIndex; Index != INDEX_NONE; Index = RefSkeleton.GetParentIndex(Index))
			{
				BoneComponentTransform = BoneComponentTransform * RefPose[Index];
			}

			const FTransform Offset(TailOffsetRotation, TailOffsetLocation, TailOffsetScale);
			OwnedTail->SetActorRelativeTransform(Offset * BoneComponentTransform.Inverse());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[CT] 꼬리를 붙일 본 '%s'을(를) 캐릭터 메시에서 찾지 못했습니다."), *TailSocketName.ToString());
		}
		OwnedTail->RefreshVisual();
	}
}

void ACTCharacter::OnRep_OwnedTail()
{
	// 클라이언트: 꼬리가 도착하면 현재 색을 바로 입혀 줍니다.
	RefreshBodyColor();
}

void ACTCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(GrabWindowStartTimerHandle);
	GetWorldTimerManager().ClearTimer(GrabWindowTickTimerHandle);

	if (HasAuthority() && OwnedTail)
	{
		OwnedTail->Destroy();
		OwnedTail = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ACTCharacter::BeginGrabWindow()
{
	StartGrabWindow(GrabActiveSeconds);
}

void ACTCharacter::StartGrabWindow(float Duration)
{
	if (!HasAuthority()) { return; }

	bGrabActive = true;
	GrabWindowEndTime = GetWorld()->GetTimeSeconds() + Duration;

	GetWorldTimerManager().SetTimer(GrabWindowTickTimerHandle, this, &ACTCharacter::TickGrabWindow, 0.03f, true);
	TickGrabWindow(); // 시작하자마자 이미 닿아 있는 꼬리도 확인
}

bool ACTCharacter::HasGrabWindowNotify() const
{
	if (!GrabMontage) { return false; }

	for (const FAnimNotifyEvent& NotifyEvent : GrabMontage->Notifies)
	{
		if (NotifyEvent.NotifyStateClass && NotifyEvent.NotifyStateClass->IsA<UCTAnimNotifyState_GrabWindow>())
		{
			return true;
		}
	}
	return false;
}

void ACTCharacter::OnGrabNotifyBegin()
{
	// 몽타주는 모든 컴퓨터에서 재생되지만, 판정은 서버만 합니다.
	if (!HasAuthority() || !bRequireTailTouch || !CanUseActions()) { return; }

	// 구간의 끝은 OnGrabNotifyEnd가 알려 줍니다. 최대 시간은 안전장치입니다.
	StartGrabWindow(GrabWindowMaxSeconds);
}

void ACTCharacter::OnGrabNotifyEnd()
{
	if (!HasAuthority()) { return; }
	EndGrabWindow();
}

void ACTCharacter::EndGrabWindow()
{
	bGrabActive = false;
	GetWorldTimerManager().ClearTimer(GrabWindowStartTimerHandle);
	GetWorldTimerManager().ClearTimer(GrabWindowTickTimerHandle);
}

void ACTCharacter::TickGrabWindow()
{
	UWorld* World = GetWorld();
	const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;

	if (!bGrabActive || IsGhost() || !CTGameState ||
		CTGameState->GetPhase() != ECTGamePhase::Playing ||
		World->GetTimeSeconds() > GrabWindowEndTime)
	{
		EndGrabWindow();
		return;
	}

	// 닿은 꼬리가 있으면 잡기를 시도합니다. 한 번 성공하면 동작 구간을 끝냅니다.
	for (TActorIterator<ACTTail> It(World); It; ++It)
	{
		if (It->TryTouch(this))
		{
			EndGrabWindow();
			return;
		}
	}
}

void ACTCharacter::ServerPush_Implementation()
{
	UWorld* World = GetWorld();
	const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	const ACTPlayerState* MyPlayerState = GetPlayerState<ACTPlayerState>();

	if (!CTGameState || !MyPlayerState) { return; }
	if (MyPlayerState->IsGhost()) { return; }
	if (CTGameState->GetPhase() != ECTGamePhase::Playing) { return; }

	// 밀기 쿨타임 확인
	const double Now = World->GetTimeSeconds();
	if (Now - LastPushTime < PushCooldownSeconds) { return; }
	LastPushTime = Now;

	MulticastPlayAction(ECTCharacterAction::Push, true);

	// 정면 부채꼴 안의 모든 상대를 밉니다. (유령은 밀 수 없음)
	for (APlayerState* OtherState : CTGameState->PlayerArray)
	{
		ACTCharacter* OtherCharacter = OtherState ? Cast<ACTCharacter>(OtherState->GetPawn()) : nullptr;
		if (!OtherCharacter || OtherCharacter == this) { continue; }
		if (OtherCharacter->IsGhost()) { continue; }
		if (!IsInFrontCone(OtherCharacter, PushDistance, PushHalfAngleDegrees)) { continue; }

		FVector Direction = OtherCharacter->GetActorLocation() - GetActorLocation();
		Direction.Z = 0.f;
		Direction = Direction.GetSafeNormal();

		const FVector LaunchVelocity = Direction * PushSpeed + FVector(0.f, 0.f, PushUpSpeed);
		OtherCharacter->LaunchCharacter(LaunchVelocity, /*bXYOverride=*/true, /*bZOverride=*/true);
		OtherCharacter->MulticastPlayAction(ECTCharacterAction::Pushed, true);
	}
}

// ============================================================
// 애니메이션
// ============================================================

void ACTCharacter::MulticastPlayAction_Implementation(ECTCharacterAction Action, bool bSuccess)
{
	// 내가 누른 잡기/밀기는 OnGrabPressed / OnPushPressed에서 이미 재생했습니다.
	const bool bAlreadyPlayedLocally =
		IsLocallyControlled() &&
		(Action == ECTCharacterAction::Grab || Action == ECTCharacterAction::Push);

	if (!bAlreadyPlayedLocally)
	{
		PlayActionMontage(Action);
	}

	BP_OnAction(Action, bSuccess);
}

void ACTCharacter::PlayActionMontage(ECTCharacterAction Action)
{
	if (UAnimMontage* Montage = GetActionMontage(Action))
	{
		PlayAnimMontage(Montage, ActionMontagePlayRate);
	}
}

void ACTCharacter::StopAttackMontages()
{
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance) { return; }

	for (UAnimMontage* Montage : { GrabMontage.Get(), PushMontage.Get() })
	{
		if (Montage && AnimInstance->Montage_IsPlaying(Montage))
		{
			AnimInstance->Montage_Stop(Montage->BlendOut.GetBlendTime(), Montage);
		}
	}
}

// ============================================================
// 도우미
// ============================================================

double ACTCharacter::GetServerTime() const
{
	const UWorld* World = GetWorld();
	if (!World) { return 0.0; }

	if (const AGameStateBase* GameStateBase = World->GetGameState())
	{
		return GameStateBase->GetServerWorldTimeSeconds();
	}
	return World->GetTimeSeconds();
}

bool ACTCharacter::IsInFrontCone(const AActor* Other, float MaxDistance, float HalfAngleDegrees) const
{
	if (!Other) { return false; }

	FVector ToOther = Other->GetActorLocation() - GetActorLocation();
	ToOther.Z = 0.f; // 높이는 무시하고 바닥 기준으로만 계산

	const float Distance = ToOther.Size();
	if (Distance > MaxDistance) { return false; }
	if (Distance < KINDA_SMALL_NUMBER) { return true; } // 거의 겹쳐 있으면 성공

	FVector Forward = GetActorForwardVector();
	Forward.Z = 0.f;
	Forward = Forward.GetSafeNormal();

	const float CosAngle = FVector::DotProduct(Forward, ToOther / Distance);
	return CosAngle >= FMath::Cos(FMath::DegreesToRadians(HalfAngleDegrees));
}

// ============================================================
// 몸 색상
// ============================================================

void ACTCharacter::RefreshBodyColor()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp) { return; }

	// 슬롯마다 동적 머티리얼을 한 번만 만들어 재사용합니다.
	const int32 SlotCount = MeshComp->GetNumMaterials();
	if (BodyMaterials.Num() != SlotCount)
	{
		BodyMaterials.Reset();
		for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
		{
			const bool bTargetSlot = BodyColorMaterialSlots.IsEmpty() || BodyColorMaterialSlots.Contains(SlotIndex);
			BodyMaterials.Add(bTargetSlot && MeshComp->GetMaterial(SlotIndex)
				? MeshComp->CreateAndSetMaterialInstanceDynamic(SlotIndex)
				: nullptr);
		}
	}

	const ACTPlayerState* CTPlayerState = GetPlayerState<ACTPlayerState>();
	const ECTColor Color = CTPlayerState ? CTPlayerState->GetAssignedColor() : ECTColor::None;
	const FLinearColor Tint = Color == ECTColor::None ? NoColorTint : ACTPlayerState::GetLinearColorOf(Color);

	for (UMaterialInstanceDynamic* Material : BodyMaterials)
	{
		if (Material)
		{
			Material->SetVectorParameterValue(BodyColorParameterName, Tint);
		}
	}

	// 꼬리도 같은 색을 쓰므로 함께 갱신합니다.
	if (OwnedTail)
	{
		OwnedTail->RefreshVisual();
	}
}
