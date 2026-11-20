// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CTBroadcastCamera.generated.h"

class ACTCharacter;
class UCameraComponent;

// 홍보/플레이 영상용 중계 카메라 (개발용). AI 들이 실제 경기를 하는 동안 로컬 플레이어의 시점을 이 카메라로 바꿉니다.
// 켜는 법(명령줄): -CTBroadcast=1 (플레이 컷 편집) / -CTBroadcast=2 (공중 비행 촬영)  -CTAI=8 (AI 수)  -CTBroadcastHud=1 (HUD 보이기)
// 영상 만들기: Tools/CTRenderBroadcast.ps1
UCLASS()
class CATCHTHETAIL_API ACTBroadcastCamera : public AActor
{
	GENERATED_BODY()

public:
	ACTBroadcastCamera();

	// 0 = 플레이 컷 편집(추격, 쿼터뷰, 옆, 정면, 몸싸움), 1 = 공중 비행
	int32 Style = 0;
	// 공중 촬영 고도, 반경 배율 (맵이 클수록 키움). 명령줄 -CTBroadcastScale=2
	float AerialScale = 1.f;
	bool bHideHud = true;

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	void UpdateGameplay(float Dt);
	void UpdateAerial(float Dt);
	void Apply(const FVector& Target, const FRotator& Rot, float Fov, bool bSnap, float PosSpeed, float RotSpeed);
	FVector Unoccluded(const FVector& LookAt, const FVector& Wanted) const;
	void HideWidgets();

	UPROPERTY() TObjectPtr<UCameraComponent> Cam;
	UPROPERTY() TArray<TObjectPtr<ACTCharacter>> Chars;
	UPROPERTY() TObjectPtr<ACTCharacter> Hero;

	float Elapsed = 0.f;
	float HudTimer = 0.f;
	float LogTimer = 0.f;
	int32 LastShot = -1;
	FVector Centroid = FVector::ZeroVector;
	FVector Heading = FVector::ForwardVector;
	FVector SmoothFocus = FVector::ZeroVector;
	bool bFocusInit = false;
	bool bPoseInit = false;
	bool bFx1 = false;
	bool bFx2 = false;
};
