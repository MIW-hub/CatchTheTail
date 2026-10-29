// Copyright Moon Party Games. All Rights Reserved.
#include "CTCharacter.h"
#include "../Core/CTAudioSubsystem.h"
#include "../Core/CTGameMode.h"
#include "../Core/CTGameState.h"
#include "CTPlayerState.h"
#include "../Tail/CTTail.h"
#include "../World/CTBush.h"
#include "../UI/CTHUDWidget.h"
#include "CTPlayerController.h"
#include "../Tongue/CTTongueComponent.h"
#include "../Animation/CTAnimNotifyState_GrabWindow.h"
#include "EngineUtils.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Curves/CurveFloat.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
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
	PrimaryActorTick.bCanEverTick = true; // 로컬 카메라 보간용

	// 이동 방향으로 회전 (3인칭)
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 공중 점프 방지
	JumpMaxCount = 1;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = true;
	MoveComp->RotationRate = FRotator(0.f, 540.f, 0.f);
	MoveComp->JumpZVelocity = 700.f;
	MoveComp->AirControl = 0.35f;
	MoveComp->MaxWalkSpeed = BaseWalkSpeed;
	MoveComp->BrakingDecelerationWalking = 2000.f;

	// 서버 메시도 몽타주 진행 (잡기 노티파이)
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;

	// 3인칭 카메라, 거리는 Tick 보간
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = CameraDistanceBehind;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 60.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = bCameraCollision; // 벽 충돌 시 당김 (기본 꺼짐)
	CameraBoom->ProbeSize = CameraProbeSize;
	CameraBoom->ProbeChannel = ECC_Camera;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 15.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// 부쉬 속 겹친 몸 (내 화면만)
	XRayMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("XRayMesh"));
	XRayMesh->SetupAttachment(GetMesh());
	XRayMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	XRayMesh->SetGenerateOverlapEvents(false);
	XRayMesh->SetCanEverAffectNavigation(false);
	XRayMesh->SetCastShadow(false);
	XRayMesh->SetOnlyOwnerSee(true);
	XRayMesh->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> XRayFinder(TEXT("/Game/Material/M_Player_XRay.M_Player_XRay"));
	if (XRayFinder.Succeeded()) { XRayMaterial = XRayFinder.Object; }

	// 혀: Head 본에 부착
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
	DOREPLIFETIME(ACTCharacter, CurrentBush);
	DOREPLIFETIME(ACTCharacter, bBoothLocked);
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
	if (IsGhost())
	{
		return false;
	}

	// 색 배정 전 점프 불가
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
	RefreshTailVisual();

	// 유령 알림 선행 시 누락 보정
	if (IsGhost())
	{
		OnGhostStateChanged(true);
	}
}

// 상태 읽기

bool ACTCharacter::IsGhost() const
{
	const ACTPlayerState* CTPlayerState = GetPlayerState<ACTPlayerState>();
	return CTPlayerState && CTPlayerState->IsGhost();
}

float ACTCharacter::GetSpeedMultiplier() const
{
	// 1.0 + 달리기 0.5 + 추격 0.2
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

float ACTCharacter::GetPushCooldownRemainingSeconds() const
{
	if (PushCooldownSeconds <= 0.f || !GetWorld()) { return 0.f; }

	const double Elapsed = GetWorld()->GetTimeSeconds() - LocalLastPushTime;
	return FMath::Clamp(static_cast<float>(PushCooldownSeconds - Elapsed), 0.f, PushCooldownSeconds);
}

float ACTCharacter::GetGrabReadyPercent() const
{
	if (GrabCooldownSeconds <= 0.f || !GetWorld()) { return 1.f; }

	const double Elapsed = GetWorld()->GetTimeSeconds() - LocalLastGrabTime;
	return FMath::Clamp(static_cast<float>(Elapsed / GrabCooldownSeconds), 0.f, 1.f);
}

float ACTCharacter::GetGrabCooldownRemainingSeconds() const
{
	if (GrabCooldownSeconds <= 0.f || !GetWorld()) { return 0.f; }

	const double Elapsed = GetWorld()->GetTimeSeconds() - LocalLastGrabTime;
	return FMath::Clamp(static_cast<float>(GrabCooldownSeconds - Elapsed), 0.f, GrabCooldownSeconds);
}

bool ACTCharacter::CanMoveNow() const
{
	// 전화부스 사용 중에는 못 움직임
	if (bBoothLocked) { return false; }

	const ACTGameState* CTGameState = GetWorld() ? GetWorld()->GetGameState<ACTGameState>() : nullptr;
	// 대기 로비는 자유롭게 돌아다닐 수 있음
	if (CTGameState && CTGameState->GetPhase() == ECTGamePhase::Lobby) { return true; }

	if (!CTGameState || CTGameState->GetPhase() != ECTGamePhase::Playing) { return false; }

	// 색 배정 전 이동 불가
	const ACTPlayerState* CTPlayerState = GetPlayerState<ACTPlayerState>();
	return CTPlayerState && CTPlayerState->GetAssignedColor() != ECTColor::None;
}

bool ACTCharacter::CanUseActions() const
{
	if (IsGhost()) { return false; }

	// 부쉬 안, 전화부스 사용 중에는 잡기/밀기 불가
	if (CurrentBush || bBoothLocked) { return false; }

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

// 속도 버프

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
	// 버프 목록 복제, 양쪽 계산
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

	// 게임 중 + 유령 아님 → 추격 ON
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
			SetCurrentBush(nullptr);
			SetBoothLocked(false);
		}

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

		// 유령: 꼬리 불 끔+회색, 부활 시 복구
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
		// 잡힘 몽타주 유지, 잡기/밀기만 중단
		StopAttackMontages();
	}

	// 유령 회색, 부활 시 원래 색
	RefreshBodyColor();

	BP_OnGhostStateChanged(bNewIsGhost);
}

// 입력 연결

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

	// BP 값은 생성자 이후 적용
	CameraBoom->bDoCollisionTest = bCameraCollision;
	CameraBoom->ProbeSize = CameraProbeSize;
	CameraBoom->TargetArmLength = CameraDistanceBehind;
	FollowCamera->SetFieldOfView(CameraFieldOfView);
}

void ACTCharacter::UpdateCameraClip()
{
	if (CameraHideDistance <= 0.f || !FollowCamera)
	{
		if (bHiddenFromOwnCamera) { SetHiddenFromOwnCamera(false); }
		return;
	}

	// 숨김 해제는 더 멀어져야 (깜빡임 방지)
	const float Distance = FVector::Dist(FollowCamera->GetComponentLocation(), GetActorLocation());
	const float Threshold = bHiddenFromOwnCamera ? CameraHideDistance + CameraHideHysteresis : CameraHideDistance;
	const bool bShouldHide = Distance < Threshold;

	if (bShouldHide != bHiddenFromOwnCamera)
	{
		SetHiddenFromOwnCamera(bShouldHide);
	}
}

void ACTCharacter::SetHiddenFromOwnCamera(bool bNewHidden)
{
	bHiddenFromOwnCamera = bNewHidden;

	// OwnerNoSee: 조종자 화면에서만 숨김
	auto Apply = [bNewHidden](AActor* Actor)
	{
		if (!Actor) { return; }

		TInlineComponentArray<UPrimitiveComponent*> Primitives;
		Actor->GetComponents(Primitives);
		for (UPrimitiveComponent* Primitive : Primitives)
		{
			Primitive->SetOwnerNoSee(bNewHidden);
		}
	};

	Apply(this);      // 몸, 혀
	Apply(OwnedTail); // 꼬리

	UE_LOG(LogTemp, Log, TEXT("[CT] 카메라 %s: 내 캐릭터 %s"), bNewHidden ? TEXT("가까움") : TEXT("멀어짐"), bNewHidden ? TEXT("숨김") : TEXT("보임"));
}

void ACTCharacter::UpdateMovementSounds(float DeltaSeconds)
{
	if (GetNetMode() == NM_DedicatedServer) { return; }

	const UCharacterMovementComponent* Move = GetCharacterMovement();
	UCTAudioSubsystem* Audio = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTAudioSubsystem>() : nullptr;
	if (!Move || !Audio) { return; }

	const bool bOnGround = Move->IsMovingOnGround();
	const FVector Velocity = GetVelocity();

	// 점프: 땅에서 떨어지면서 위로 올라갈 때
	if (bWasOnGroundForSound && !bOnGround && Velocity.Z > 150.f && !IsGhost())
	{
		Audio->PlaySfxAtLocation(ECTSfx::Jump, GetActorLocation());
	}

	// 착지: 공중에서 땅에 닿을 때 (충분히 빠르게 떨어졌을 때만)
	if (!bOnGround)
	{
		LastFallSpeedForSound = FMath::Max(LastFallSpeedForSound, -Velocity.Z);
	}
	else if (!bWasOnGroundForSound)
	{
		if (LastFallSpeedForSound >= LandMinFallSpeed && !IsGhost())
		{
			Audio->PlaySfxAtLocation(ECTSfx::Land, GetActorLocation());
		}
		LastFallSpeedForSound = 0.f;
		FootstepDistanceAccum = 0.f;
	}
	bWasOnGroundForSound = bOnGround;

	// 발소리: 땅 위에서 일정 거리를 움직일 때마다
	const float Speed = Velocity.Size2D();
	if (!bUseDistanceFootsteps)
	{
		// 발소리는 애니메이션 노티파이가 담당
	}
	else if (bOnGround && Speed >= MinFootstepSpeed && !IsGhost())
	{
		FootstepDistanceAccum += Speed * DeltaSeconds;
		if (FootstepDistanceAccum >= FootstepDistance)
		{
			FootstepDistanceAccum = 0.f;
			Audio->PlaySfxAtLocation(ECTSfx::Footstep, GetActorLocation(), FootstepVolume, FMath::FRandRange(0.92f, 1.08f));
		}
	}
	else if (!bOnGround || Speed < MinFootstepSpeed)
	{
		// 멈추면 다음 걸음 바로 나게 절반 채움
		FootstepDistanceAccum = FootstepDistance * 0.5f;
	}
}

void ACTCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateRunMontage();
	UpdateGhostWalkMontage();
	UpdateBushVisibility();
	UpdateBushXRay();
	UpdateMovementSounds(DeltaSeconds);

	// 로컬 전용
	if (IsLocallyControlled())
	{
		UpdateCameraDistance(DeltaSeconds);
	}
}

void ACTCharacter::UpdateCameraDistance(float DeltaSeconds)
{
	if (!CameraBoom || !Controller) { return; }

	// 0 근처=수평, 음수 클수록 위에서 봄
	const float Pitch = FRotator::NormalizeAxis(Controller->GetControlRotation().Pitch);

	// BehindPitch→0, TopPitch→1
	const float Range = BehindPitch - TopPitch;
	float Alpha = Range > KINDA_SMALL_NUMBER ? FMath::Clamp((BehindPitch - Pitch) / Range, 0.f, 1.f) : 0.f;

	// 곡선 없으면 SmoothStep
	Alpha = DistanceCurve ? FMath::Clamp(DistanceCurve->GetFloatValue(Alpha), 0.f, 1.f) : FMath::SmoothStep(0.f, 1.f, Alpha);

	const float TargetLength = FMath::Lerp(CameraDistanceBehind, CameraDistanceTop, Alpha);
	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetLength, DeltaSeconds, CameraDistanceInterpSpeed);

	// 근접 시 내 캐릭터 숨김
	UpdateCameraClip();
}

void ACTCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MoveValue = Value.Get<FVector2D>();

	// 색 배정 전 이동 불가, 회전은 가능
	if (!CanMoveNow()) { return; }

	if (Controller)
	{
		// 카메라 기준 방향 (유령도 이동)
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

	// 쿨타임 중 서버 거절
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LocalLastGrabTime < GrabCooldownSeconds) { return; }
	LocalLastGrabTime = Now;

	// 서버 응답 전 선재생
	PlayActionMontage(ECTCharacterAction::Grab);
	ServerGrab();
}

void ACTCharacter::OnPushPressed()
{
	if (!CanUseActions()) { return; }

	// 쿨타임 중 서버 거절
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LocalLastPushTime < PushCooldownSeconds) { return; }
	LocalLastPushTime = Now;

	PlayActionMontage(ECTCharacterAction::Push);
	ServerPush();
}

// 달리기 (서버)

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
	if (IsGhost() || bBoothLocked || RunState != ECTRunState::Idle)
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

// 잡기 / 밀기 (서버)

void ACTCharacter::ServerGrab_Implementation()
{
	UWorld* World = GetWorld();
	ACTGameMode* CTGameMode = World ? World->GetAuthGameMode<ACTGameMode>() : nullptr;
	const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	ACTPlayerState* MyPlayerState = GetPlayerState<ACTPlayerState>();

	if (!CTGameMode || !CTGameState || !MyPlayerState) { return; }
	if (MyPlayerState->IsGhost()) { return; }
	if (CurrentBush || bBoothLocked) { return; }
	if (CTGameState->GetPhase() != ECTGamePhase::Playing) { return; }

	// 동작 중이면 재시작 안 함 (연타 방지)
	const double Now = World->GetTimeSeconds();
	if (bGrabActive || Now < GrabLockEndTime || GetWorldTimerManager().IsTimerActive(GrabWindowStartTimerHandle)) { return; }

	// 쿨타임, 시계 오차로 90%
	if (Now - LastGrabTime < GrabCooldownSeconds * 0.9f) { return; }
	LastGrabTime = Now;

	bool bSuccess = false;

	if (bRequireTailTouch)
	{
		// 꼬리 터치 방식, 성공 여부 미정이라 false
		if (bUseMontageGrabWindow && HasGrabWindowNotify())
		{
			// Grab Window 구간 동안 판정
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

	// 연출은 성공 여부와 무관
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

	// 목표 색만
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
			// 잡힘 연출은 GameMode가 처리
			return CTGameMode->HandleTailGrabbed(MyPlayerState, OtherPlayerState);
		}
	}
	return false;
}

// 꼬리 / 잡기 동작 구간 (서버)

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
		ApplyTailOffset();
		OwnedTail->RefreshVisual();
	}
}

void ACTCharacter::ApplyTailOffset()
{
	if (!OwnedTail) { return; }

	// 본 기준 자세 보정, 기본 자세에서 TailOffset에 위치
	const USkeletalMesh* MeshAsset = GetMesh() ? GetMesh()->GetSkeletalMeshAsset() : nullptr;
	const int32 BoneIndex = MeshAsset ? MeshAsset->GetRefSkeleton().FindBoneIndex(TailSocketName) : INDEX_NONE;
	if (BoneIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CT] 꼬리를 붙일 본 '%s'을(를) 캐릭터 메시에서 찾지 못했습니다."), *TailSocketName.ToString());
		return;
	}

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

void ACTCharacter::OnRep_OwnedTail()
{
	// 복제 스케일이 소수 2자리로 양자화돼 오프셋 직접 재계산
	ApplyTailOffset();
	RefreshBodyColor();
	RefreshTailVisual();
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
	TickGrabWindow(); // 이미 닿은 꼬리 즉시 확인
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
	// 몽타주는 전원, 판정은 서버만
	if (!HasAuthority() || !bRequireTailTouch || !CanUseActions()) { return; }

	// 끝은 OnGrabNotifyEnd, 최대 시간은 안전장치
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

	// 닿으면 시도, 성공 시 구간 종료
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
	if (CurrentBush || bBoothLocked) { return; }
	if (CTGameState->GetPhase() != ECTGamePhase::Playing) { return; }

	// 쿨타임 확인
	const double Now = World->GetTimeSeconds();
	if (Now - LastPushTime < PushCooldownSeconds) { return; }
	LastPushTime = Now;

	MulticastPlayAction(ECTCharacterAction::Push, true);

	// 정면 부채꼴 전원 밀기 (유령 제외)
	for (APlayerState* OtherState : CTGameState->PlayerArray)
	{
		ACTCharacter* OtherCharacter = OtherState ? Cast<ACTCharacter>(OtherState->GetPawn()) : nullptr;
		if (!OtherCharacter || OtherCharacter == this) { continue; }
		if (OtherCharacter->IsGhost()) { continue; }
		if (!IsInFrontCone(OtherCharacter, PushDistance, PushHalfAngleDegrees)) { continue; }

		FVector Direction = OtherCharacter->GetActorLocation() - GetActorLocation();
		Direction.Z = 0.f;
		Direction = Direction.GetSafeNormal();

		// 키만큼 밀리게 속도 역산
		float PushSpeed = 0.f;
		const UCharacterMovementComponent* OtherMovement = OtherCharacter->GetCharacterMovement();
		if (OtherMovement && PushUpSpeed > 0.f)
		{
			const float Gravity = FMath::Max(FMath::Abs(OtherMovement->GetGravityZ()), 1.f);
			const float AirTime = 2.f * PushUpSpeed / Gravity;
			const float BrakingK = FMath::Max(OtherMovement->GroundFriction * OtherMovement->BrakingFrictionFactor, 1.f);
			const float Height = OtherCharacter->GetSimpleCollisionHalfHeight() * 2.f;
			PushSpeed = Height * PushDistanceInHeights / (AirTime + 1.f / BrakingK);
		}

		const FVector LaunchVelocity = Direction * PushSpeed + FVector(0.f, 0.f, PushUpSpeed);
		OtherCharacter->LaunchCharacter(LaunchVelocity, /*bXYOverride=*/true, /*bZOverride=*/true);
		OtherCharacter->MulticastPlayAction(ECTCharacterAction::Pushed, true);
	}
}

// 애니메이션

void ACTCharacter::MulticastPlayAction_Implementation(ECTCharacterAction Action, bool bSuccess)
{
	// 본인 잡기/밀기는 입력 시 이미 재생
	const bool bAlreadyPlayedLocally =
		IsLocallyControlled() &&
		(Action == ECTCharacterAction::Grab || Action == ECTCharacterAction::Push);

	if (!bAlreadyPlayedLocally)
	{
		PlayActionMontage(Action);
	}

	// 동작 소리 (밀기, 잡힘)
	if (UCTAudioSubsystem* Audio = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTAudioSubsystem>() : nullptr)
	{
		if (Action == ECTCharacterAction::Push) { Audio->PlaySfxAtLocation(ECTSfx::Push, GetActorLocation()); }
		else if (Action == ECTCharacterAction::Caught) { Audio->PlaySfxAtLocation(ECTSfx::TailLost, GetActorLocation()); }
	}

	BP_OnAction(Action, bSuccess);
}

void ACTCharacter::PlayActionMontage(ECTCharacterAction Action)
{
	UAnimMontage* Montage = GetActionMontage(Action);
	if (!Montage) { return; }

	PlayAnimMontage(Montage, Action == ECTCharacterAction::Push ? PushMontagePlayRate : ActionMontagePlayRate);

	// 넘어진 뒤 일어나기 이어 재생
	if (Action == ECTCharacterAction::Pushed && GetUpMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			FOnMontageBlendingOutStarted Delegate;
			Delegate.BindUObject(this, &ACTCharacter::HandlePushedMontageEnded);
			AnimInstance->Montage_SetBlendingOutDelegate(Delegate, Montage);
		}
	}
}

void ACTCharacter::HandlePushedMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted || !GetUpMontage || IsGhost()) { return; }
	PlayAnimMontage(GetUpMontage, ActionMontagePlayRate);
}

void ACTCharacter::UpdateGhostWalkMontage()
{
	// 전용 서버는 연출 불필요
	if (!GhostWalkMontage || GetNetMode() == NM_DedicatedServer) { return; }

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance) { return; }

	// 유령+이동+지상만, 공중은 점프 모션
	const bool bWantWalk =
		IsGhost() &&
		!GetCharacterMovement()->IsFalling() &&
		GetVelocity().Size2D() > GhostWalkMinSpeed;

	const bool bPlaying = AnimInstance->Montage_IsPlaying(GhostWalkMontage);

	if (bWantWalk)
	{
		if (!bPlaying)
		{
			// 다른 몽타주(잡힘 등) 끝날 때까지 대기
			const UAnimMontage* Active = AnimInstance->GetCurrentActiveMontage();
			if (Active && Active != GhostWalkMontage) { return; }

			PlayAnimMontage(GhostWalkMontage, 1.f, GhostWalkLoopSection);
			AnimInstance->Montage_SetNextSection(GhostWalkLoopSection, GhostWalkLoopSection, GhostWalkMontage);
		}

		// 실속도에 재생 속도 맞춤 (발 미끄러짐)
		const float Rate = FMath::Clamp(GetVelocity().Size2D() / GhostWalkReferenceSpeed, GhostWalkMinPlayRate, GhostWalkMaxPlayRate);
		AnimInstance->Montage_SetPlayRate(GhostWalkMontage, Rate);
	}
	else if (bPlaying)
	{
		AnimInstance->Montage_Stop(GhostWalkMontage->BlendOut.GetBlendTime(), GhostWalkMontage);
	}
}

void ACTCharacter::UpdateRunMontage()
{
	// 전용 서버는 연출 불필요
	if (!RunMontage || GetNetMode() == NM_DedicatedServer) { return; }

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance) { return; }

	// 달리기+이동+지상만, 공중은 점프 모션
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
			// 다른 몽타주 끝날 때까지 대기
			const UAnimMontage* Active = AnimInstance->GetCurrentActiveMontage();
			if (Active && Active != RunMontage) { return; }

			PlayAnimMontage(RunMontage, 1.f, RunMontageLoopSection);

			// 섹션 종료 시 자동 반복
			AnimInstance->Montage_SetNextSection(RunMontageLoopSection, RunMontageLoopSection, RunMontage);
		}

		// 실속도에 재생 속도 맞춤 (발 미끄러짐)
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

// 도우미

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
	ToOther.Z = 0.f; // 높이 무시

	const float Distance = ToOther.Size();
	if (Distance > MaxDistance) { return false; }
	if (Distance < KINDA_SMALL_NUMBER) { return true; } // 거의 겹침 → 성공

	FVector Forward = GetActorForwardVector();
	Forward.Z = 0.f;
	Forward = Forward.GetSafeNormal();

	const float CosAngle = FVector::DotProduct(Forward, ToOther / Distance);
	return CosAngle >= FMath::Cos(FMath::DegreesToRadians(HalfAngleDegrees));
}

// 몸 색상

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

	// 슬롯당 MID 1회 생성
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

	// 잡힌 유령 회색, 색 없으면 기본색
	const FLinearColor Tint = bIsGhost ? GhostBodyColor
		: (Color == ECTColor::None ? NoColorTint : ACTPlayerState::GetLinearColorOf(Color));

	for (UMaterialInstanceDynamic* Material : BodyMaterials)
	{
		if (Material)
		{
			Material->SetVectorParameterValue(BodyColorParameterName, Tint);
		}
	}

	// 부쉬 안에서 보이는 겹친 몸도 같은 색
	for (UMaterialInstanceDynamic* Material : XRayMaterials)
	{
		if (Material)
		{
			Material->SetVectorParameterValue(BodyColorParameterName, Tint);
		}
	}

	// 꼬리도 같은 색
	if (OwnedTail)
	{
		OwnedTail->RefreshVisual();
	}
}

// 레벨 기믹 (부쉬, 전화부스)

void ACTCharacter::SetCurrentBush(ACTBush* NewBush)
{
	if (!HasAuthority() || CurrentBush == NewBush) { return; }

	CurrentBush = NewBush;
	OnRep_CurrentBush();
}

void ACTCharacter::OnRep_CurrentBush()
{
	// 몸 숨김은 UpdateBushVisibility가 판단
	UpdateBushVisibility();
}

void ACTCharacter::SetBoothLocked(bool bNewLocked)
{
	if (!HasAuthority() || bBoothLocked == bNewLocked) { return; }

	bBoothLocked = bNewLocked;
	if (bNewLocked)
	{
		StopJumping();
		GetCharacterMovement()->StopMovementImmediately();
	}
}

void ACTCharacter::ClientShowNotice_Implementation(const FText& Message, float Seconds)
{
	if (const ACTPlayerController* CTController = Cast<ACTPlayerController>(GetController()))
	{
		if (UCTHUDWidget* HUD = CTController->GetHUDWidget())
		{
			HUD->ShowNotice(Message, Seconds);
		}
	}
}

void ACTCharacter::UpdateBushVisibility()
{
	// 전용 서버는 화면 없음
	if (GetNetMode() == NM_DedicatedServer) { return; }

	bool bShouldHide = false;
	if (CurrentBush && !IsLocallyControlled())
	{
		// 같은 부쉬 안/유령은 숨은 몸 보임
		const APlayerController* LocalController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		const ACTCharacter* Viewer = LocalController ? Cast<ACTCharacter>(LocalController->GetPawn()) : nullptr;
		const bool bViewerSees = !Viewer || Viewer->IsGhost() || Viewer->CurrentBush == CurrentBush;
		bShouldHide = !bViewerSees;
	}

	if (bShouldHide == bBodyHiddenByBush) { return; }
	bBodyHiddenByBush = bShouldHide;

	// 꼬리는 그대로 둠, 부착된 꼬리까지 숨기면 안 됨
	if (GetMesh()) { GetMesh()->SetVisibility(!bShouldHide, /*bPropagateToChildren=*/false); }
	if (TongueMesh) { TongueMesh->SetVisibility(!bShouldHide, /*bPropagateToChildren=*/false); }
}

void ACTCharacter::UpdateBushXRay()
{
	// 부쉬 안일 때 겹친 몸 켬 (내 화면만)
	if (!IsLocallyControlled() || !XRayMesh || !GetMesh()) { return; }

	const bool bWantXRay = CurrentBush != nullptr && XRayMaterial != nullptr;
	if (bWantXRay == bXRayActive) { return; }
	bXRayActive = bWantXRay;

	if (bWantXRay && !bXRaySetup)
	{
		// 최초 1회 메시 설정, 원본 포즈 추종
		XRayMesh->SetSkeletalMesh(GetMesh()->GetSkeletalMeshAsset());
		XRayMesh->SetLeaderPoseComponent(GetMesh());

		XRayMaterials.Reset();
		for (int32 SlotIndex = 0; SlotIndex < XRayMesh->GetNumMaterials(); ++SlotIndex)
		{
			XRayMaterials.Add(XRayMesh->CreateAndSetMaterialInstanceDynamicFromMaterial(SlotIndex, XRayMaterial));
		}
		bXRaySetup = true;
		RefreshBodyColor(); // 색을 겹친 몸에도 적용
	}

	XRayMesh->SetVisibility(bWantXRay);
}
