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
#include "Curves/CurveFloat.h"
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
	PrimaryActorTick.bCanEverTick = true; // 내 화면의 카메라 거리 보간에만 사용

	// 카메라 방향으로 몸을 돌리지 않고, "움직이는 방향"으로 몸을 돌립니다. (3인칭 방식)
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

	// 3인칭 자유 카메라: 마우스로 회전하고, 벽에 막히면 스프링암이 플레이어 쪽으로 당겨집니다.
	// 거리는 위/아래 보는 각도에 따라 Tick에서 보간합니다. (UpdateCameraDistance)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = CameraDistanceBehind;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 60.f);
	CameraBoom->bUsePawnControlRotation = true; // 마우스로 카메라 회전
	CameraBoom->bDoCollisionTest = true;        // 벽에 닿으면 카메라가 플레이어 쪽으로 가까워짐
	CameraBoom->ProbeSize = CameraProbeSize;
	CameraBoom->ProbeChannel = ECC_Camera;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 15.f;

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

	// 경기 시작 전(색 배정 전)에는 점프할 수 없습니다.
	if (!CanMoveNow())
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

bool ACTCharacter::CanMoveNow() const
{
	const ACTGameState* CTGameState = GetWorld() ? GetWorld()->GetGameState<ACTGameState>() : nullptr;
	if (!CTGameState || CTGameState->GetPhase() != ECTGamePhase::Playing) { return false; }

	// 색이 배정되기 전에는 움직일 수 없습니다. (경기 시작 직후 색이 정해지는 동안 대기)
	const ACTPlayerState* CTPlayerState = GetPlayerState<ACTPlayerState>();
	return CTPlayerState && CTPlayerState->GetAssignedColor() != ECTColor::None;
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

	// 잡히면 몸이 회색으로, 부활하면 다시 내 색으로
	RefreshBodyColor();

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

void ACTCharacter::ApplyCameraSettings()
{
	if (!CameraBoom || !FollowCamera) { return; }

	// BP에서 바꾼 값을 반영합니다. (생성자 시점에는 아직 BP 값이 적용되기 전)
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeSize = CameraProbeSize;
	CameraBoom->TargetArmLength = CameraDistanceBehind;
	FollowCamera->SetFieldOfView(CameraFieldOfView);
}

void ACTCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateRunMontage();

	// 카메라는 내 화면에서만 필요합니다. (서버에 있는 다른 사람의 캐릭터는 건너뜀)
	if (IsLocallyControlled())
	{
		UpdateCameraDistance(DeltaSeconds);
	}
}

void ACTCharacter::UpdateCameraDistance(float DeltaSeconds)
{
	if (!CameraBoom || !Controller) { return; }

	// 위/아래로 보는 각도. 0 근처 = 수평(뒤에서 봄), 음수가 클수록 = 위에서 내려다봄
	const float Pitch = FRotator::NormalizeAxis(Controller->GetControlRotation().Pitch);

	// BehindPitch → 0, TopPitch → 1 (범위 밖은 0~1로 고정)
	const float Range = BehindPitch - TopPitch;
	float Alpha = Range > KINDA_SMALL_NUMBER ? FMath::Clamp((BehindPitch - Pitch) / Range, 0.f, 1.f) : 0.f;

	// 곡선이 있으면 곡선으로, 없으면 부드러운 S자 곡선으로 보간
	Alpha = DistanceCurve ? FMath::Clamp(DistanceCurve->GetFloatValue(Alpha), 0.f, 1.f) : FMath::SmoothStep(0.f, 1.f, Alpha);

	const float TargetLength = FMath::Lerp(CameraDistanceBehind, CameraDistanceTop, Alpha);
	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetLength, DeltaSeconds, CameraDistanceInterpSpeed);
}

void ACTCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MoveValue = Value.Get<FVector2D>();

	// 경기가 시작되고 색이 배정될 때까지 움직일 수 없습니다. (카메라 회전은 가능)
	if (!CanMoveNow()) { return; }

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
	if (IsGhost() || !CanMoveNow()) { return; }
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

void ACTCharacter::UpdateRunMontage()
{
	// 화면이 없는 전용 서버에서는 연출이 필요 없습니다.
	if (!RunMontage || GetNetMode() == NM_DedicatedServer) { return; }

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance) { return; }

	// 달리기 중 + 이동 중 + 땅 위일 때만 재생합니다. (공중에서는 점프 동작을 보여 줌)
	const bool bWantRunMontage =
		RunState == ECTRunState::Running &&
		!IsGhost() &&
		!GetCharacterMovement()->IsFalling() &&
		GetVelocity().Size2D() > RunMontageMinSpeed;

	const bool bPlaying = AnimInstance->Montage_IsPlaying(RunMontage);

	if (bWantRunMontage)
	{
		if (!bPlaying)
		{
			// 잡기/밀기 같은 다른 몽타주가 재생 중이면 끝날 때까지 기다립니다.
			const UAnimMontage* Active = AnimInstance->GetCurrentActiveMontage();
			if (Active && Active != RunMontage) { return; }

			PlayAnimMontage(RunMontage, 1.f, RunMontageLoopSection);

			// 섹션이 끝나면 스스로 이어지게 해서 반복합니다.
			AnimInstance->Montage_SetNextSection(RunMontageLoopSection, RunMontageLoopSection, RunMontage);
		}

		// 실제 이동 속도에 맞춰 재생 속도를 조절 (발 미끄러짐 줄이기)
		const float Rate = FMath::Clamp(GetVelocity().Size2D() / RunMontageReferenceSpeed, RunMontageMinPlayRate, RunMontageMaxPlayRate);
		AnimInstance->Montage_SetPlayRate(RunMontage, Rate);
	}
	else if (bPlaying)
	{
		AnimInstance->Montage_Stop(RunMontage->BlendOut.GetBlendTime(), RunMontage);
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

void ACTCharacter::RefreshTailVisual()
{
	if (OwnedTail)
	{
		OwnedTail->RefreshVisual();
	}
}

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
	const bool bIsGhost = CTPlayerState && CTPlayerState->IsGhost();

	// 꼬리를 잡힌 유령은 회색, 색이 없으면 기본 색, 아니면 내 색
	const FLinearColor Tint = bIsGhost ? GhostBodyColor
		: (Color == ECTColor::None ? NoColorTint : ACTPlayerState::GetLinearColorOf(Color));

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
