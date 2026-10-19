// Copyright Moon Party Games. All Rights Reserved.
#include "CTPlayerState.h"
#include "CTCharacter.h"
#include "Net/UnrealNetwork.h"

namespace
{
	// 사업계획서 2-1 색상 코드
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
	// PlayerState는 기본 갱신 주기가 느리므로 높여 줍니다. (유령/점수가 빨리 보이도록)
	SetNetUpdateFrequency(10.f);
}

void ACTPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACTPlayerState, AssignedColor);
	DOREPLIFETIME(ACTPlayerState, TargetColor);
	DOREPLIFETIME(ACTPlayerState, TailCount);
	DOREPLIFETIME(ACTPlayerState, bIsGhost);
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
	OnRep_Colors(); // 서버(방장)에서도 알림이 가도록 직접 호출
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

void ACTPlayerState::OnRep_Colors()
{
	// 캐릭터 몸 색을 바꿉니다.
	if (ACTCharacter* CTCharacter = Cast<ACTCharacter>(GetPawn()))
	{
		CTCharacter->RefreshBodyColor();
	}
	NotifyChanged();
}

void ACTPlayerState::OnRep_TailCount()
{
	// 꼬리가 늘거나 줄면 캐릭터 뒤의 꼬리 몸통 개수도 바꿉니다.
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

void ACTPlayerState::NotifyChanged()
{
	OnPlayerStateChanged.Broadcast(this);
}

void ACTPlayerState::NotifyPawnGhostState()
{
	// 이 플레이어의 캐릭터에게 "유령 상태가 바뀌었다"고 알려 줍니다.
	if (ACTCharacter* CTCharacter = Cast<ACTCharacter>(GetPawn()))
	{
		CTCharacter->OnGhostStateChanged(bIsGhost);
	}
}
