// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CTExplosionFx.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

// 폭발 드럼통이 터질 때의 불덩이, 충격파 고리, 불꽃, 연기. 각 화면에서 로컬로만 만들어졌다가 스스로 사라짐 (복제 없음)
UCLASS(NotBlueprintable)
class CATCHTHETAIL_API ACTExplosionFx : public AActor
{
	GENERATED_BODY()

public:
	ACTExplosionFx();

	// 위치에서 폭발 효과를 터뜨림. Radius 는 폭발 반경(cm)
	UFUNCTION(BlueprintCallable, Category = "CT|VFX", meta = (WorldContext = "World"))
	static ACTExplosionFx* Spawn(UWorld* World, const FVector& Location, float Radius = 450.f);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "CT|VFX", meta = (ClampMin = "0", ClampMax = "80"))
	int32 SparkCount = 28;

	UPROPERTY(EditDefaultsOnly, Category = "CT|VFX", meta = (ClampMin = "0", ClampMax = "30"))
	int32 SmokeCount = 10;

	// 전체 길이(초)
	UPROPERTY(EditDefaultsOnly, Category = "CT|VFX", meta = (ClampMin = "0.5"))
	float TotalSeconds = 1.6f;

private:
	struct FPart
	{
		FVector Position = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		float Size = 20.f;
		float Life = 1.f;
		int32 InstanceIndex = 0;
	};

	void Build();
	static void SetFx(UMaterialInstanceDynamic* Material, const FLinearColor& Color, float Brightness, float Opacity);

	TArray<FPart> Sparks;
	TArray<FPart> Smokes;
	TArray<FPart> Blobs;
	float Radius = 450.f;
	float Age = 0.f;

	UPROPERTY() TObjectPtr<UStaticMeshComponent> Fireball;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Ring;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> BlobMeshes;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> SparkMeshes;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> SmokeMeshes;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FireMat;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> RingMat;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SparkMat;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SmokeMat;
};
