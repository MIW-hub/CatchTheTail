// Copyright Moon Party Games. All Rights Reserved.
#include "CTAnimNotify_Footstep.h"
#include "../Core/CTAudioSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void UCTAnimNotify_Footstep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	// 에디터 미리보기나 전용 서버에서는 소리를 내지 않음
	UWorld* World = MeshComp ? MeshComp->GetWorld() : nullptr;
	if (!World || !World->IsGameWorld() || World->GetNetMode() == NM_DedicatedServer) { return; }

	UGameInstance* GameInstance = World->GetGameInstance();
	UCTAudioSubsystem* Audio = GameInstance ? GameInstance->GetSubsystem<UCTAudioSubsystem>() : nullptr;
	if (!Audio) { return; }

	const FVector Location = (FootBoneName != NAME_None && MeshComp->DoesSocketExist(FootBoneName))
		? MeshComp->GetSocketLocation(FootBoneName)
		: MeshComp->GetComponentLocation();

	Audio->PlaySfxAtLocation(ECTSfx::Footstep, Location, Volume, FMath::FRandRange(PitchMin, FMath::Max(PitchMin, PitchMax)));
}

FString UCTAnimNotify_Footstep::GetNotifyName_Implementation() const
{
	return FootBoneName != NAME_None ? FString::Printf(TEXT("Footstep (%s)"), *FootBoneName.ToString()) : TEXT("Footstep");
}
