// Copyright Moon Party Games. All Rights Reserved.
#include "CTCharacter.h"
#include "../Core/CTGameMode.h"
#include "../Core/CTGameState.h"
#include "CTPlayerState.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
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

	// 카메라 방향으로 몸을 돌리지 않고, "움직이는 방향"으로 몸을 돌립니다. (3인칭 추적 방식)
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

	// 3인칭 추적 카메라
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 450.f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 60.f);
	CameraBoom->bUsePawnControlRotation = true; // 마우스로 카메라 회전
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 15.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void ACTCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACTCharacter, SpeedBuffs);
	DOREPLIFETIME(ACTCharacter, RunState);
	DOREPLIFETIME(ACTCharacter, RunStateEndTime);
}

void ACTCharacter::BeginPlay()
{
	Super::BeginPlay();

	RecalculateMaxWalkSpeed();

	if (HasAuthority())
	{
		RefreshChaseBuff();
	}
}

void ACTCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

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
	if (LookAction)
	{
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACTCharacter::Look);
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

void ACTCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MoveValue = Value.Get<FVector2D>();

	if (Controller)
	{
		// 카메라가 보는 방향 기준으로 앞/옆을 계산합니다. (유령도 이동 가능)
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(Forward, MoveValue.Y);
		AddMovementInput(Right, MoveValue.X);
	}
}

void ACTCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookValue = Value.Get<FVector2D>();

	AddControllerYawInput(LookValue.X);
	AddControllerPitchInput(LookValue.Y);
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

	bool bSuccess = false;
	const ECTColor MyTargetColor = MyPlayerState->GetTargetColor();

	// 정해진 목표 색을 가진 사람만 잡을 수 있습니다.
	if (MyTargetColor != ECTColor::None)
	{
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
				bSuccess = CTGameMode->HandleTailGrabbed(MyPlayerState, OtherPlayerState);
				break; // 목표 색은 한 명뿐이므로 더 찾지 않습니다.
			}
		}
	}

	// 손을 뻗는 연출은 잡기에 성공하든 못하든 재생합니다. (성공 여부는 이펙트용)
	MulticastPlayAction(ECTCharacterAction::Grab, bSuccess);
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
