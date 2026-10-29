// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/CollisionProfile.h"
#include "CTFallingSpawnVolume.generated.h"

class UBoxComponent;
class ACTCharacter;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

// 낙하 액터 정보 (서버 결정, 복제)
USTRUCT()
struct FCTFallEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> Actor = nullptr;

	// Materials 인덱스
	UPROPERTY()
	int32 MaterialIndex = INDEX_NONE;

	// 바닥 위치 (XY 자리, Z 높이)
	UPROPERTY()
	FVector_NetQuantize GroundLocation = FVector::ZeroVector;

	// 시작 높이 (그림자 진하기용)
	UPROPERTY()
	float StartZ = 0.f;

	// 그림자 반지름(cm)
	UPROPERTY()
	float ShadowRadius = 50.f;

	UPROPERTY()
	int32 Id = 0;
};

// 착지 (서버)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCTFallingLandedSignature, AActor*, LandedActor, FVector, LandLocation);

// 캐릭터 적중 (서버)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCTFallingHitSignature, ACTCharacter*, HitCharacter, AActor*, FallingActor);

UCLASS()
class CATCHTHETAIL_API ACTFallingSpawnVolume : public AActor
{
	GENERATED_BODY()

public:
	ACTFallingSpawnVolume();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	// 스폰 시작 (서버)
	UFUNCTION(BlueprintCallable, Category = "CT|Falling")
	void StartSpawning();

	// 스폰 중지 (서버)
	UFUNCTION(BlueprintCallable, Category = "CT|Falling")
	void StopSpawning();

	// 즉시 하나 스폰 (서버)
	UFUNCTION(BlueprintCallable, Category = "CT|Falling")
	AActor* SpawnOne();

	UFUNCTION(BlueprintPure, Category = "CT|Falling")
	int32 GetFallingCount() const { return Entries.Num(); }

	// 착지 시 (서버)
	UPROPERTY(BlueprintAssignable, Category = "CT|Falling")
	FCTFallingLandedSignature OnActorLanded;

	// 캐릭터 적중 시 (서버)
	UPROPERTY(BlueprintAssignable, Category = "CT|Falling")
	FCTFallingHitSignature OnCharacterHit;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 컴포넌트
	// 스폰 영역. 맨 위 높이에서 낙하 시작
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Falling")
	TObjectPtr<UBoxComponent> SpawnBox;

	// 스폰
	// 무작위로 고를 액터 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Spawn")
	TArray<TSubclassOf<AActor>> ActorClasses;

	// 무작위로 고를 메시. 클래스와 한 묶음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Spawn")
	TArray<TObjectPtr<UStaticMesh>> StaticMeshes;

	// 메시 액터 크기 배율 최소
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Spawn", meta = (ClampMin = "0.01"))
	float StaticMeshScaleMin = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Spawn", meta = (ClampMin = "0.01"))
	float StaticMeshScaleMax = 1.f;

	// 스폰 간격 최소(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Spawn", meta = (ClampMin = "0.05"))
	float SpawnIntervalMin = 1.5f;

	// 스폰 간격 최대(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Spawn", meta = (ClampMin = "0.05"))
	float SpawnIntervalMax = 3.5f;

	// 동시 낙하 최대 수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Spawn", meta = (ClampMin = "1"))
	int32 MaxFallingActors = 8;

	// 시작 시 자동 스폰
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Spawn")
	bool bAutoStart = true;

	// Playing 단계에서만 스폰
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Spawn")
	bool bOnlySpawnWhilePlaying = true;

	// 스폰 시 Yaw 무작위
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Spawn")
	bool bRandomYaw = true;

	// 낙하
	// 낙하 속도 최소(cm/초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Fall", meta = (ClampMin = "1"))
	float FallSpeedMin = 200.f;

	// 낙하 속도 최대(cm/초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Fall", meta = (ClampMin = "1"))
	float FallSpeedMax = 350.f;

	// 시작 기울기 최대(도). 착지 때 반듯해짐
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Fall", meta = (ClampMin = "0", ClampMax = "80"))
	float FallTiltDegrees = 20.f;

	// 낙하 중 회전량 최대(도)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Fall", meta = (ClampMin = "0", ClampMax = "360"))
	float FallSpinDegrees = 40.f;

	// 착지 후 소멸까지(초), 0이면 유지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Fall", meta = (ClampMin = "0"))
	float LandedLifeSeconds = 8.f;

	// 바닥 탐색 채널
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Fall")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	// 충돌
	// 충돌 프로파일 직접 적용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Collision")
	bool bControlCollision = true;

	// 낙하 중 프로파일 (직접 이동이라 Block 금지)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Collision")
	FCollisionProfileName CollisionWhileFalling = FCollisionProfileName(TEXT("NoCollision"));

	// 착지 후 프로파일
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Collision")
	FCollisionProfileName CollisionAfterLanding = FCollisionProfileName(TEXT("BlockAll"));

	// 카메라 채널 무시
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Collision")
	bool bIgnoreCameraChannel = true;

	// 튕겨내기
	// 낙하 중 겹치면 튕겨냄
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Hit")
	bool bKnockbackOnHit = true;

	// 튕겨나는 거리(cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Hit", meta = (ClampMin = "0"))
	float HitKnockbackDistance = 300.f;

	// 튕김 상승 속도(cm/초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Hit", meta = (ClampMin = "1"))
	float HitKnockbackUpSpeed = 400.f;

	// 가로 속도 상한(cm/초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Hit", meta = (ClampMin = "0"))
	float HitKnockbackMaxSpeed = 1200.f;

	// 연속 적중 방지 대기(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Hit", meta = (ClampMin = "0"))
	float HitCooldownSeconds = 0.6f;

	// 적중 범위 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Hit", meta = (ClampMin = "0.1"))
	float HitRangeScale = 1.f;

	// 적중 시 밀려남 몽타주 재생
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Hit")
	bool bPlayPushedReaction = true;

	// 머티리얼
	// 무작위 적용 머티리얼, 비면 원본
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Material")
	TArray<TObjectPtr<UMaterialInterface>> Materials;

	// 적용 슬롯, -1이면 전부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Material", meta = (ClampMin = "-1"))
	int32 MaterialSlotIndex = -1;

	// 그림자
	// 그림자 표시
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Shadow")
	bool bShowShadow = true;

	// 그림자 메시
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Shadow")
	TObjectPtr<UStaticMesh> ShadowMesh;

	// 그림자 머티리얼
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Shadow")
	TObjectPtr<UMaterialInterface> ShadowMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Shadow")
	FLinearColor ShadowColor = FLinearColor(0.1f, 0.1f, 0.1f, 1.f);

	// 액터 반지름 대비 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Shadow", meta = (ClampMin = "0.1"))
	float ShadowRadiusMultiplier = 1.2f;

	// 시작 시 그림자 진하기(0~1)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Shadow", meta = (ClampMin = "0", ClampMax = "1"))
	float ShadowMinOpacity = 0.15f;

	// 착지 직전 진하기(0~1)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Shadow", meta = (ClampMin = "0", ClampMax = "1"))
	float ShadowMaxOpacity = 0.7f;

	// 시작 시 그림자 크기 비율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Shadow", meta = (ClampMin = "0.05", ClampMax = "1"))
	float ShadowStartScale = 0.6f;

	// 바닥 깜빡임 방지 높이(cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Shadow", meta = (ClampMin = "0"))
	float ShadowHeightOffset = 3.f;

	// 색 파라미터 이름(Vector)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Shadow")
	FName ShadowColorParameterName = TEXT("ShadowColor");

	// 진하기 파라미터 이름(Scalar)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Shadow")
	FName ShadowOpacityParameterName = TEXT("Opacity");

	// 낙하 목록 (복제)
	UPROPERTY(ReplicatedUsing = OnRep_Entries)
	TArray<FCTFallEntry> Entries;

	UFUNCTION()
	void OnRep_Entries();

private:
	// 클라이언트별 그림자/머티리얼 상태
	struct FLocalFall
	{
		TObjectPtr<UStaticMeshComponent> ShadowComponent = nullptr;
		TObjectPtr<UMaterialInstanceDynamic> ShadowMaterialInstance = nullptr;
		TWeakObjectPtr<AActor> Actor;
		bool bMaterialApplied = false;
	};

	void TickServer(float DeltaSeconds);
	void TickVisuals();

	// Entries 기준 그림자 갱신
	void SyncLocalVisuals();
	void DestroyLocalFall(FLocalFall& Fall);

	void ApplyMaterial(AActor* Actor, int32 MaterialIndex) const;

	// 모든 컴포넌트에 프로파일 적용
	void ApplyCollision(AActor* Actor, const FCollisionProfileName& Profile) const;

	// 서버: 겹친 캐릭터 튕김
	void ApplyHitKnockback(const FCTFallEntry& Entry, AActor* Falling, const FVector& BoundsOrigin, const FVector& BoundsExtent);

	// 서버: 바닥 높이 (실패 시 박스 바닥)
	float FindGroundZ(const FVector& From) const;

	float PickInterval() const;

	TMap<int32, FLocalFall> LocalFalls;

	// 서버 전용: 낙하 보간 값
	struct FServerFall
	{
		float Duration = 1.f;
		float Elapsed = 0.f;
		float Speed = 0.f;      // 로그용
		float StartZ = 0.f;     // 기준점 높이
		float EndZ = 0.f;
		FVector2D XY = FVector2D::ZeroVector;
		FQuat StartRotation = FQuat::Identity;
		FQuat EndRotation = FQuat::Identity;
	};

	TMap<int32, FServerFall> ServerFalls;

	// 서버: 캐릭터별 마지막 적중 시각
	TMap<TWeakObjectPtr<AActor>, double> LastHitTimes;

	int32 NextEntryId = 1;
	float TimeToNextSpawn = 1.f;
	bool bSpawning = false;
};
