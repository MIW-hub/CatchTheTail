// Copyright Moon Party Games. All Rights Reserved.
#include "CTAnimNotifyState_GrabWindow.h"
#include "../Player/CTCharacter.h"
#include "Components/SkeletalMeshComponent.h"

void UCTAnimNotifyState_GrabWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	// 판정은 서버만 합니다. (캐릭터 쪽에서 서버가 아니면 무시)
	if (ACTCharacter* Character = MeshComp ? Cast<ACTCharacter>(MeshComp->GetOwner()) : nullptr)
	{
		Character->OnGrabNotifyBegin();
	}
}

void UCTAnimNotifyState_GrabWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (ACTCharacter* Character = MeshComp ? Cast<ACTCharacter>(MeshComp->GetOwner()) : nullptr)
	{
		Character->OnGrabNotifyEnd();
	}
}

FString UCTAnimNotifyState_GrabWindow::GetNotifyName_Implementation() const
{
	return TEXT("CT Grab Window");
}
