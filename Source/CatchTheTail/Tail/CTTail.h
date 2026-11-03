// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CTTail.generated.h"

class ACTCharacter;
class UMaterialInstanceDynamic;
class USceneComponent;
class USkeletalMesh;
class USkeletalMeshComponent;

UCLASS()
class CATCHTHETAIL_API ACTTail : public AActor
{
	GENERATED_BODY()

public:
	ACTTail();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	// 읽기
	UFUNCTION(BlueprintPure, Category = "CT|Tail")
	bool IsCaught() const { return bCaught; }

	// 이 꼬리의 주인 캐릭터
	UFUNCTION(BlueprintPure, Category = "CT|Tail")
	ACTCharacter* GetTailOwnerCharacter() const;

	// 보이는 몸통 수
	UFUNCTION(BlueprintPure, Category = "CT|Tail")
	int32 GetBodySegmentCount() const { return BodyMeshes.Num(); }

	// 닿음 기준점: 끝 캡, 없으면 받침
	FVector GetTouchLocation() const;

	// 서버 전용
	// 잡힘 변경 (서버, 복제)
	void SetCaught(bool bNewCaught);

	// 잡기 성공 요청, 성공 시 true
	bool TryTouch(ACTCharacter* Toucher);

	// 주인 색/꼬리 수/잡힘 기준 재설정
	void RefreshVisual();

protected:
	virtual void BeginPlay() override;
	virtual void OnRep_AttachmentReplication() override;

	// 잡힘/풀림 연출
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Events")
	void BP_OnCaughtChanged(bool bNewCaught);

	// 몸통 수 변경 연출
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Events")
	void BP_OnSegmentCountChanged(int32 NewCount);

	// 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Tail")
	TObjectPtr<USceneComponent> Root;

	// 받침 (tail_03), 항상 표시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Tail")
	TObjectPtr<USkeletalMeshComponent> BaseMesh;

	// 끝 캡 (tail_01), 몸통 있을 때만
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Tail")
	TObjectPtr<USkeletalMeshComponent> CapMesh;

	// 메시
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Mesh")
	TObjectPtr<USkeletalMesh> BaseAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Mesh")
	TObjectPtr<USkeletalMesh> BodyAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Mesh")
	TObjectPtr<USkeletalMesh> CapAsset;

	// 몸통 쌓기
	// 몸통 간격(cm), 높이 97보다 작으면 겹침
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Segments", meta = (ClampMin = "1"))
	float SegmentSpacing = 94.f;

	// 몸통 최대 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Segments", meta = (ClampMin = "0", ClampMax = "30"))
	int32 MaxBodySegments = 7;

	// 새 몸통 성장 시간(초), 0이면 없음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Segments", meta = (ClampMin = "0"))
	float SegmentPopSeconds = 0.2f;

	// 터치
	// 끝 접촉 거리(cm), 캡슐 반지름 가산
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail", meta = (ClampMin = "0"))
	float TouchRadius = 90.f;

	// 겉모습
	// 켜짐 불빛 세기 (EmissiveStrength)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Look", meta = (ClampMin = "0"))
	float LitEmissiveStrength = 4.f;

	// 잡힘 색
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Look")
	FLinearColor CaughtColor = FLinearColor(0.12f, 0.12f, 0.12f, 1.f);

	// 불 전환 시간(초), 0이면 즉시
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Look", meta = (ClampMin = "0"))
	float LightFadeSeconds = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Look")
	FName ColorParameterName = TEXT("TailColor");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Look")
	FName EmissiveParameterName = TEXT("EmissiveStrength");

	// 주인 색 없을 때 색
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Look")
	FLinearColor NoOwnerColor = FLinearColor::White;

	UPROPERTY(ReplicatedUsing = OnRep_Caught, BlueprintReadOnly, Category = "CT|Tail")
	bool bCaught = false;

	UFUNCTION()
	void OnRep_Caught();

private:
	// TailCount대로 몸통/캡 배치
	void RefreshSegments();

	USkeletalMeshComponent* AddBodySegment();

	// 머티리얼을 동적으로 모아 둠
	void RebuildMaterials();
	void ApplyLook();
	void ApplyMeshAssets();

	UPROPERTY(Transient)
	TArray<TObjectPtr<USkeletalMeshComponent>> BodyMeshes;

	// 몸통별 등장 진행도 0~1
	TArray<float> BodyPop;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;

	// 현재 불빛 0~1, 목표로 이동
	float LightAmount = 1.f;
	FLinearColor LitColor = FLinearColor::White;
	bool bMeshAssetsApplied = false;
};
