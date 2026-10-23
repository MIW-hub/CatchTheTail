// Copyright Moon Party Games. All Rights Reserved.
#include "CTAnimInstance.h"
#include "../Player/CTCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UCTAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwnerCharacter = Cast<ACTCharacter>(TryGetPawnOwner());
	MovementComponent = OwnerCharacter ? OwnerCharacter->GetCharacterMovement() : nullptr;
}

void UCTAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 에디터 미리보기에서 늦게 잡힘
	if (!OwnerCharacter || !MovementComponent)
	{
		NativeInitializeAnimation();
		if (!OwnerCharacter || !MovementComponent)
		{
			return;
		}
	}

	// 이동
	Velocity = OwnerCharacter->GetVelocity();
	GroundSpeed = Velocity.Size2D();
	bShouldMove = GroundSpeed > MoveThreshold;

	const float BaseWalkSpeed = OwnerCharacter->GetBaseWalkSpeed();
	NormalizedSpeed = BaseWalkSpeed > KINDA_SMALL_NUMBER ? GroundSpeed / BaseWalkSpeed : 0.f;

	MovePlayRate = bShouldMove
		? FMath::Clamp(GroundSpeed / AnimReferenceSpeed, MinMovePlayRate, MaxMovePlayRate)
		: 1.f;

	// 점프 / 공중
	bIsFalling = MovementComponent->IsFalling();
	VerticalVelocity = Velocity.Z;
	bIsJumping = bIsFalling && VerticalVelocity > 0.f;
	bIsFallingDown = bIsFalling && VerticalVelocity <= 0.f;
	TimeInAir = bIsFalling ? TimeInAir + DeltaSeconds : 0.f;

	// 캐릭터 상태
	bIsRunning = OwnerCharacter->GetRunState() == ECTRunState::Running;
	bIsGhost = OwnerCharacter->IsGhost();
	bIsPlayingAction = IsAnyMontagePlaying();
}
