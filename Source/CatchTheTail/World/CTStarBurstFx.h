// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CTStarBurstFx.generated.h"

class UInstancedStaticMeshComponent;

// 꼬리를 빼앗길 때 터지는 별 효과. 각 화면에서 로컬로만 만들어졌다가 스스로 사라짐 (복제 없음)
UCLASS(NotBlueprintable)
class CATCHTHETAIL_API ACTStarBurstFx : public AActor
{
	GENERATED_BODY()

public:
	ACTStarBurstFx();

	// 위치에 별 효과를 터뜨림. Scale 은 크기 배율 (블루프린트/테스트에서도 부를 수 있음)
	UFUNCTION(BlueprintCallable, Category = "CT|VFX", meta = (WorldContext = "World"))
	static ACTStarBurstFx* Spawn(UWorld* World, const FVector& Location, float Scale = 1.f);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	// 별 개수 (금색 + 분홍색 합계)
	UPROPERTY(EditDefaultsOnly, Category = "CT|VFX", meta = (ClampMin = "1", ClampMax = "40"))
	int32 StarCount = 12;

	// 별 한 개가 보이는 시간(초) 범위
	UPROPERTY(EditDefaultsOnly, Category = "CT|VFX")
	FVector2D LifeRange = FVector2D(0.7f, 1.1f);

	// 튀어 오르는 속도(cm/s) 범위
	UPROPERTY(EditDefaultsOnly, Category = "CT|VFX")
	FVector2D SpeedRange = FVector2D(260.f, 520.f);

	// 별 한 변 크기(cm) 범위
	UPROPERTY(EditDefaultsOnly, Category = "CT|VFX")
	FVector2D SizeRange = FVector2D(28.f, 48.f);

	// 아래로 떨어지는 힘(cm/s^2)
	UPROPERTY(EditDefaultsOnly, Category = "CT|VFX")
	float Gravity = 900.f;

	// 별 색
	UPROPERTY(EditDefaultsOnly, Category = "CT|VFX")
	FLinearColor GoldColor = FLinearColor(1.f, 0.82f, 0.12f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "CT|VFX")
	FLinearColor PinkColor = FLinearColor(1.f, 0.3f, 0.65f, 1.f);

private:
	struct FStar
	{
		FVector Position = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		float Size = 30.f;
		float Life = 1.f;
		float Age = 0.f;
		float Roll = 0.f;
		float Spin = 0.f;
		bool bPink = false;
		int32 InstanceIndex = 0;
	};

	TArray<FStar> Stars;
	float EffectScale = 1.f;

	UPROPERTY(VisibleAnywhere, Category = "CT|VFX")
	TObjectPtr<UInstancedStaticMeshComponent> GoldStars;

	UPROPERTY(VisibleAnywhere, Category = "CT|VFX")
	TObjectPtr<UInstancedStaticMeshComponent> PinkStars;

	void BuildStars();
	void UpdateStars(float DeltaSeconds);
};
