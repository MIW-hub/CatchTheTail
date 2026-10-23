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

/** 떨어지는 액터 하나의 정보. 서버가 정하고 모든 화면에 복제합니다. */
USTRUCT()
struct FCTFallEntry
{
	GENERATED_BODY()

	/** 떨어지는 액터 */
	UPROPERTY()
	TObjectPtr<AActor> Actor = nullptr;

	/** 이 액터에 적용할 머티리얼 번호 (Materials 배열의 인덱스, 없으면 INDEX_NONE) */
	UPROPERTY()
	int32 MaterialIndex = INDEX_NONE;

	/** 그림자가 놓일 바닥 위치 (XY = 떨어질 자리, Z = 바닥 높이) */
	UPROPERTY()
	FVector_NetQuantize GroundLocation = FVector::ZeroVector;

	/** 떨어지기 시작한 높이 (그림자 진하기 계산용) */
	UPROPERTY()
	float StartZ = 0.f;

	/** 그림자 반지름(cm) */
	UPROPERTY()
	float ShadowRadius = 50.f;

	/** 구분용 번호 */
	UPROPERTY()
	int32 Id = 0;
};

/** 액터가 바닥에 닿았을 때 (서버에서만 호출됨) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCTFallingLandedSignature, AActor*, LandedActor, FVector, LandLocation);

/** 떨어지는 액터가 캐릭터를 쳤을 때 (서버에서만 호출됨) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCTFallingHitSignature, ACTCharacter*, HitCharacter, AActor*, FallingActor);

UCLASS()
class CATCHTHETAIL_API ACTFallingSpawnVolume : public AActor
{
	GENERATED_BODY()

public:
	ACTFallingSpawnVolume();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	/** 스폰을 시작합니다. (서버에서만 동작) */
	UFUNCTION(BlueprintCallable, Category = "CT|Falling")
	void StartSpawning();

	/** 스폰을 멈춥니다. 이미 떨어지는 중인 것은 계속 떨어집니다. (서버에서만 동작) */
	UFUNCTION(BlueprintCallable, Category = "CT|Falling")
	void StopSpawning();

	/** 지금 하나를 바로 스폰합니다. 성공하면 그 액터 (서버에서만 동작) */
	UFUNCTION(BlueprintCallable, Category = "CT|Falling")
	AActor* SpawnOne();

	/** 지금 떨어지는 중인 액터 수 */
	UFUNCTION(BlueprintPure, Category = "CT|Falling")
	int32 GetFallingCount() const { return Entries.Num(); }

	/** 바닥에 닿았을 때 (서버) */
	UPROPERTY(BlueprintAssignable, Category = "CT|Falling")
	FCTFallingLandedSignature OnActorLanded;

	/** 떨어지는 액터가 캐릭터를 쳐서 튕겨냈을 때 (서버) */
	UPROPERTY(BlueprintAssignable, Category = "CT|Falling")
	FCTFallingHitSignature OnCharacterHit;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ===== 컴포넌트 =====
	/** 스폰 영역. 박스의 맨 위 높이에서 떨어지기 시작합니다. 위치, 회전, 크기를 조절해서 영역을 맞추세요. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Falling")
	TObjectPtr<UBoxComponent> SpawnBox;

	// ===== 스폰 =====
	/** 떨어뜨릴 액터 클래스들. 스폰할 때마다 이 중에서 무작위로 하나를 고릅니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Spawn")
	TArray<TSubclassOf<AActor>> ActorClasses;

	/**
	 * 떨어뜨릴 스태틱 메시들. 메시를 넣으면 볼륨이 액터를 만들어서 그 메시를 넣고 떨어뜨립니다.
	 * Actor Classes와 함께 한 묶음으로 보고 무작위로 하나를 고릅니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Spawn")
	TArray<TObjectPtr<UStaticMesh>> StaticMeshes;

	/** 스태틱 메시로 만든 액터의 크기 배율 최소 (스폰할 때마다 최소~최대 사이에서 무작위) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Spawn", meta = (ClampMin = "0.01"))
	float StaticMeshScaleMin = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Spawn", meta = (ClampMin = "0.01"))
	float StaticMeshScaleMax = 1.f;

	/** 다음 스폰까지 걸리는 시간(초)의 최소 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Spawn", meta = (ClampMin = "0.05"))
	float SpawnIntervalMin = 1.5f;

	/** 다음 스폰까지 걸리는 시간(초)의 최대 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Spawn", meta = (ClampMin = "0.05"))
	float SpawnIntervalMax = 3.5f;

	/** 동시에 떨어지는 액터의 최대 수. 이 수에 닿으면 하나가 내려앉을 때까지 스폰을 기다립니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Spawn", meta = (ClampMin = "1"))
	int32 MaxFallingActors = 8;

	/** 게임이 시작되면 자동으로 스폰을 시작할까? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Spawn")
	bool bAutoStart = true;

	/** true면 경기 중(Playing 단계)에만 스폰합니다. 대기, 카운트다운, 종료 후에는 스폰하지 않음 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Spawn")
	bool bOnlySpawnWhilePlaying = true;

	/** 스폰할 때 좌우 방향(Yaw)을 무작위로 돌릴까? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Spawn")
	bool bRandomYaw = true;

	// ===== 떨어짐 =====
	/**
	 * 떨어지는 속도(cm/초)의 최소. 천천히 떨어지게 하려면 작게.
	 * 속도로 걸리는 시간(높이 ÷ 속도)을 구한 뒤, 시작 높이에서 바닥까지 선형보간(Lerp)으로 내려옵니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Fall", meta = (ClampMin = "1"))
	float FallSpeedMin = 200.f;

	/** 떨어지는 속도(cm/초)의 최대. 액터마다 최소~최대 사이에서 무작위 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Fall", meta = (ClampMin = "1"))
	float FallSpeedMax = 350.f;

	/**
	 * 떨어지기 시작할 때 박스가 기울어진 정도(도)의 최대. 앞뒤, 좌우로 무작위로 기울어진 채 시작해서
	 * 바닥에 닿을 때는 반듯해집니다. (물리처럼 보이게 하는 연출, 0이면 기울지 않음)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Fall", meta = (ClampMin = "0", ClampMax = "80"))
	float FallTiltDegrees = 20.f;

	/** 떨어지는 동안 좌우로 더 돌아가는 정도(도)의 최대. 시작 방향에서 ±이만큼 돌며 내려옵니다. 0이면 돌지 않음 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Fall", meta = (ClampMin = "0", ClampMax = "360"))
	float FallSpinDegrees = 40.f;

	/** 바닥에 닿은 뒤 액터가 사라지기까지의 시간(초). 0이면 사라지지 않음 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Fall", meta = (ClampMin = "0"))
	float LandedLifeSeconds = 8.f;

	/** 바닥을 찾을 때 쓰는 충돌 채널 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Fall")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	// ===== 충돌 =====
	/**
	 * true면 스폰한 액터의 충돌을 아래 두 프로파일로 정합니다. (모든 컴포넌트에 적용)
	 * false면 액터가 원래 가진 충돌 설정을 그대로 씁니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Collision")
	bool bControlCollision = true;

	/**
	 * 떨어지는 동안의 충돌 프로파일. 볼륨이 액터를 충돌 검사 없이 직접 내리기 때문에,
	 * 막는(Block) 설정이면 캐릭터와 겹친 채로 내려올 수 있습니다. 기본은 NoCollision(부딪히지 않음)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Collision")
	FCollisionProfileName CollisionWhileFalling = FCollisionProfileName(TEXT("NoCollision"));

	/** 바닥에 닿은 뒤의 충돌 프로파일. 기본은 BlockAll(캐릭터와 물체를 막음) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Collision")
	FCollisionProfileName CollisionAfterLanding = FCollisionProfileName(TEXT("BlockAll"));

	/**
	 * true면 스폰한 액터가 카메라(Camera 채널)를 막지 않게 합니다.
	 * 박스가 캐릭터와 카메라 사이에 있어도 카메라가 갑자기 당겨지거나 튀지 않습니다. (충돌 프로파일을 적용한 뒤에 덮어씀)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Collision")
	bool bIgnoreCameraChannel = true;

	// ===== 캐릭터 충돌 (튕겨내기) =====
	/**
	 * true면 떨어지는 액터가 캐릭터와 겹치는 순간 캐릭터를 튕겨냅니다.
	 * 떨어지는 동안만 적용되고, 착지한 액터는 그냥 막는 물체입니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Hit")
	bool bKnockbackOnHit = true;

	/**
	 * 튕겨나가는 거리(cm). 이 거리만큼 날아가도록 속도를 계산하므로, 맞은 위치와 상관없이 항상 비슷하게 날아갑니다.
	 * (캐릭터가 공중에서 조작하면 조금 달라질 수 있음)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Hit", meta = (ClampMin = "0"))
	float HitKnockbackDistance = 300.f;

	/** 튕겨날 때 위로 뜨는 속도(cm/초). 클수록 오래 떠 있다가 떨어지고, 같은 거리라도 천천히 날아갑니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Hit", meta = (ClampMin = "1"))
	float HitKnockbackUpSpeed = 400.f;

	/** 안전장치: 가로 속도의 최대(cm/초). 거리를 크게 잡아도 이 속도를 넘지 않습니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Hit", meta = (ClampMin = "0"))
	float HitKnockbackMaxSpeed = 1200.f;

	/** 같은 캐릭터가 연달아 맞지 않도록 쉬는 시간(초). 박스 여러 개가 겹쳐 내려와도 한 번만 튕겨냅니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Hit", meta = (ClampMin = "0"))
	float HitCooldownSeconds = 0.6f;

	/** 맞는 범위 배율. 1이면 액터의 눈에 보이는 크기, 크면 더 넓게 맞습니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Hit", meta = (ClampMin = "0.1"))
	float HitRangeScale = 1.f;

	/** 맞은 캐릭터에게 "밀려남" 몽타주(Pushed Montage)를 재생할까? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Hit")
	bool bPlayPushedReaction = true;

	// ===== 머티리얼 =====
	/** 스폰할 때마다 이 중에서 무작위로 하나를 골라 액터에 적용합니다. 비워 두면 원래 머티리얼 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Material")
	TArray<TObjectPtr<UMaterialInterface>> Materials;

	/** 적용할 머티리얼 슬롯 번호. -1이면 액터의 모든 메시의 모든 슬롯 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Material", meta = (ClampMin = "-1"))
	int32 MaterialSlotIndex = -1;

	// ===== 그림자 =====
	/** 떨어질 자리에 그림자를 보여줄까? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Shadow")
	bool bShowShadow = true;

	/** 그림자 모양 메시 (기본: 엔진의 Plane. 가운데가 동그랗게 흐려지는 머티리얼을 씁니다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Shadow")
	TObjectPtr<UStaticMesh> ShadowMesh;

	/** 그림자 머티리얼 (기본: M_FallingShadow). 아래 두 파라미터 이름을 가진 머티리얼이어야 색과 진하기가 바뀝니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Shadow")
	TObjectPtr<UMaterialInterface> ShadowMaterial;

	/** 그림자 색 (회색) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Shadow")
	FLinearColor ShadowColor = FLinearColor(0.1f, 0.1f, 0.1f, 1.f);

	/** 그림자 크기 배율. 1이면 액터의 가로 크기(반지름)만큼 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Shadow", meta = (ClampMin = "0.1"))
	float ShadowRadiusMultiplier = 1.2f;

	/** 떨어지기 시작할 때 그림자의 진하기 (0~1) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Shadow", meta = (ClampMin = "0", ClampMax = "1"))
	float ShadowMinOpacity = 0.15f;

	/** 바닥에 닿기 직전 그림자의 진하기 (0~1) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Shadow", meta = (ClampMin = "0", ClampMax = "1"))
	float ShadowMaxOpacity = 0.7f;

	/** 떨어지기 시작할 때 그림자 크기 (최종 크기 대비 0~1). 가까워질수록 커집니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Shadow", meta = (ClampMin = "0.05", ClampMax = "1"))
	float ShadowStartScale = 0.6f;

	/** 그림자를 바닥에서 띄우는 높이(cm). 바닥과 겹쳐서 깜빡이는 것을 막습니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Shadow", meta = (ClampMin = "0"))
	float ShadowHeightOffset = 3.f;

	/** 그림자 머티리얼의 색 파라미터 이름 (Vector) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Shadow")
	FName ShadowColorParameterName = TEXT("ShadowColor");

	/** 그림자 머티리얼의 진하기 파라미터 이름 (Scalar) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Falling|Shadow")
	FName ShadowOpacityParameterName = TEXT("Opacity");

	/** 지금 떨어지는 액터들. 서버가 정하고 복제합니다. (머티리얼 적용과 그림자에 쓰임) */
	UPROPERTY(ReplicatedUsing = OnRep_Entries)
	TArray<FCTFallEntry> Entries;

	UFUNCTION()
	void OnRep_Entries();

private:
	/** 한 화면에서 그림자와 머티리얼을 맞춥니다. */
	struct FLocalFall
	{
		TObjectPtr<UStaticMeshComponent> ShadowComponent = nullptr;
		TObjectPtr<UMaterialInstanceDynamic> ShadowMaterialInstance = nullptr;
		TWeakObjectPtr<AActor> Actor;
		bool bMaterialApplied = false;
	};

	void TickServer(float DeltaSeconds);
	void TickVisuals();

	/** Entries에 맞게 그림자를 만들고 지웁니다. */
	void SyncLocalVisuals();
	void DestroyLocalFall(FLocalFall& Fall);

	void ApplyMaterial(AActor* Actor, int32 MaterialIndex) const;

	/** 액터의 모든 충돌 컴포넌트에 프로파일을 적용 */
	void ApplyCollision(AActor* Actor, const FCollisionProfileName& Profile) const;

	/** 서버: 떨어지는 액터와 겹친 캐릭터를 튕겨냅니다. */
	void ApplyHitKnockback(const FCTFallEntry& Entry, AActor* Falling, const FVector& BoundsOrigin, const FVector& BoundsExtent);

	/** 서버: 바닥 높이를 찾습니다. 못 찾으면 박스 바닥 높이 */
	float FindGroundZ(const FVector& From) const;

	float PickInterval() const;

	TMap<int32, FLocalFall> LocalFalls;

	/** 서버 전용: 액터 하나의 낙하 정보 (선형보간에 쓰는 값) */
	struct FServerFall
	{
		float Duration = 1.f;   // 떨어지는 데 걸리는 시간
		float Elapsed = 0.f;    // 지금까지 흐른 시간
		float Speed = 0.f;      // 떨어지는 속도 (로그용)
		float StartZ = 0.f;     // 시작 높이 (액터 기준점)
		float EndZ = 0.f;       // 바닥에 닿았을 때의 높이 (액터 기준점)
		FVector2D XY = FVector2D::ZeroVector;
		FQuat StartRotation = FQuat::Identity;
		FQuat EndRotation = FQuat::Identity;
	};

	TMap<int32, FServerFall> ServerFalls;

	/** 서버 전용: 캐릭터가 마지막으로 맞은 시각 (연속 충돌 방지) */
	TMap<TWeakObjectPtr<AActor>, double> LastHitTimes;

	int32 NextEntryId = 1;
	float TimeToNextSpawn = 1.f;
	bool bSpawning = false;
};
