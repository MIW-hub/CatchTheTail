// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CTTail.generated.h"

class ACTCharacter;
class UMaterialInstanceDynamic;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * 캐릭터 뒤에 달린 꼬리(배터리). 서버가 만들고 캐릭터의 소켓에 붙입니다.
 *
 * 상태
 *  - 켜짐(Lit)   : 주인의 색 + 이미시브(불빛)가 들어간 밝은 상태
 *  - 잡힘(Caught): 불빛이 꺼지고 회색으로 변한 상태
 *
 * 잡힘 판정
 *  - 목표 색 플레이어가 "잡기 동작" 중에 꼬리에 닿으면 (TryTouch) 서버가 GameMode에 잡기 성공을 요청합니다.
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

	/** 닿았다고 보는 범위의 중심 */
	FVector GetTouchLocation() const;

	// ----- 서버 전용 -----
	/** 잡힘 상태 변경. 서버에서만 동작하고 모든 클라이언트에 복제됩니다. */
	void SetCaught(bool bNewCaught);

	/**
	 * Toucher(잡기 동작 중인 캐릭터)가 이 꼬리에 닿았는지 확인하고, 닿았으면 잡기 성공을 요청합니다.
	 * @return 잡기가 성공하면 true
	 */
	bool TryTouch(ACTCharacter* Toucher);

	/** 주인의 색 / 잡힘 상태에 맞춰 머티리얼을 다시 설정 (서버, 클라이언트 모두) */
	void RefreshVisual();

protected:
	virtual void BeginPlay() override;
	virtual void OnRep_AttachmentReplication() override;

	/** 잡힘 / 풀림 연출 (이펙트, 사운드용) */
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Events")
	void BP_OnCaughtChanged(bool bNewCaught);

	// ===== 컴포넌트 =====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Tail")
	TObjectPtr<USceneComponent> Root;

	/** 꼬리 메시. 블루프린트에서 StaticMesh와 M_Tail을 지정하세요. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Tail")
	TObjectPtr<UStaticMeshComponent> TailMesh;

	/** 닿는 범위를 에디터에서 눈으로 보기 위한 구 (충돌은 사용하지 않음) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Tail")
	TObjectPtr<USphereComponent> TouchSphere;

	// ===== 설정 =====
	/** 꼬리에 손이 닿는 거리 (cm). 캐릭터 캡슐 반지름이 더해집니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail", meta = (ClampMin = "0"))
	float TouchRadius = 90.f;

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
	void EnsureMaterials();
	void ApplyLook();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;

	/** 현재 불빛 정도 (0~1). 목표값으로 서서히 이동 */
	float LightAmount = 1.f;
	FLinearColor LitColor = FLinearColor::White;
};
