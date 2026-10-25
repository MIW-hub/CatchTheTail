// Copyright Moon Party Games. All Rights Reserved.
#include "CTPlayerState.h"
#include "../Core/CTGameState.h"
#include "../Core/CTAudioSubsystem.h"
#include "CTCharacter.h"
#include "Net/UnrealNetwork.h"

namespace
{
	// 사업계획서 2-1 색상
	FColor GetColorRGB(ECTColor InColor)
	{
		switch (InColor)
		{
		case ECTColor::Red:    return FColor(0xE5, 0x39, 0x35); // #E53935
		case ECTColor::Orange: return FColor(0xFB, 0x8C, 0x00); // #FB8C00
		case ECTColor::Yellow: return FColor(0xFD, 0xD8, 0x35); // #FDD835
		case ECTColor::Green:  return FColor(0x43, 0xA0, 0x47); // #43A047
		case ECTColor::Sky:    return FColor(0x29, 0xB6, 0xF6); // #29B6F6
		case ECTColor::Blue:   return FColor(0x2B, 0x3B, 0xFF); // #2B3BFF
		case ECTColor::Purple: return FColor(0x8E, 0x24, 0xAA); // #8E24AA
		case ECTColor::Pink:   return FColor(0xFF, 0x4F, 0xA3); // #FF4FA3
		default:               return FColor(0x80, 0x80, 0x80); // 색 없음: 회색
		}
	}
}

ACTPlayerState::ACTPlayerState()
{
	// 기본 갱신 주기 느려서 상향
	SetNetUpdateFrequency(10.f);
}

void ACTPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACTPlayerState, AssignedColor);
	DOREPLIFETIME(ACTPlayerState, TargetColor);
	DOREPLIFETIME(ACTPlayerState, TailCount);
	DOREPLIFETIME(ACTPlayerState, bIsGhost);
	DOREPLIFETIME(ACTPlayerState, bReady);
	DOREPLIFETIME(ACTPlayerState, bIsHost);
}

FLinearColor ACTPlayerState::GetLinearColorOf(ECTColor InColor)
{
	return FLinearColor::FromSRGBColor(GetColorRGB(InColor));
}

FText ACTPlayerState::GetColorDisplayName(ECTColor InColor)
{
	switch (InColor)
	{
	case ECTColor::Red:    return NSLOCTEXT("CT", "Color_Red", "빨강");
	case ECTColor::Orange: return NSLOCTEXT("CT", "Color_Orange", "주황");
	case ECTColor::Yellow: return NSLOCTEXT("CT", "Color_Yellow", "노랑");
	case ECTColor::Green:  return NSLOCTEXT("CT", "Color_Green", "초록");
	case ECTColor::Sky:    return NSLOCTEXT("CT", "Color_Sky", "하늘");
	case ECTColor::Blue:   return NSLOCTEXT("CT", "Color_Blue", "파랑");
	case ECTColor::Purple: return NSLOCTEXT("CT", "Color_Purple", "보라");
	case ECTColor::Pink:   return NSLOCTEXT("CT", "Color_Pink", "분홍");
	default:               return NSLOCTEXT("CT", "Color_None", "없음");
	}
}

void ACTPlayerState::SetColors(ECTColor NewAssignedColor, ECTColor NewTargetColor)
{
	if (!HasAuthority()) { return; }
	if (AssignedColor == NewAssignedColor && TargetColor == NewTargetColor) { return; }

	AssignedColor = NewAssignedColor;
	TargetColor = NewTargetColor;
	ForceNetUpdate();
	OnRep_Colors(); // 서버에서도 직접 호출
}

void ACTPlayerState::SetTailCount(int32 NewTailCount)
{
	if (!HasAuthority()) { return; }

	NewTailCount = FMath::Max(0, NewTailCount);
	if (TailCount == NewTailCount) { return; }

	TailCount = NewTailCount;
	ForceNetUpdate();
	OnRep_TailCount();
}

void ACTPlayerState::AddTailCount(int32 Delta)
{
	SetTailCount(TailCount + Delta);
}

void ACTPlayerState::SetIsGhost(bool bNewIsGhost)
{
	if (!HasAuthority()) { return; }
	if (bIsGhost == bNewIsGhost) { return; }

	bIsGhost = bNewIsGhost;
	ForceNetUpdate();
	OnRep_IsGhost();
}

void ACTPlayerState::SetReady(bool bNewReady)
{
	if (!HasAuthority()) { return; }
	if (bReady == bNewReady) { return; }

	bReady = bNewReady;
	ForceNetUpdate();
	OnRep_Lobby();
}

void ACTPlayerState::SetIsHost(bool bNewIsHost)
{
	if (!HasAuthority()) { return; }
	if (bIsHost == bNewIsHost) { return; }

	bIsHost = bNewIsHost;
	ForceNetUpdate();
	OnRep_Lobby();
}

void ACTPlayerState::OnRep_Lobby()
{
	NotifyChanged();
}

void ACTPlayerState::OnRep_Colors()
{
	// 캐릭터 몸 색 반영
	if (ACTCharacter* CTCharacter = Cast<ACTCharacter>(GetPawn()))
	{
		CTCharacter->RefreshBodyColor();
	}
	NotifyChanged();
}

void ACTPlayerState::OnRep_TailCount()
{
	// 게임 중 내 꼬리가 늘어나면 소리
	if (TailCount > LastTailCountForSound && IsLocalPlayerForSound())
	{
		const ACTGameState* CTGameState = GetWorld() ? GetWorld()->GetGameState<ACTGameState>() : nullptr;
		if (CTGameState && CTGameState->GetPhase() == ECTGamePhase::Playing)
		{
			if (UCTAudioSubsystem* Audio = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTAudioSubsystem>() : nullptr)
			{
				Audio->PlaySfx(ECTSfx::TailGained);
			}
		}
	}
	LastTailCountForSound = TailCount;

	// 꼬리 몸통 수 반영
	if (ACTCharacter* CTCharacter = Cast<ACTCharacter>(GetPawn()))
	{
		CTCharacter->RefreshTailVisual();
	}
	NotifyChanged();
}

void ACTPlayerState::OnRep_IsGhost()
{
	NotifyPawnGhostState();
	NotifyChanged();
}

bool ACTPlayerState::IsLocalPlayerForSound() const
{
	const APawn* ControlledPawn = GetPawn();
	return ControlledPawn && ControlledPawn->IsLocallyControlled();
}

void ACTPlayerState::NotifyChanged()
{
	OnPlayerStateChanged.Broadcast(this);
}

void ACTPlayerState::NotifyPawnGhostState()
{
	// 유령이었다가 부활하면 소리
	if (!bIsGhost && bLastGhostForSound && IsLocalPlayerForSound())
	{
		if (UCTAudioSubsystem* Audio = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTAudioSubsystem>() : nullptr)
		{
			Audio->PlaySfx(ECTSfx::GhostRespawn);
		}
	}
	bLastGhostForSound = bIsGhost;

	// 캐릭터에 유령 상태 통지
	if (ACTCharacter* CTCharacter = Cast<ACTCharacter>(GetPawn()))
	{
		CTCharacter->OnGhostStateChanged(bIsGhost);
	}
}
