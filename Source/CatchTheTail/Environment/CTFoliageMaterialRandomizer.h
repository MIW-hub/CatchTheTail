// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CTFoliageMaterialRandomizer.generated.h"

class UStaticMesh;
class UMaterialInterface;

UCLASS()
class CATCHTHETAIL_API ACTFoliageMaterialRandomizer : public AActor
{
	GENERATED_BODY()

public:
	ACTFoliageMaterialRandomizer();

protected:
	virtual void BeginPlay() override;

	// 대상 폴리지 메시
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Foliage")
	TObjectPtr<UStaticMesh> TargetMesh;

	// 랜덤 후보
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Foliage")
	TArray<TObjectPtr<UMaterialInterface>> CandidateMaterials;

	// 슬롯 번호 (-1 = 전체)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Foliage", meta = (ClampMin = "-1"))
	int32 MaterialSlotIndex = -1;

	// 인스턴스 분배
	UFUNCTION(BlueprintCallable, Category = "CT|Foliage")
	void RandomizeFoliageMaterials();

private:
	// 중복 실행 방지
	bool bApplied = false;
};
