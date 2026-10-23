// Copyright Moon Party Games. All Rights Reserved.
#include "CTAnimNotifyState_GrabWindow.h"
#include "../Player/CTCharacter.h"
#include "Components/SkeletalMeshComponent.h"

void UCTAnimNotifyState_GrabWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	// 판정은 서버만 (캐릭터에서 필터)
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
