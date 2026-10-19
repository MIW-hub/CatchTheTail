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

/**
 * 캐릭터 뒤에 끌리는 꼬리(배터리). 서버가 만들고 캐릭터의 본에 붙입니다.
 *
 * 모양: 꼬리 수만큼 몸통을 이어 붙입니다. (크기를 키우지 않고 "개수"를 늘림)
 *   꼬리 0개 : 받침(tail_03)만
 *   꼬리 N개 : 받침(tail_03) + 몸통(tail_02) × N + 끝 캡(tail_01)
 *   N은 MaxBodySegments(기본 7)까지만 보입니다.
 *   각 클라이언트가 복제된 TailCount(PlayerState)로 직접 만들기 때문에, 몸통 하나하나를 복제하지 않습니다.
 *
 * 상태
 *  - 켜짐(Lit)   : 주인의 색 + 이미시브(불빛)가 들어간 밝은 상태
 *  - 잡힘(Caught): 불빛이 꺼지고 회색으로 변한 상태 (쌓인 전체가 같이 바뀜)
 *
 * 잡힘 판정
 *  - 목표 색 플레이어가 "잡기 동작" 중에 꼬리 끝(맨 끝 캡)에 닿으면 (TryTouch) 서버가 GameMode에 잡기 성공을 요청합니다.
 *  - 꼬리가 0개이면 받침이 닿는 지점입니다.
 *  - 성공하면 주인이 유령이 되고, 캐릭터가 SetCaught(true)를 호출해 꼬리를 회색으로 만듭니다.
 *  - 다음 라운드에 주인이 부활하면 SetCaught(false)로 다시 켜집니다.
 *
 * 머티리얼 파라미터 (M_Tail에 같은 이름으로 만드세요)
 *  - TailColor (Vector)       : 마스크 영역에 칠할 색
 *  - EmissiveStrength (Scalar): 불빛 세기. 잡히면 0으로 줄어듭니다.
 */
UCLASS()
class CATCHTHETAIL_API ACTTail : public AActor
{
	GENERATED_BODY()

public:
	ACTTail();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	// ----- 읽기 -----
	UFUNCTION(BlueprintPure, Category = "CT|Tail")
	bool IsCaught() const { return bCaught; }

	/** 이 꼬리를 달고 있는 캐릭터 (붙어 있는 부모) */
	UFUNCTION(BlueprintPure, Category = "CT|Tail")
	ACTCharacter* GetTailOwnerCharacter() const;

	/** 지금 보이는 몸통 개수 (0~MaxBodySegments) */
	UFUNCTION(BlueprintPure, Category = "CT|Tail")
	int32 GetBodySegmentCount() const { return BodyMeshes.Num(); }

	/** 닿았다고 보는 지점. 몸통이 있으면 맨 끝 캡, 없으면 받침 */
	FVector GetTouchLocation() const;

	// ----- 서버 전용 -----
	/** 잡힘 상태 변경. 서버에서만 동작하고 모든 클라이언트에 복제됩니다. */
	void SetCaught(bool bNewCaught);

	/**
	 * Toucher(잡기 동작 중인 캐릭터)가 이 꼬리에 닿았는지 확인하고, 닿았으면 잡기 성공을 요청합니다.
	 * @return 잡기가 성공하면 true
	 */
	bool TryTouch(ACTCharacter* Toucher);

	/** 주인의 색 / 꼬리 수 / 잡힘 상태에 맞춰 모양과 머티리얼을 다시 설정 (서버, 클라이언트 모두) */
	void RefreshVisual();

protected:
	virtual void BeginPlay() override;
	virtual void OnRep_AttachmentReplication() override;

	/** 잡힘 / 풀림 연출 (이펙트, 사운드용) */
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Events")
	void BP_OnCaughtChanged(bool bNewCaught);

	/** 몸통 개수가 바뀜 (이펙트, 사운드용) */
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Events")
	void BP_OnSegmentCountChanged(int32 NewCount);

	// ===== 컴포넌트 =====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Tail")
	TObjectPtr<USceneComponent> Root;

	/** 받침 (tail_03). 항상 보입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Tail")
	TObjectPtr<USkeletalMeshComponent> BaseMesh;

	/** 맨 끝 캡 (tail_01). 몸통이 1개 이상일 때만 보입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Tail")
	TObjectPtr<USkeletalMeshComponent> CapMesh;

	// ===== 메시 (기본값은 tail_03 / tail_02 / tail_01. 블루프린트에서 바꿀 수 있음) =====
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Mesh")
	TObjectPtr<USkeletalMesh> BaseAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Mesh")
	TObjectPtr<USkeletalMesh> BodyAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Mesh")
	TObjectPtr<USkeletalMesh> CapAsset;

	// ===== 몸통 쌓기 =====
	/** 몸통 한 개를 이을 때 꼬리 방향(+Z)으로 움직이는 거리(cm). 몸통 높이(약 97)보다 작으면 살짝 겹침 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Segments", meta = (ClampMin = "1"))
	float SegmentSpacing = 94.f;

	/** 보이는 몸통의 최대 개수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Segments", meta = (ClampMin = "0", ClampMax = "30"))
	int32 MaxBodySegments = 7;

	/** 새 몸통이 작게 시작해서 커지며 붙는 연출 시간(초). 0이면 연출 없음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Segments", meta = (ClampMin = "0"))
	float SegmentPopSeconds = 0.2f;

	// ===== 터치 =====
	/** 꼬리 끝에 손이 닿는 거리 (cm). 캐릭터 캡슐 반지름이 더해집니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail", meta = (ClampMin = "0"))
	float TouchRadius = 90.f;

	// ===== 겉모습 =====
	/** 켜졌을 때 불빛 세기 (머티리얼의 EmissiveStrength에 들어감) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Look", meta = (ClampMin = "0"))
	float LitEmissiveStrength = 4.f;

	/** 잡혔을 때 꼬리 색 (회색) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Look")
	FLinearColor CaughtColor = FLinearColor(0.12f, 0.12f, 0.12f, 1.f);

	/** 불이 꺼지거나 켜지는 데 걸리는 시간(초). 0이면 즉시 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Look", meta = (ClampMin = "0"))
	float LightFadeSeconds = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Look")
	FName ColorParameterName = TEXT("TailColor");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Look")
	FName EmissiveParameterName = TEXT("EmissiveStrength");

	/** 주인의 색이 아직 없을 때 쓸 색 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail|Look")
	FLinearColor NoOwnerColor = FLinearColor::White;

	UPROPERTY(ReplicatedUsing = OnRep_Caught, BlueprintReadOnly, Category = "CT|Tail")
	bool bCaught = false;

	UFUNCTION()
	void OnRep_Caught();

private:
	/** 꼬리 수(TailCount)에 맞춰 몸통과 캡을 만들고 위치를 정함 */
	void RefreshSegments();

	USkeletalMeshComponent* AddBodySegment();

	/** 받침 / 몸통 / 캡의 머티리얼을 동적 머티리얼로 모아 둠 */
	void RebuildMaterials();
	void ApplyLook();
	void ApplyMeshAssets();

	UPROPERTY(Transient)
	TArray<TObjectPtr<USkeletalMeshComponent>> BodyMeshes;

	/** 몸통마다 등장 진행도 (0~1) */
	TArray<float> BodyPop;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;

	/** 현재 불빛 정도 (0~1). 목표값으로 서서히 이동 */
	float LightAmount = 1.f;
	FLinearColor LitColor = FLinearColor::White;
	bool bMeshAssetsApplied = false;
};
