// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CTFallingProp.generated.h"

class UStaticMesh;
class UStaticMeshComponent;


UCLASS()
class CATCHTHETAIL_API ACTFallingProp : public AActor
{
	GENERATED_BODY()

public:
	ACTFallingProp();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 메시, 크기 설정 (FinishSpawning 전)
	void Setup(UStaticMesh* InMesh, float InUniformScale);

	UFUNCTION(BlueprintPure, Category = "CT|Falling")
	UStaticMeshComponent* GetMeshComponent() const { return MeshComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Falling")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(ReplicatedUsing = OnRep_Setup)
	TObjectPtr<UStaticMesh> MeshAsset;

	UPROPERTY(ReplicatedUsing = OnRep_Setup)
	float UniformScale = 1.f;

	UFUNCTION()
	void OnRep_Setup();

private:
	void ApplySetup();
};
