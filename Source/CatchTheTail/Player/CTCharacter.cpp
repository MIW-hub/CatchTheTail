// Copyright Moon Party Games. All Rights Reserved.
#include "CTCharacter.h"
#include "../Core/CTAudioSubsystem.h"
#include "../Core/CTGameMode.h"
#include "../Core/CTGameState.h"
#include "CTPlayerState.h"
#include "../Tail/CTTail.h"
#include "../World/CTBush.h"
#include "../UI/CTHUDWidget.h"
#include "../UI/CTEmoteBubbleComponent.h"
#include "../UI/CTEmoteSelectWidget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Texture2D.h"
#include "CTCharacterMovementComponent.h"
#include "CTGrappleComponent.h"
#include "CTPlayerController.h"
#include "../Tongue/CTTongueComponent.h"
#include "../Animation/CTAnimNotifyState_GrabWindow.h"
#include "EngineUtils.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
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
#include "InputAction.h"
#include "InputActionValue.h"
#include "Net/VoiceConfig.h"
#include "Sound/SoundAttenuation.h"
#include "../Core/CTSettingsSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"

namespace CTBuffNames
{
	static const FName Run(TEXT("Run"));
	static const FName Chase(TEXT("Chase"));
	static const FName Ghost(TEXT("Ghost"));
}

// 이동이 느리게 출발해서 점점 빨라지는 이동 컴포넌트 사용.
// 예전 방식으로 되돌리려면 아래 3줄을 주석 해제하고 그 아래 ObjectInitializer 버전을 지우면 됨 (헤더도 ACTCharacter() 로 복구).
// ACTCharacter::ACTCharacter()
// {
ACTCharacter::ACTCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCTCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// 눌러서 말하기 입력 (프로젝트의 IA_PushToTalk)
	static ConstructorHelpers::FObjectFinder<UInputAction> VoiceActionFinder(TEXT("/Game/Input/IA_PushToTalk.IA_PushToTalk"));
	if (VoiceActionFinder.Succeeded()) { VoiceAction = VoiceActionFinder.Object; }

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

	// AI 길찾기 이동도 가속(Acceleration)으로 움직이게 함. 끄면 AI는 속도만 직접 주어져 가속이 0이라,
	// 이동 애니메이션(가속이 있을 때만 걷기로 전환)이 안 나오고 다리가 안 움직인 채 미끄러짐
	if (FNavMovementProperties* NavProperties = MoveComp->GetNavMovementProperties())
	{
		NavProperties->bUseAccelerationForPaths = true;
	}

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
	// 보일지는 UpdateBushXRay가 화면마다 따로 정함 (내 부쉬 안 / 같은 부쉬 안 다른 플레이어)
	XRayMesh->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> XRayFinder(TEXT("/Game/Material/M_Player_XRay.M_Player_XRay"));
	if (XRayFinder.Succeeded()) { XRayMaterial = XRayFinder.Object; }

	// 혀: Head 본에 부착
	TongueMesh = CreateDefaultSubobject<UCTTongueComponent>(TEXT("TongueMesh"));
	TongueMesh->SetupAttachment(GetMesh(), TEXT("Head"));

	// 이모트 말풍선: 캡슐에 붙여 머리 위에서 올라옴
	EmoteBubble = CreateDefaultSubobject<UCTEmoteBubbleComponent>(TEXT("EmoteBubble"));
	EmoteBubble->SetupAttachment(RootComponent);

	// 꼬리 잡기 F 연타 대결
	Grapple = CreateDefaultSubobject<UCTGrappleComponent>(TEXT("Grapple"));

	// 이모트 고르는 창 모양 (WBP). 디자인은 WBP 에서 함
	static ConstructorHelpers::FClassFinder<UCTEmoteSelectWidget> EmoteMenuFinder(TEXT("/Game/UI/WBP_CTEmoteSelectWidget"));
	if (EmoteMenuFinder.Succeeded()) { EmoteMenuClass = EmoteMenuFinder.Class; }

	// 이모트 목록: 아틀라스(3x3) 3장 = 27개. 말풍선 글자가 다른 한국어/영어/일본어 시트가 따로 있음
	static const TCHAR* const SheetPathsKo[3] =
	{
		TEXT("/Game/2D/UI/Emote/Emote_Ko_Team.Emote_Ko_Team"),
		TEXT("/Game/2D/UI/Emote/Emote_Ko_Game.Emote_Ko_Game"),
		TEXT("/Game/2D/UI/Emote/Emote_Ko_Feelings.Emote_Ko_Feelings"),
	};
	static const TCHAR* const SheetPathsEn[3] =
	{
		TEXT("/Game/2D/UI/Emote/Emote_En_Team.Emote_En_Team"),
		TEXT("/Game/2D/UI/Emote/Emote_En_Game.Emote_En_Game"),
		TEXT("/Game/2D/UI/Emote/Emote_En_Feelings.Emote_En_Feelings"),
	};
	static const TCHAR* const SheetPathsJa[3] =
	{
		TEXT("/Game/2D/UI/Emote/Emote_Ja_Team.Emote_Ja_Team"),
		TEXT("/Game/2D/UI/Emote/Emote_Ja_Game.Emote_Ja_Game"),
		TEXT("/Game/2D/UI/Emote/Emote_Ja_Feelings.Emote_Ja_Feelings"),
	};
	static const TCHAR* const SheetLabels[27] =
	{
		TEXT("같이 가!"), TEXT("기다려!"), TEXT("도와줘!"), TEXT("따라와!"), TEXT("준비 끝!"), TEXT("가자!"), TEXT("잘했어!"), TEXT("복수할게"), TEXT("또 하자!"),
		TEXT("헌터야?"), TEXT("경계!"), TEXT("도망가!"), TEXT("살려줘!"), TEXT("잡았다!"), TEXT("들켰다!"), TEXT("쉿!"), TEXT("저기!"), TEXT("난 아냐!"),
		TEXT("안녕!"), TEXT("고마워!"), TEXT("미안!"), TEXT("ㅋㅋㅋ"), TEXT("흑흑"), TEXT("헉!"), TEXT("흥!"), TEXT("좋아!"), TEXT("사랑해"),
	};
	static const TCHAR* const SheetLabelsEn[27] =
	{
		TEXT("Together!"), TEXT("Wait!"), TEXT("Help!"), TEXT("Follow me!"), TEXT("Ready!"), TEXT("Let's go!"), TEXT("Well done!"), TEXT("I'll get you!"), TEXT("Again!"),
		TEXT("Hunter?"), TEXT("Watch out!"), TEXT("Run!"), TEXT("Help me!"), TEXT("Got you!"), TEXT("Busted!"), TEXT("Shh!"), TEXT("Over there!"), TEXT("Not me!"),
		TEXT("Hi!"), TEXT("Thanks!"), TEXT("Sorry!"), TEXT("LOL"), TEXT("Sob..."), TEXT("Gasp!"), TEXT("Hmph!"), TEXT("Nice!"), TEXT("Love you!"),
	};
	static const TCHAR* const SheetLabelsJa[27] =
	{
		TEXT("一緒に行こう!"), TEXT("待って!"), TEXT("手伝って!"), TEXT("ついてきて!"), TEXT("準備OK!"), TEXT("行こう!"), TEXT("やったね!"), TEXT("仕返しだ!"), TEXT("もう一回!"),
		TEXT("ハンター?"), TEXT("警戒!"), TEXT("逃げて!"), TEXT("助けて!"), TEXT("捕まえた!"), TEXT("バレた!"), TEXT("しーっ!"), TEXT("あそこ!"), TEXT("私じゃない!"),
		TEXT("やっほー!"), TEXT("ありがとう!"), TEXT("ごめん!"), TEXT("あはは!"), TEXT("えーん"), TEXT("えっ!"), TEXT("ふん!"), TEXT("いいね!"), TEXT("大好き!"),
	};
	for (int32 Sheet = 0; Sheet < 3; ++Sheet)
	{
		ConstructorHelpers::FObjectFinder<UTexture2D> SheetFinderKo(SheetPathsKo[Sheet]);
		ConstructorHelpers::FObjectFinder<UTexture2D> SheetFinderEn(SheetPathsEn[Sheet]);
		ConstructorHelpers::FObjectFinder<UTexture2D> SheetFinderJa(SheetPathsJa[Sheet]);
		for (int32 Cell = 0; Cell < 9; ++Cell)
		{
			FCTEmoteSlot& NewSlot = EmoteSlots.AddDefaulted_GetRef();
			if (SheetFinderKo.Succeeded()) { NewSlot.Atlas = SheetFinderKo.Object; }
			if (SheetFinderEn.Succeeded()) { NewSlot.AtlasEn = SheetFinderEn.Object; }
			if (SheetFinderJa.Succeeded()) { NewSlot.AtlasJa = SheetFinderJa.Object; }
			NewSlot.AtlasColumns = 3;
			NewSlot.AtlasRows = 3;
			NewSlot.AtlasCell = Cell;
			NewSlot.Label = FText::FromString(SheetLabels[Sheet * 9 + Cell]);
			NewSlot.LabelEn = FText::FromString(SheetLabelsEn[Sheet * 9 + Cell]);
			NewSlot.LabelJa = FText::FromString(SheetLabelsJa[Sheet * 9 + Cell]);
		}
	}
}

// 지금 게임 언어에 맞는 이모트 그림 / 이름 (없으면 한국어)
namespace
{
	FString CurrentEmoteLanguage()
	{
		return FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName();
	}
}

UTexture2D* FCTEmoteSlot::GetAtlas() const
{
	const FString Language = CurrentEmoteLanguage();
	if (Language == TEXT("en") && AtlasEn) { return AtlasEn; }
	if (Language == TEXT("ja") && AtlasJa) { return AtlasJa; }
	return Atlas;
}

FText FCTEmoteSlot::GetLabel() const
{
	const FString Language = CurrentEmoteLanguage();
	if (Language == TEXT("en") && !LabelEn.IsEmpty()) { return LabelEn; }
	if (Language == TEXT("ja") && !LabelJa.IsEmpty()) { return LabelJa; }
	return Label;
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

// 블루프린트에 저장된 이모트 칸에 빠진 그림/이름(특히 영어, 일본어)을 코드 기본값으로 채움
void ACTCharacter::FillMissingEmoteDefaults()
{
	const ACTCharacter* NativeDefaults = ACTCharacter::StaticClass()->GetDefaultObject<ACTCharacter>();
	if (!NativeDefaults || NativeDefaults == this) { return; }

	for (int32 Index = 0; Index < EmoteSlots.Num() && Index < NativeDefaults->EmoteSlots.Num(); ++Index)
	{
		FCTEmoteSlot& Slot = EmoteSlots[Index];
		const FCTEmoteSlot& Default = NativeDefaults->EmoteSlots[Index];
		if (!Slot.Atlas) { Slot.Atlas = Default.Atlas; }
		if (!Slot.AtlasEn) { Slot.AtlasEn = Default.AtlasEn; }
		if (!Slot.AtlasJa) { Slot.AtlasJa = Default.AtlasJa; }
		if (Slot.Label.IsEmpty()) { Slot.Label = Default.Label; }
		if (Slot.LabelEn.IsEmpty()) { Slot.LabelEn = Default.LabelEn; }
		if (Slot.LabelJa.IsEmpty()) { Slot.LabelJa = Default.LabelJa; }
	}
}

void ACTCharacter::BeginPlay()
{
	FillMissingEmoteDefaults();
	Super::BeginPlay();

	EnsureVoiceTalker();

	// 팔 늘리기: 포즈 확정 직후 팔 뼈 조정 (화면이 있는 쪽만)
	if (GetNetMode() != NM_DedicatedServer && GetMesh())
	{
		ArmStretchPoseHandle = GetMesh()->RegisterOnBoneTransformsFinalizedDelegate(
			FOnBoneTransformsFinalizedMultiCast::FDelegate::CreateUObject(this, &ACTCharacter::ApplyArmStretchToPose));
	}

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

	EnsureVoiceTalker();

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

	// 잡기/밀기 동작, 넘어져 있는 동안(넘어짐~일어나기)은 그 자리에 멈춤 (입력만 막고 감속은 이동 컴포넌트가 처리)
	if (const UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		for (const UAnimMontage* Montage : { GrabMontage.Get(), PushMontage.Get(), PushedMontage.Get(), PushedBackMontage.Get(), GetUpMontage.Get(), GetUpBackMontage.Get() })
		{
			if (Montage && AnimInstance->Montage_IsPlaying(Montage)) { return false; }
		}
	}

	const ACTGameState* CTGameState = GetWorld() ? GetWorld()->GetGameState<ACTGameState>() : nullptr;
	// 대기 로비는 자유롭게 돌아다닐 수 있음
	if (CTGameState && CTGameState->GetPhase() == ECTGamePhase::Lobby) { return true; }

	if (!CTGameState || CTGameState->GetPhase() != ECTGamePhase::Playing) { return false; }

	// 색 배정 전 이동 불가
	const ACTPlayerState* CTPlayerState = GetPlayerState<ACTPlayerState>();
	return CTPlayerState && CTPlayerState->GetAssignedColor() != ECTColor::None;
}

bool ACTCharacter::IsGrappling() const
{
	return Grapple && Grapple->IsGrappling();
}

bool ACTCharacter::IsPlayingCaughtMontage() const
{
	const UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	return AnimInstance && CaughtMontage && AnimInstance->Montage_IsPlaying(CaughtMontage);
}

bool ACTCharacter::IsPlayingKnockdownMontage() const
{
	const UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance) { return false; }

	for (const UAnimMontage* Montage : { PushedMontage.Get(), PushedBackMontage.Get(), GetUpMontage.Get(), GetUpBackMontage.Get() })
	{
		if (Montage && AnimInstance->Montage_IsPlaying(Montage)) { return true; }
	}
	return false;
}

bool ACTCharacter::CanUseActions() const
{
	if (IsGhost()) { return false; }

	// 꼬리 잡기 대결 중에는 다른 행동 불가 (G는 연타)
	if (IsGrappling()) { return false; }

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
	case ECTCharacterAction::PushedBack: return PushedBackMontage ? PushedBackMontage.Get() : PushedMontage.Get();
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
	if (VoiceAction)
	{
		EnhancedInput->BindAction(VoiceAction, ETriggerEvent::Started, this, &ACTCharacter::OnVoicePressed);
		EnhancedInput->BindAction(VoiceAction, ETriggerEvent::Completed, this, &ACTCharacter::OnVoiceReleased);
		EnhancedInput->BindAction(VoiceAction, ETriggerEvent::Canceled, this, &ACTCharacter::OnVoiceReleased);
	}
	if (PushAction)
	{
		EnhancedInput->BindAction(PushAction, ETriggerEvent::Started, this, &ACTCharacter::OnPushPressed);
	}

	// 저장해 둔 이모트 등록 불러오기 (내가 조종하게 된 캐릭터에서만 호출됨)
	if (const UCTSettingsSubsystem* Settings = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTSettingsSubsystem>() : nullptr)
	{
		const TArray<int32>& Saved = Settings->GetSavedEmoteLoadout();
		for (int32 Index = 0; Index < EmoteLoadout.Num() && Index < Saved.Num(); ++Index)
		{
			if (EmoteSlots.IsValidIndex(Saved[Index])) { EmoteLoadout[Index] = Saved[Index]; }
		}
	}

	// 잡기 보조 키(기본 G)와 이모트 창 키(기본 R)는 설정에서 바꿀 수 있어서 Tick 에서 현재 키를 읽어 처리

	// 꼬리 줄다리기 중에는 마우스 왼쪽 클릭도 연타로 인정 (평소에는 아무 일도 안 함)
	{
		FInputKeyBinding ClickBinding(FInputChord(EKeys::LeftMouseButton), IE_Pressed);
		ClickBinding.KeyDelegate.GetDelegateForManualSet().BindUObject(this, &ACTCharacter::OnGrappleMouseClick);
		EnhancedInput->KeyBindings.Emplace(MoveTemp(ClickBinding));
	}

	// 이모트: 윗줄 숫자 키 1~9 (칸 수만큼)
	static const FKey EmoteKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
	for (int32 Index = 0; Index < EmoteLoadout.Num() && Index < UE_ARRAY_COUNT(EmoteKeys); ++Index)
	{
		FInputKeyBinding Binding(FInputChord(EmoteKeys[Index]), IE_Pressed);
		Binding.KeyDelegate.GetDelegateForManualSet().BindUObject(this, &ACTCharacter::OnEmoteKeyPressed, Index);
		EnhancedInput->KeyBindings.Emplace(MoveTemp(Binding));
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

	// 점프: BP에서 정한 값을 적용 (체공시간이 길지 않게 중력 배율을 높임)
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->JumpZVelocity = JumpVelocity;
		MoveComp->GravityScale = JumpGravityScale;
	}
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
	UpdateEmoteMontage();
	UpdateBushVisibility();
	UpdateBushXRay();
	UpdateMovementSounds(DeltaSeconds);
	UpdateArmStretch(DeltaSeconds);

	// 로컬 전용
	if (IsLocallyControlled())
	{
		UpdateCameraDistance(DeltaSeconds);
		UpdateVoiceTransmit();

		// 설정에서 바꿀 수 있는 키 (이모트 창, 잡기 보조)
		const APlayerController* LocalController = Cast<APlayerController>(GetController());
		const UGameInstance* KeyGameInstance = GetGameInstance();
		if (LocalController && KeyGameInstance)
		{
			if (const UCTSettingsSubsystem* KeySettings = KeyGameInstance->GetSubsystem<UCTSettingsSubsystem>())
			{
				// 누른 순간에만 한 번 (꾹 누르고 있어도 반복 안 함), 0.25초 안에는 다시 안 받음
				const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
				if (LocalController->WasInputKeyJustPressed(KeySettings->GetCodeKey(CTCodeKeys::EmoteMenu)) && Now - LastEmoteMenuToggleTime > 0.25)
				{
					LastEmoteMenuToggleTime = Now;
					ToggleEmoteMenu();
				}
				if (LocalController->WasInputKeyJustPressed(KeySettings->GetCodeKey(CTCodeKeys::GrabAlt))) { OnGrabPressed(); }
			}
		}
	}
}

void ACTCharacter::UpdateArmStretch(float DeltaSeconds)
{
	if (GetNetMode() == NM_DedicatedServer) { return; }

	// 수동 목표가 기본이고, 잡기 몽타주 중이면 재생 위치에 따라 늘어났다 돌아옴
	float Desired = ArmStretchTarget;
	const UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (AnimInstance && GrabMontage && ArmStretchGrabScale > 1.f && AnimInstance->Montage_IsPlaying(GrabMontage))
	{
		const float Length = GrabMontage->GetPlayLength();
		const float Alpha = Length > KINDA_SMALL_NUMBER ? FMath::Clamp(AnimInstance->Montage_GetPosition(GrabMontage) / Length, 0.f, 1.f) : 0.f;
		const float Pulse = FMath::Sin(Alpha * UE_PI);
		Desired = FMath::Max(Desired, FMath::Lerp(1.f, ArmStretchGrabScale, Pulse));
	}

	ArmStretch = FMath::FInterpTo(ArmStretch, Desired, DeltaSeconds, ArmStretchInterpSpeed);
}

void ACTCharacter::ApplyArmStretchToPose()
{
	if (!bApplyArmStretchToPose || FMath::IsNearlyEqual(ArmStretch, 1.f, 0.01f)) { return; }

	USkeletalMeshComponent* SkelMesh = GetMesh();
	const USkeletalMesh* MeshAsset = SkelMesh ? SkelMesh->GetSkeletalMeshAsset() : nullptr;
	if (!MeshAsset) { return; }

	TArray<FTransform>& ComponentPose = SkelMesh->GetEditableComponentSpaceTransforms();
	const FReferenceSkeleton& RefSkeleton = MeshAsset->GetRefSkeleton();
	const FVector BoneScale = GetArmStretchScale3D();

	static const FName ArmBones[2][3] =
	{
		{ TEXT("LeftArm"),  TEXT("LeftForeArm"),  TEXT("LeftHand")  },
		{ TEXT("RightArm"), TEXT("RightForeArm"), TEXT("RightHand") },
	};

	bool bChanged = false;
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const int32 Arm = SkelMesh->GetBoneIndex(ArmBones[Side][0]);
		const int32 Fore = SkelMesh->GetBoneIndex(ArmBones[Side][1]);
		const int32 Hand = SkelMesh->GetBoneIndex(ArmBones[Side][2]);
		if (Arm == INDEX_NONE || Fore == INDEX_NONE || Hand == INDEX_NONE) { continue; }
		if (!ComponentPose.IsValidIndex(Arm) || !ComponentPose.IsValidIndex(Fore) || !ComponentPose.IsValidIndex(Hand)) { continue; }

		// 관절 위치를 늘어난 길이만큼 밀어냄
		const FVector ArmLocation = ComponentPose[Arm].GetLocation();
		const FVector ForeOld = ComponentPose[Fore].GetLocation();
		const FVector HandOld = ComponentPose[Hand].GetLocation();
		const FVector ForeNew = ArmLocation + (ForeOld - ArmLocation) * ArmStretch;
		const FVector HandNew = ForeNew + (HandOld - ForeOld) * ArmStretch;
		const FVector HandDelta = HandNew - HandOld;

		// 손가락 등 손 아래 뼈는 손과 같이 이동
		for (int32 Index = Hand + 1; Index < ComponentPose.Num(); ++Index)
		{
			for (int32 Parent = RefSkeleton.GetParentIndex(Index); Parent != INDEX_NONE; Parent = RefSkeleton.GetParentIndex(Parent))
			{
				if (Parent == Hand)
				{
					ComponentPose[Index].AddToTranslation(HandDelta);
					break;
				}
			}
		}

		ComponentPose[Fore].SetLocation(ForeNew);
		ComponentPose[Hand].SetLocation(HandNew);

		// 팔 뼈는 길이 축으로 늘려 메시가 같이 늘어남. 손은 그대로
		ComponentPose[Arm].SetScale3D(ComponentPose[Arm].GetScale3D() * BoneScale);
		ComponentPose[Fore].SetScale3D(ComponentPose[Fore].GetScale3D() * BoneScale);
		bChanged = true;
	}

	if (bChanged) { SkelMesh->MarkRenderDynamicDataDirty(); }
}

FVector ACTCharacter::GetArmStretchScale3D() const
{
	// 축 성분만 ArmStretch, 나머지 1
	const FVector Axis = ArmStretchAxis.GetClampedToMaxSize(1.0);
	return FVector::OneVector + Axis * (ArmStretch - 1.f);
}

FVector ACTCharacter::GetArmStretchHandScale3D() const
{
	const FVector Axis = ArmStretchAxis.GetClampedToMaxSize(1.0);
	return FVector::OneVector + Axis * ((1.f / FMath::Max(ArmStretch, 0.1f)) - 1.f);
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
	// 꼬리 잡기 대결 중에는 G가 연타 입력
	if (IsGrappling())
	{
		Grapple->LocalMash();
		return;
	}

	if (!CanUseActions()) { return; }

	// 쿨타임 중 서버 거절
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LocalLastGrabTime < GrabCooldownSeconds) { return; }
	LocalLastGrabTime = Now;

	// 서버 응답 전 선재생
	PlayActionMontage(ECTCharacterAction::Grab);
	ServerGrab();
}

void ACTCharacter::OnGrappleMouseClick()
{
	if (IsGrappling()) { Grapple->LocalMash(); }
}

void ACTCharacter::OnVoicePressed()
{
	bVoiceKeyHeld = true;
	UpdateVoiceTransmit();
}

void ACTCharacter::OnVoiceReleased()
{
	bVoiceKeyHeld = false;
	UpdateVoiceTransmit();
}

void ACTCharacter::UpdateVoiceTransmit()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !IsLocallyControlled()) { return; }

	// 마이크를 끈 설정이면 전송 안 함. 상시 말하기면 키 없이 전송
	const UCTSettingsSubsystem* Settings = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTSettingsSubsystem>() : nullptr;
	const bool bMicEnabled = !Settings || Settings->IsMicEnabled();
	const bool bAlwaysOn = Settings && Settings->IsMicAlwaysOn();
	const bool bWantTransmit = bMicEnabled && (bAlwaysOn || bVoiceKeyHeld);

	if (bWantTransmit == bVoiceTransmitting) { return; }
	PlayerController->ToggleSpeaking(bWantTransmit);
	bVoiceTransmitting = bWantTransmit;
}

void ACTCharacter::EnsureVoiceTalker()
{
	if (bVoiceTalkerCreated || GetNetMode() == NM_DedicatedServer) { return; }

	APlayerState* VoicePlayerState = GetPlayerState();
	if (!VoicePlayerState || VoicePlayerState->IsABot()) { return; } // AI는 목소리가 없음

	UVOIPTalker* Talker = UVOIPTalker::CreateTalkerForPlayer(VoicePlayerState);
	if (!Talker) { return; }
	bVoiceTalkerCreated = true;

	// 캐릭터 위치에서 들리고, 거리가 멀면 작아지다가 안 들림
	if (!VoiceAttenuation) { VoiceAttenuation = NewObject<USoundAttenuation>(this); }
	VoiceAttenuation->Attenuation.bAttenuate = true;
	VoiceAttenuation->Attenuation.bSpatialize = true;
	VoiceAttenuation->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
	VoiceAttenuation->Attenuation.AttenuationShapeExtents = FVector(VoiceInnerRadius, 0.f, 0.f);
	VoiceAttenuation->Attenuation.FalloffDistance = VoiceFalloffDistance;

	Talker->Settings.ComponentToAttachTo = GetRootComponent();
	Talker->Settings.AttenuationSettings = VoiceAttenuation;

	// 목소리가 안 들릴 때 거리 때문인지 확인하는 용도 (로그에 최대 크기 거리와 들리는 한계 거리)
	UE_LOG(LogTemp, Log, TEXT("[CT-Voice] %s 의 목소리: 최대 크기 %.0fcm, 들리는 한계 %.0fcm"),
		*VoicePlayerState->GetPlayerName(), VoiceInnerRadius, VoiceInnerRadius + VoiceFalloffDistance);
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
	if (CurrentBush || bBoothLocked || IsGrappling()) { return; }
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
	// 말하는 중에 사라지면 전송 중지
	if (IsLocallyControlled()) { OnVoiceReleased(); }

	// 이모트 창을 연 채로 사라지면 입력 막힘이 남지 않게 닫음
	if (EmoteMenu) { CloseEmoteMenu(); }

	GetWorldTimerManager().ClearTimer(GrabWindowStartTimerHandle);
	GetWorldTimerManager().ClearTimer(GrabWindowTickTimerHandle);

	if (ArmStretchPoseHandle.IsValid() && GetMesh())
	{
		GetMesh()->UnregisterOnBoneTransformsFinalizedDelegate(ArmStretchPoseHandle);
		ArmStretchPoseHandle.Reset();
	}

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

void ACTCharacter::AIGrab()
{
	if (HasAuthority()) { ServerGrab_Implementation(); }
}

void ACTCharacter::AIPush()
{
	if (HasAuthority()) { ServerPush_Implementation(); }
}

void ACTCharacter::AISetRun(bool bRun)
{
	if (!HasAuthority()) { return; }
	if (bRun) { TryStartRun(); } else { StopRun(/*bEndedNaturally=*/false); }
}

void ACTCharacter::ServerPush_Implementation()
{
	UWorld* World = GetWorld();
	const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	const ACTPlayerState* MyPlayerState = GetPlayerState<ACTPlayerState>();

	if (!CTGameState || !MyPlayerState) { return; }
	if (MyPlayerState->IsGhost()) { return; }
	if (CurrentBush || bBoothLocked || IsGrappling()) { return; }
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
		OtherCharacter->PlayKnockedDown(Direction);
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

	// 넘어진 뒤 일어나기 이어 재생 (앞/뒤에 맞는 일어나기)
	const bool bFellDown = Action == ECTCharacterAction::Pushed || Action == ECTCharacterAction::PushedBack;
	if (bFellDown && (GetUpMontage || GetUpBackMontage))
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
	// 뒤로 넘어진 몽타주면 뒤쪽 일어나기, 아니면 앞쪽 일어나기
	UAnimMontage* GetUp = (PushedBackMontage && Montage == PushedBackMontage) ? GetUpBackMontage.Get() : GetUpMontage.Get();
	if (bInterrupted || !GetUp || IsGhost()) { return; }
	PlayAnimMontage(GetUp, ActionMontagePlayRate);
}

// 이모트

void ACTCharacter::OnEmoteKeyPressed(int32 SlotIndex)
{
	RequestEmote(SlotIndex);
}

void ACTCharacter::SetEmoteLoadout(int32 KeyPosition, int32 LibraryIndex)
{
	if (!EmoteLoadout.IsValidIndex(KeyPosition) || !EmoteSlots.IsValidIndex(LibraryIndex)) { return; }
	EmoteLoadout[KeyPosition] = LibraryIndex;

	// 다음에도 쓰도록 저장
	if (UCTSettingsSubsystem* Settings = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTSettingsSubsystem>() : nullptr)
	{
		Settings->SaveEmoteLoadout(EmoteLoadout);
	}
}

void ACTCharacter::ToggleEmoteMenu()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!IsLocallyControlled() || !PlayerController) { return; }

	if (EmoteMenu && EmoteMenu->IsInViewport())
	{
		CloseEmoteMenu();
		return;
	}

	// 창 모양 WBP 가 없으면 열지 않음 (빈 화면에 입력만 막히는 것을 방지)
	if (!EmoteMenuClass) { return; }
	EmoteMenu = CreateWidget<UCTEmoteSelectWidget>(PlayerController, EmoteMenuClass);
	if (!EmoteMenu) { return; }
	if (!EmoteMenu->Setup(this))
	{
		EmoteMenu = nullptr; // 칸 WBP 를 못 불러오면 빈 창으로 입력만 막히지 않게 열지 않음
		return;
	}
	EmoteMenuController = PlayerController;
	EmoteMenu->OnCloseRequested.AddUObject(this, &ACTCharacter::CloseEmoteMenu);
	EmoteMenu->AddToViewport(30);

	// 이동과 시점을 막고 마우스를 보여줌 (원래 커서 상태는 닫을 때 되돌림)
	bEmoteMenuPrevCursor = PlayerController->bShowMouseCursor;
	PlayerController->SetIgnoreMoveInput(true);
	PlayerController->SetIgnoreLookInput(true);
	PlayerController->bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(EmoteMenu->TakeWidget());
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
}

void ACTCharacter::CloseEmoteMenu()
{
	// 캐릭터가 사라지는 중이면 컨트롤러가 이미 떨어졌을 수 있어 열 때 저장해 둔 것을 씀
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController) { PlayerController = EmoteMenuController.Get(); }
	EmoteMenuController.Reset();
	if (EmoteMenu)
	{
		EmoteMenu->RemoveFromParent();
		EmoteMenu = nullptr;
	}
	if (!PlayerController) { return; }

	PlayerController->SetIgnoreMoveInput(false);
	// 로비처럼 원래 시점이 고정이던 화면은 고정 유지
	PlayerController->bShowMouseCursor = bEmoteMenuPrevCursor;
	if (bEmoteMenuPrevCursor)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
	}
	else
	{
		PlayerController->SetIgnoreLookInput(false);
		PlayerController->SetInputMode(FInputModeGameOnly());
	}
}

void ACTCharacter::RequestEmote(int32 KeyPosition)
{
	// 키 자리 -> 이모트 목록 번호
	if (!EmoteLoadout.IsValidIndex(KeyPosition)) { return; }
	const int32 SlotIndex = EmoteLoadout[KeyPosition];
	if (!IsLocallyControlled() || !EmoteSlots.IsValidIndex(SlotIndex) || IsBoothLocked()) { return; }

	// 연타 방지 (서버도 다시 검사)
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LocalLastEmoteTime < EmoteCooldownSeconds) { return; }
	LocalLastEmoteTime = Now;

	ServerPlayEmote(SlotIndex);
}

void ACTCharacter::ServerPlayEmote_Implementation(int32 SlotIndex)
{
	if (!EmoteSlots.IsValidIndex(SlotIndex) || bBoothLocked) { return; }

	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastEmoteServerTime < EmoteCooldownSeconds * 0.5) { return; }
	LastEmoteServerTime = Now;

	MulticastPlayEmote(SlotIndex);
}

void ACTCharacter::MulticastPlayEmote_Implementation(int32 SlotIndex)
{
	if (!EmoteSlots.IsValidIndex(SlotIndex) || GetNetMode() == NM_DedicatedServer) { return; }
	const FCTEmoteSlot& Slot = EmoteSlots[SlotIndex];

	// 말풍선: 그림이 비어 있어도 빈 말풍선이 뜸
	if (EmoteBubble)
	{
		if (UTexture2D* SlotAtlas = Slot.GetAtlas())
		{
			// 아틀라스에서 이 칸만 잘라 씀
			const int32 Columns = FMath::Max(1, Slot.AtlasColumns);
			const int32 Rows = FMath::Max(1, Slot.AtlasRows);
			const int32 Column = Slot.AtlasCell % Columns;
			const int32 Row = FMath::Clamp(Slot.AtlasCell / Columns, 0, Rows - 1);
			const FVector2D CellSize(1.0 / Columns, 1.0 / Rows);
			const FVector2D UVMin(Column * CellSize.X, Row * CellSize.Y);
			EmoteBubble->ShowEmote(SlotAtlas, UVMin, UVMin + CellSize);
		}
		else
		{
			EmoteBubble->ShowEmote(Slot.Icon);
		}
	}

	// 동작: 서 있을 때만 (이동 중, 공중, 유령, 다른 동작 중이면 건너뜀)
	if ((Slot.Montage || Slot.Sequence) && !IsGhost() && !GetCharacterMovement()->IsFalling() && GetVelocity().Size2D() <= EmoteCancelSpeed)
	{
		UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
		const UAnimMontage* Active = AnimInstance ? AnimInstance->GetCurrentActiveMontage() : nullptr;
		const bool bBusy = Active && Active != ActiveEmoteMontage;
		if (AnimInstance && !bBusy)
		{
			if (Slot.Montage)
			{
				ActiveEmoteMontage = Slot.Montage;
				PlayAnimMontage(Slot.Montage, 1.f);
			}
			else
			{
				// 애니메이션을 바로 몽타주로 만들어 재생 (기존 몽타주와 같은 DefaultSlot)
				ActiveEmoteMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(Slot.Sequence, TEXT("DefaultSlot"), 0.2f, 0.2f, 1.f, 1);
			}
		}
	}
}

void ACTCharacter::UpdateEmoteMontage()
{
	if (!ActiveEmoteMontage) { return; }

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance || !AnimInstance->Montage_IsPlaying(ActiveEmoteMontage))
	{
		ActiveEmoteMontage = nullptr;
		return;
	}

	// 움직이거나 점프하면 동작을 끊음
	if (GetCharacterMovement()->IsFalling() || GetVelocity().Size2D() > EmoteCancelSpeed || IsGhost())
	{
		AnimInstance->Montage_Stop(ActiveEmoteMontage->BlendOut.GetBlendTime(), ActiveEmoteMontage);
		ActiveEmoteMontage = nullptr;
	}
}

void ACTCharacter::MulticastPlayHitSfx_Implementation(ECTSfx Sfx)
{
	if (GetNetMode() == NM_DedicatedServer) { return; }
	if (UCTAudioSubsystem* Audio = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTAudioSubsystem>() : nullptr)
	{
		Audio->PlaySfxAtLocation(Sfx, GetActorLocation());
	}
}

void ACTCharacter::PlayKnockedDown(const FVector& KnockDirection)
{
	if (!HasAuthority()) { return; }

	// 날아가는 방향이 보는 방향과 같으면 앞으로 엎어지고, 반대면 뒤로 넘어짐 (옆은 가까운 쪽)
	FVector Flat = KnockDirection;
	Flat.Z = 0.f;
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	const bool bFallForward = Flat.IsNearlyZero() || FVector::DotProduct(Flat.GetSafeNormal(), Forward) >= 0.f;

	MulticastPlayAction(bFallForward ? ECTCharacterAction::Pushed : ECTCharacterAction::PushedBack, true);
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

void ACTCharacter::ClientShowPhoneResult_Implementation(const FText& Message, float Seconds, bool bShowArrow, FVector TargetLocation)
{
	if (const ACTPlayerController* CTController = Cast<ACTPlayerController>(GetController()))
	{
		if (UCTHUDWidget* HUD = CTController->GetHUDWidget())
		{
			HUD->ShowNotice(Message, Seconds);
			if (bShowArrow) { HUD->ShowDirectionArrow(TargetLocation, Seconds); }
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
	if (GetNetMode() == NM_DedicatedServer || !XRayMesh || !GetMesh()) { return; }

	// 부쉬 안이면 내 몸, 같은 부쉬 안 다른 플레이어 몸이 가려져도 실루엣이 보임
	bool bWantXRay = false;
	if (CurrentBush && XRayMaterial)
	{
		if (IsLocallyControlled())
		{
			bWantXRay = true;
		}
		else
		{
			const APlayerController* LocalController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
			const ACTCharacter* Viewer = LocalController ? Cast<ACTCharacter>(LocalController->GetPawn()) : nullptr;
			bWantXRay = Viewer && Viewer->CurrentBush == CurrentBush;
		}
	}
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
