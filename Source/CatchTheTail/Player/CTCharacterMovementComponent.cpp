// Copyright Moon Party Games. All Rights Reserved.
#include "CTCharacterMovementComponent.h"

float UCTCharacterMovementComponent::GetGravityZ() const
{
	const float Gravity = Super::GetGravityZ();
	// 내려가는 중에만 강하게
	return (IsFalling() && Velocity.Z < 0.f) ? Gravity * FallGravityMultiplier : Gravity;
}

float UCTCharacterMovementComponent::GetMaxAcceleration() const
{
	const float BaseAcceleration = Super::GetMaxAcceleration();
	if (!bUseAccelerationRamp || !IsMovingOnGround()) { return BaseAcceleration; }

	// 현재 속도 / 최대 속도 (0~1). 낮을수록 약한 가속 -> 느리게 출발해서 점점 빨라짐
	const float MaxSpeed = FMath::Max(1.f, GetMaxSpeed());
	const float SpeedRatio = FMath::Clamp(Velocity.Size2D() / MaxSpeed, 0.f, 1.f);
	const float Factor = FMath::Lerp(StartAccelerationFactor, 1.f, FMath::Pow(SpeedRatio, AccelerationRampExponent));
	return BaseAcceleration * Factor;
}
