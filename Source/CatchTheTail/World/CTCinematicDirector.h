// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CTCinematicDirector.generated.h"

class ACTCharacter;
class ACameraActor;
class UAnimMontage;

// 인트로/홍보 영상용 연출 감독. 레벨에 하나 놓으면 시작할 때 흰 닭과 초록 닭을 만들고 컷을 진행합니다 (게임 규칙 없이).
//  컷1: 아무도 없는 전경 → 컷2: 흰 닭이 달림(옆) → 컷3: 초록 닭이 쫓아감(옆) → 컷4: 정면에서 닭이 잡히는 순간까지
// 영상 만들기: Tools/CTRenderIntroCinematic.ps1
UCLASS()
class CATCHTHETAIL_API ACTCinematicDirector : public AActor
{
	GENERATED_BODY()

public:
	ACTCinematicDirector();

	// 달릴 길의 시작점 (닭 캡슐 중심 높이까지 포함). 길은 +X 방향으로 이어짐
	UPROPERTY(EditAnywhere, Category = "CT|Cinematic")
	FVector LaneStart = FVector::ZeroVector;

	// 바닥 높이 (카메라 높이 계산용)
	UPROPERTY(EditAnywhere, Category = "CT|Cinematic")
	float GroundZ = 45.f;

	// 옆 카메라가 서는 쪽 (-1 이면 -Y 쪽에서 +Y 를 봄, 1 이면 반대)
	UPROPERTY(EditAnywhere, Category = "CT|Cinematic")
	float SideSign = -1.f;

	UPROPERTY(EditAnywhere, Category = "CT|Cinematic")
	float SideDistance = 800.f;

	UPROPERTY(EditAnywhere, Category = "CT|Cinematic")
	TSubclassOf<ACTCharacter> CharacterClass;

	// 컷 길이(초)
	UPROPERTY(EditAnywhere, Category = "CT|Cinematic")
	float ShotSeconds[4] = { 2.5f, 2.7f, 2.8f, 8.0f };

	// 달리는 속도 (cm/s)
	UPROPERTY(EditAnywhere, Category = "CT|Cinematic")
	float RunSpeed = 650.f;

	// 잡는 쪽/잡히는 쪽 동작 (게임의 꼬리 대결 동작)
	UPROPERTY(EditAnywhere, Category = "CT|Cinematic")
	TObjectPtr<UAnimMontage> PullMontage;

	UPROPERTY(EditAnywhere, Category = "CT|Cinematic")
	TObjectPtr<UAnimMontage> FleeMontage;

	// 컷4에서 잡히는 순간 (컷4 시작 후 초)
	UPROPERTY(EditAnywhere, Category = "CT|Cinematic")
	float CatchAtSeconds = 2.4f;

	// 붙잡고 발버둥치는 시간(초)
	UPROPERTY(EditAnywhere, Category = "CT|Cinematic")
	float StruggleSeconds = 2.0f;

	// 이기고 난 뒤: 초록 닭이 넘어지는 동작 / 일어나는 동작
	UPROPERTY(EditAnywhere, Category = "CT|Cinematic")
	TObjectPtr<UAnimMontage> FallMontage;

	UPROPERTY(EditAnywhere, Category = "CT|Cinematic")
	TObjectPtr<UAnimMontage> GetUpMontage;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	ACTCharacter* SpawnChicken(const FLinearColor& Tint);
	ACameraActor* SpawnCamera(float Fov);
	void SetChickenVisible(ACTCharacter* Chicken, bool bVisible);
	void Place(ACTCharacter* Chicken, const FVector& Location, float Yaw);
	void Run(ACTCharacter* Chicken, const FVector& Direction, float Scale);
	void StartShot(int32 Shot);
	void UpdateShot(float ShotTime);

	UPROPERTY() TObjectPtr<ACTCharacter> White;
	UPROPERTY() TObjectPtr<ACTCharacter> Green;
	UPROPERTY() TObjectPtr<ACameraActor> Cams[4];

	float Elapsed = 0.f;
	int32 CurrentShot = -1;
	int32 Phase3 = 0;       // 컷4 진행: 0 달려옴, 1 발버둥, 2 넘어짐, 3 일어남, 4 쫓아감
	float Phase3Start = 0.f;
	float FallLen = 0.f;
	float GetUpLen = 0.f;
	FVector CatchPoint = FVector::ZeroVector;
	float LogTimer = 0.f;
};
