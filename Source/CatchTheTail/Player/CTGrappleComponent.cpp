// Copyright Moon Party Games. All Rights Reserved.
#include "CTGrappleComponent.h"
#include "CTCharacter.h"
#include "CTPlayerState.h"
#include "../Core/CTGameMode.h"
#include "../Core/CTGameState.h"
#include "../Tail/CTTail.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Fonts/SlateFontInfo.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Animation/AnimSequence.h"
#include "Net/UnrealNetwork.h"
#include "Styling/CoreStyle.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

UCTGrappleComponent::UCTGrappleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	// 기본 애니메이션: 당기기(Grapple_Pull), 도망(Grapple_Flee). 에디터에서 바꿀 수 있음
	static ConstructorHelpers::FObjectFinder<UAnimSequence> GrabberFinder(TEXT("/Game/Character/Animation/Blender/Grapple_Pull.Grapple_Pull"));
	if (GrabberFinder.Succeeded()) { GrabberSequence = GrabberFinder.Object; }
	static ConstructorHelpers::FObjectFinder<UAnimSequence> VictimFinder(TEXT("/Game/Character/Animation/Blender/Grapple_Flee.Grapple_Flee"));
	if (VictimFinder.Succeeded()) { VictimSequence = VictimFinder.Object; }
}

void UCTGrappleComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCTGrappleComponent, Role);
	DOREPLIFETIME(UCTGrappleComponent, Partner);
	DOREPLIFETIME(UCTGrappleComponent, Gauge);
	DOREPLIFETIME(UCTGrappleComponent, EndServerTime);
}

ACTCharacter* UCTGrappleComponent::GetCharacter() const
{
	return Cast<ACTCharacter>(GetOwner());
}

double UCTGrappleComponent::ServerNow() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	return GameState ? GameState->GetServerWorldTimeSeconds() : (World ? World->GetTimeSeconds() : 0.0);
}

float UCTGrappleComponent::GetRemainingSeconds() const
{
	return IsGrappling() ? FMath::Max(0.f, static_cast<float>(EndServerTime - ServerNow())) : 0.f;
}

void UCTGrappleComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShowGaugeUI(false);

	// 대결 중에 사라지면 상대도 풀어 줌
	if (GetOwner() && GetOwner()->HasAuthority() && Partner)
	{
		if (UCTGrappleComponent* Other = Partner->GetGrapple())
		{
			if (Other->Partner == GetCharacter()) { Other->ClearState(); }
		}
	}

	Super::EndPlay(EndPlayReason);
}

// 시작 / 정리 (서버)

bool UCTGrappleComponent::BeginGrapple(ACTCharacter* Victim)
{
	ACTCharacter* Self = GetCharacter();
	if (!Self || !Victim || Self == Victim || !Self->HasAuthority()) { return false; }

	UCTGrappleComponent* Other = Victim->GetGrapple();
	if (!Other || IsGrappling() || Other->IsGrappling()) { return false; }

	// 같은 상대를 놓친 직후 다시 잡으면 시작 게이지가 줄어듦 (60 -> 55 -> 50)
	const double Now = ServerNow();
	int32 Count = 0;
	if (Other->LastGrabber.Get() == Self && Now - Other->LastEscapeTime <= RematchResetSeconds)
	{
		Count = Other->RematchCount;
	}
	const float Start = FMath::Clamp(FMath::Max(MinStartGauge, StartGauge - RematchStartStep * Count), 0.f, 1.f);
	const double End = Now + GrappleSeconds;

	// 잡은 사람이 보는 방향을 기준 축으로 (수평). 둘 다 이 방향을 보고 일직선으로 섬
	FVector Axis = Self->GetActorForwardVector();
	Axis.Z = 0.f;
	if (!Axis.Normalize())
	{
		Axis = Victim->GetActorLocation() - Self->GetActorLocation();
		Axis.Z = 0.f;
		if (!Axis.Normalize()) { Axis = FVector::ForwardVector; }
	}
	GrappleAxis = Axis;
	Other->GrappleAxis = Axis;

	SetState(ECTGrappleRole::Grabber, Victim, Start, End);
	Other->SetState(ECTGrappleRole::Victim, Self, Start, End);
	return true;
}

void UCTGrappleComponent::SetState(ECTGrappleRole NewRole, ACTCharacter* NewPartner, float NewGauge, double NewEndTime)
{
	ACTCharacter* Self = GetCharacter();
	if (!Self) { return; }

	Role = NewRole;
	Partner = NewPartner;
	Gauge = NewGauge;
	EndServerTime = NewEndTime;
	RecentMashTimes.Reset();
	AIMashAccum = 0.f;
	AIMashDelayLeft = AIMashStartDelaySeconds * FMath::FRandRange(0.7f, 1.3f);
	AIMashRate = FMath::FRandRange(FMath::Min(AIMashPerSecondMin, AIMashPerSecondMax), FMath::Max(AIMashPerSecondMin, AIMashPerSecondMax));

	// 이동을 멈추고 서로 마주봄
	Self->ForceResetRun();
	if (UCharacterMovementComponent* Move = Self->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
		bMovementLocked = true;
	}
	// 둘 다 잡은 사람이 보는 방향(기준 축)을 봄. 잡히는 쪽은 그 방향으로 도망가는 자세, 꼬리는 등 뒤에서 당겨짐
	if (NewPartner)
	{
		FRotator Look = GrappleAxis.Rotation();
		Look.Pitch = 0.f;
		Look.Roll = 0.f;
		Self->SetActorRotation(Look);
	}

	ApplyRoleEffects();
}

void UCTGrappleComponent::ClearState()
{
	ACTCharacter* Self = GetCharacter();

	Role = ECTGrappleRole::None;
	Partner = nullptr;
	Gauge = 0.5f;
	RecentMashTimes.Reset();
	AIMashAccum = 0.f;

	if (bMovementLocked && Self)
	{
		if (UCharacterMovementComponent* Move = Self->GetCharacterMovement())
		{
			Move->SetMovementMode(MOVE_Walking);
		}
	}
	bMovementLocked = false;

	ApplyRoleEffects();
}

// 입력 / 게이지

void UCTGrappleComponent::LocalMash()
{
	if (!IsGrappling()) { return; }
	ServerMash();
}

void UCTGrappleComponent::ServerMash_Implementation()
{
	ApplyMash();
}

void UCTGrappleComponent::ApplyMash()
{
	if (!IsGrappling() || !GetOwner() || !GetOwner()->HasAuthority()) { return; }

	ACTCharacter* OtherCharacter = Partner.Get();
	UCTGrappleComponent* Other = OtherCharacter ? OtherCharacter->GetGrapple() : nullptr;
	if (!Other || !Other->IsGrappling()) { return; }

	// 초당 연타 횟수 제한
	const double Now = ServerNow();
	RecentMashTimes.RemoveAll([Now](double Time) { return Now - Time > 1.0; });
	if (RecentMashTimes.Num() >= MaxMashPerSecond) { return; }
	RecentMashTimes.Add(Now);

	UCTGrappleComponent* GrabberComp = Role == ECTGrappleRole::Grabber ? this : Other;
	UCTGrappleComponent* VictimComp = Role == ECTGrappleRole::Grabber ? Other : this;

	const float Delta = Role == ECTGrappleRole::Grabber ? GrabberComp->GrabberGainPerMash : -VictimComp->VictimGainPerMash;
	const float NewGauge = FMath::Clamp(GrabberComp->Gauge + Delta, 0.f, 1.f);
	GrabberComp->Gauge = NewGauge;
	VictimComp->Gauge = NewGauge;

	if (NewGauge >= 1.f) { GrabberComp->Finish(true); }
	else if (NewGauge <= 0.f) { GrabberComp->Finish(false); }
}

void UCTGrappleComponent::Finish(bool bGrabberWon)
{
	UWorld* World = GetWorld();
	ACTCharacter* Grabber = GetCharacter();
	ACTCharacter* Victim = Partner.Get();
	if (!World || !Grabber || !Victim || Role != ECTGrappleRole::Grabber) { return; }

	UCTGrappleComponent* VictimComp = Victim->GetGrapple();
	ACTGameMode* GameMode = World->GetAuthGameMode<ACTGameMode>();
	ACTPlayerState* GrabberState = Grabber->GetPlayerState<ACTPlayerState>();
	ACTPlayerState* VictimState = Victim->GetPlayerState<ACTPlayerState>();

	ClearState();
	if (VictimComp) { VictimComp->ClearState(); }
	if (!VictimComp || !GameMode || !GrabberState || !VictimState) { return; }

	const double Now = ServerNow();
	if (bGrabberWon)
	{
		// 꼬리를 뺏음. 재도전 기록은 지움
		VictimComp->RematchCount = 0;
		VictimComp->LastGrabber = nullptr;
		GameMode->FinalizeTailGrabbed(GrabberState, VictimState);
	}
	else
	{
		// 뿌리침. 같은 상대를 곧 다시 잡으면 유리함이 줄어듦
		const bool bSameChain = VictimComp->LastGrabber.Get() == Grabber && Now - VictimComp->LastEscapeTime <= RematchResetSeconds;
		VictimComp->RematchCount = bSameChain ? VictimComp->RematchCount + 1 : 1;
		VictimComp->LastGrabber = Grabber;
		VictimComp->LastEscapeTime = Now;

		FVector Direction = Grabber->GetActorLocation() - Victim->GetActorLocation();
		Direction.Z = 0.f;
		Direction = Direction.GetSafeNormal();
		Grabber->LaunchCharacter(Direction * EscapeKnockbackSpeed + FVector(0.f, 0.f, EscapeKnockUpSpeed), /*bXYOverride=*/true, /*bZOverride=*/true);
		Grabber->PlayKnockedDown(Direction);
	}
}

// 틱

void UCTGrappleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ACTCharacter* Self = GetCharacter();
	if (!Self || !IsGrappling()) { return; }

	// 몽타주가 끝났거나 다른 동작에 끊기면 다시 재생 (모든 화면)
	if (Self->GetMesh())
	{
		if (const UAnimInstance* Anim = Self->GetMesh()->GetAnimInstance())
		{
			if (!ActiveMontage || !Anim->Montage_IsPlaying(ActiveMontage)) { StartMontage(); }
		}
	}

	// 도망치는 쪽: 앞서 갈수록 더 빨리 달리는 동작
	if (Role == ECTGrappleRole::Victim && ActiveMontage && Self->GetMesh())
	{
		if (UAnimInstance* Anim = Self->GetMesh()->GetAnimInstance())
		{
			Anim->Montage_SetPlayRate(ActiveMontage, FMath::Lerp(0.8f, 1.8f, FMath::Clamp(GetMyProgress(), 0.f, 1.f)));
		}
	}

	// 도망치는 쪽 꼬리: 위로 서 있지 않고 잡은 쪽 손을 향해 뻗음
	if (Role == ECTGrappleRole::Victim) { UpdateTailAim(DeltaTime); }

	// 잡은 쪽 팔: 상대가 뿌리칠수록(게이지가 낮을수록) 더 늘어남
	if (Role == ECTGrappleRole::Grabber)
	{
		Self->SetArmStretchTarget(FMath::Lerp(1.f, Self->GetArmStretchGrabScale(), FMath::Clamp(1.f - Gauge, 0.f, 1.f)));
	}

	if (!Self->HasAuthority()) { return; }

	// 상대가 사라졌거나 유령이 되었거나 경기가 끝났으면 중단
	UWorld* World = GetWorld();
	const ACTGameState* GameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	ACTCharacter* OtherCharacter = Partner.Get();
	UCTGrappleComponent* Other = OtherCharacter ? OtherCharacter->GetGrapple() : nullptr;
	const bool bBroken = !Other || !Other->IsGrappling() || Self->IsGhost() || OtherCharacter->IsGhost() ||
		!GameState || GameState->GetPhase() != ECTGamePhase::Playing;
	if (bBroken)
	{
		ClearState();
		if (Other && Other->Partner == Self) { Other->ClearState(); }
		return;
	}

	// 이기고 있는 쪽 위치로 진 쪽이 조금씩 끌려옴 (잡은 쪽 컴포넌트가 처리)
	if (Role == ECTGrappleRole::Grabber)
	{
		UpdatePullDrift(DeltaTime);
		UpdateFormation(DeltaTime);
	}

	// AI는 일정 속도로 자동 연타
	if (!Cast<APlayerController>(Self->GetController()))
	{
		// 반응 시간 동안은 연타하지 않음
		AIMashDelayLeft -= DeltaTime;
		if (AIMashDelayLeft > 0.f) { return; }
		AIMashAccum += DeltaTime * AIMashRate;
		while (AIMashAccum >= 1.f && IsGrappling())
		{
			AIMashAccum -= 1.f;
			ApplyMash();
		}
		if (!IsGrappling()) { return; }
	}

	// 시간 종료: 게이지가 높은 쪽이 이김 (잡은 쪽 컴포넌트가 결판)
	if (Role == ECTGrappleRole::Grabber && ServerNow() >= EndServerTime)
	{
		Finish(Gauge >= 0.5f);
	}
}

// 연출 (서버, 클라이언트 공통)

void UCTGrappleComponent::OnRep_Role()
{
	ApplyRoleEffects();
}

void UCTGrappleComponent::ApplyRoleEffects()
{
	ACTCharacter* Self = GetCharacter();
	if (!Self) { return; }

	UAnimInstance* Anim = Self->GetMesh() ? Self->GetMesh()->GetAnimInstance() : nullptr;

	if (IsGrappling())
	{
		if (Anim && ActiveMontage) { Anim->Montage_Stop(0.15f, ActiveMontage); }
		ActiveMontage = nullptr;
		StartMontage();

		// 내 화면의 내 캐릭터(사람이 조종하는 것)만 보임. AI끼리의 대결은 방장 화면에도 안 보여야 함
		// (AI도 서버에서는 "로컬 조종"으로 잡히므로 플레이어 컨트롤러인지까지 확인)
		if (bShowGaugeUI && Self->IsLocallyControlled() && Cast<APlayerController>(Self->GetController())) { ShowGaugeUI(true); }
	}
	else
	{
		if (Anim && ActiveMontage) { Anim->Montage_Stop(0.2f, ActiveMontage); }
		ActiveMontage = nullptr;
		ShowGaugeUI(false);
		Self->SetArmStretchTarget(1.f);

		// 꼬리를 원래 각도로 되돌림
		if (bTailAimed)
		{
			bTailAimed = false;
			Self->ApplyTailOffset();
		}
	}
}

void UCTGrappleComponent::UpdateTailAim(float DeltaTime)
{
	ACTCharacter* Self = GetCharacter();
	const ACTCharacter* Other = Partner.Get();
	ACTTail* Tail = Self ? Self->GetOwnedTail() : nullptr;
	if (!Tail || !Other || !Other->GetMesh()) { return; }

	// 잡은 쪽 두 손의 가운데
	const USkeletalMeshComponent* OtherMesh = Other->GetMesh();
	const FVector Target = (OtherMesh->GetSocketLocation(TEXT("LeftHand")) + OtherMesh->GetSocketLocation(TEXT("RightHand"))) * 0.5f;

	const FVector Direction = (Target - Tail->GetActorLocation()).GetSafeNormal();
	if (Direction.IsNearlyZero()) { return; }

	// 꼬리의 위쪽(Z) 축이 손을 향함. 옆(Y) 축은 캐릭터의 오른쪽 유지
	const FQuat Wanted = FRotationMatrix::MakeFromZY(Direction, Self->GetActorRightVector()).ToQuat();
	Tail->SetActorRotation(FMath::QInterpTo(Tail->GetActorQuat(), Wanted, DeltaTime, TailAimInterpSpeed));
	bTailAimed = true;
}

void UCTGrappleComponent::UpdateFormation(float DeltaTime)
{
	ACTCharacter* Self = GetCharacter();
	ACTCharacter* OtherCharacter = Partner.Get();
	if (!Self || !OtherCharacter) { return; }

	// 둘 다 같은 방향을 봄 (다른 동작이 돌려놓아도 매 틱 되돌림)
	FRotator Look = GrappleAxis.Rotation();
	Look.Pitch = 0.f;
	Look.Roll = 0.f;
	Self->SetActorRotation(Look);
	OtherCharacter->SetActorRotation(Look);

	// 잡힌 쪽을 잡은 사람 바로 앞 일직선으로 부드럽게 맞춤 (옆으로 벗어난 만큼, 뒤쪽이면 앞으로)
	FVector Relative = OtherCharacter->GetActorLocation() - Self->GetActorLocation();
	Relative.Z = 0.f;
	const float Along = FVector::DotProduct(Relative, GrappleAxis);
	// 멀리 떨어져 있으면 GrappleHoldDistance 까지 붙고, 그 안쪽은 끌려오는 움직임(최소 거리까지)에 맡김
	const float WantedAlong = Along > GrappleHoldDistance ? GrappleHoldDistance : FMath::Max(Along, MinPullDistance);
	FVector Wanted = GrappleAxis * WantedAlong;
	const FVector Correction = Wanted - Relative;
	if (Correction.SizeSquared() > 1.f)
	{
		OtherCharacter->AddActorWorldOffset(Correction * FMath::Min(1.f, DeltaTime * 8.f), /*bSweep=*/true);
	}
}

void UCTGrappleComponent::UpdatePullDrift(float DeltaTime)
{
	ACTCharacter* Self = GetCharacter();
	ACTCharacter* OtherCharacter = Partner.Get();
	if (!Self || !OtherCharacter || FMath::IsNearlyEqual(Gauge, 0.5f, 0.01f)) { return; }

	// 게이지가 높으면 잡은 쪽이 이기는 중
	ACTCharacter* Winner = Gauge > 0.5f ? Self : OtherCharacter;
	ACTCharacter* Loser = Gauge > 0.5f ? OtherCharacter : Self;

	FVector ToWinner = Winner->GetActorLocation() - Loser->GetActorLocation();
	ToWinner.Z = 0.f;
	const float Distance = ToWinner.Size();
	const float Step = FMath::Min(Distance * PullFractionPerSecond * DeltaTime, FMath::Max(0.f, Distance - MinPullDistance));
	if (Step > KINDA_SMALL_NUMBER)
	{
		Loser->AddActorWorldOffset(ToWinner.GetSafeNormal() * Step, /*bSweep=*/true);
	}
}

void UCTGrappleComponent::StartMontage()
{
	ACTCharacter* Self = GetCharacter();
	UAnimInstance* Anim = (Self && Self->GetMesh()) ? Self->GetMesh()->GetAnimInstance() : nullptr;
	if (!Anim) { return; }

	const bool bGrabber = Role == ECTGrappleRole::Grabber;
	UAnimMontage* Montage = bGrabber ? GrabberMontage.Get() : VictimMontage.Get();
	if (Montage)
	{
		ActiveMontage = Montage;
		Anim->Montage_Play(Montage);

		// 끝나면 처음 섹션으로 이어서 끊김 없이 반복 (대결이 끝날 때까지)
		if (Montage->GetNumSections() > 0)
		{
			const FName FirstSection = Montage->GetSectionName(0);
			Anim->Montage_SetNextSection(FirstSection, FirstSection, Montage);
		}
		return;
	}

	// 몽타주가 없으면 애니메이션을 바로 몽타주로 만들어 반복 재생
	UAnimSequence* Sequence = bGrabber ? GrabberSequence.Get() : VictimSequence.Get();
	if (Sequence)
	{
		ActiveMontage = Anim->PlaySlotAnimationAsDynamicMontage(Sequence, AnimSlotName, 0.15f, 0.15f, 1.f, 1000);
	}
}

void UCTGrappleComponent::ShowGaugeUI(bool bShow)
{
	UWorld* World = GetWorld();
	UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;

	if (!bShow)
	{
		if (GaugeWidget.IsValid() && Viewport) { Viewport->RemoveViewportWidgetContent(GaugeWidget.ToSharedRef()); }
		GaugeWidget.Reset();
		return;
	}
	if (!Viewport || GaugeWidget.IsValid()) { return; }

	// 프로젝트 UI 이미지로 꾸밈 (크림색 알약 막대, 잡기 아이콘). 패널(틀) 그림은 없음
	UTexture2D* PillTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/2D/UI/Style/UI_Pill_Status.UI_Pill_Status"));
	UTexture2D* GrabIconTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/2D/UI/ico/Ico_Grab.Ico_Grab"));
	UTexture2D* MouseIconTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/2D/UI/ico/ico_mouse-left.ico_mouse-left"));
	GaugeTextures = { PillTexture, GrabIconTexture, MouseIconTexture };

	auto MakeBox = [](UTexture2D* Texture, FMargin Margin) -> TSharedPtr<FSlateBrush>
	{
		TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateBrush>();
		if (Texture)
		{
			Brush->SetResourceObject(Texture);
			Brush->ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
			Brush->DrawAs = ESlateBrushDrawType::Box;
			Brush->Margin = Margin;
		}
		else
		{
			Brush->DrawAs = ESlateBrushDrawType::RoundedBox;
		}
		return Brush;
	};
	auto MakeImage = [](UTexture2D* Texture, float Size) -> TSharedPtr<FSlateBrush>
	{
		TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateBrush>();
		if (Texture) { Brush->SetResourceObject(Texture); }
		Brush->ImageSize = FVector2D(Size, Size);
		return Brush;
	};

	TrackBrush = MakeBox(PillTexture, FMargin(0.20f, 0.45f, 0.20f, 0.30f));
	FillBrush = MakeBox(PillTexture, FMargin(0.20f, 0.45f, 0.20f, 0.30f));
	GrabIconBrush = MakeImage(GrabIconTexture, 30.f);
	MouseIconBrush = MakeImage(MouseIconTexture, 22.f);

	BarStyle = MakeShared<FProgressBarStyle>();
	BarStyle->SetBackgroundImage(*TrackBrush);
	BarStyle->SetFillImage(*FillBrush);
	BarStyle->SetMarqueeImage(*FillBrush);
	BarStyle->SetEnableFillAnimation(false);

	const TWeakObjectPtr<UCTGrappleComponent> Weak(this);

	GaugeWidget = SNew(SBox)
		.Visibility(EVisibility::HitTestInvisible)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.f, 0.f, 0.f, 140.f))
		[
			SNew(SBox)
			.WidthOverride(560.f)
			.HeightOverride(110.f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SVerticalBox)
					// 키 아이콘과 남은 시간
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBox)
						.HeightOverride(44.f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								SNew(SImage).Image(GrabIconBrush.Get())
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(8.f, 0.f, 0.f, 0.f))
							[
								SNew(STextBlock)
								.Text(INVTEXT("G"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 26))
								.ColorAndOpacity(FLinearColor::White)
								.ShadowOffset(FVector2D(1.f, 2.f))
								.ShadowColorAndOpacity(FLinearColor(0.25f, 0.f, 0.2f, 0.9f))
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(10.f, 0.f, 0.f, 0.f))
							[
								SNew(SImage).Image(MouseIconBrush.Get())
							]
							+ SHorizontalBox::Slot().FillWidth(1.f)
							[
								SNullWidget::NullWidget
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(TAttribute<FText>::CreateLambda([Weak]() -> FText
								{
									return Weak.IsValid() ? FText::FromString(FString::Printf(TEXT("%.1f"), Weak->GetRemainingSeconds())) : FText::GetEmpty();
								}))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))
								.ColorAndOpacity(FLinearColor::White)
								.ShadowOffset(FVector2D(1.f, 2.f))
								.ShadowColorAndOpacity(FLinearColor(0.25f, 0.f, 0.2f, 0.9f))
							]
						]
					]
					// 흰 몸통: 내 쪽 게이지 (이기는 중이면 노랑, 밀리면 분홍)
					+ SVerticalBox::Slot()
					.FillHeight(1.f)
					.VAlign(VAlign_Center)
					.Padding(FMargin(4.f, 6.f, 4.f, 0.f))
					[
						SNew(SBox)
						.HeightOverride(52.f)
						[
							SNew(SProgressBar)
							.Style(BarStyle.Get())
							.Percent(TAttribute<TOptional<float>>::CreateLambda([Weak]() -> TOptional<float>
							{
								return Weak.IsValid() ? TOptional<float>(FMath::Max(0.12f, Weak->GetMyProgress())) : TOptional<float>(0.12f);
							}))
							.FillColorAndOpacity(TAttribute<FSlateColor>::CreateLambda([Weak]() -> FSlateColor
							{
								const bool bWinning = Weak.IsValid() && Weak->GetMyProgress() >= 0.5f;
								return FSlateColor(bWinning ? FLinearColor(1.f, 0.78f, 0.f, 1.f) : FLinearColor(1.f, 0.35f, 0.55f, 1.f));
							}))
						]
					]
				]
			]
		];

	Viewport->AddViewportWidgetContent(GaugeWidget.ToSharedRef(), 45);
}
