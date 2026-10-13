// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "../Type/CTTypes.h"
#include "CTPlayerState.generated.h"

class ACTPlayerState;

/** 값이 바뀔 때마다 UI가 받을 수 있는 알림 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCTPlayerStateChangedSignature, ACTPlayerState*, ChangedPlayerState);

/**
 * 플레이어 한 명의 게임 정보.
 * 색, 목표 색, 꼬리 수(점수), 유령 여부를 서버가 정하고 모든 클라이언트에 복제합니다.
 */
UCLASS()
class CATCHTHETAIL_API ACTPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ACTPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ----- 읽기 -----
	UFUNCTION(BlueprintPure, Category = "CT|PlayerState")
	ECTColor GetAssignedColor() const { return AssignedColor; }

	UFUNCTION(BlueprintPure, Category = "CT|PlayerState")
	ECTColor GetTargetColor() const { return TargetColor; }

	UFUNCTION(BlueprintPure, Category = "CT|PlayerState")
	int32 GetTailCount() const { return TailCount; }

	UFUNCTION(BlueprintPure, Category = "CT|PlayerState")
	bool IsGhost() const { return bIsGhost; }

	/** 색 → 화면에 쓸 실제 색 값 (사업계획서 HEX 기준) */
	UFUNCTION(BlueprintPure, Category = "CT|Color")
	static FLinearColor GetLinearColorOf(ECTColor InColor);

	/** 색 → 화면에 보여줄 이름 (빨강, 주황 ...) */
	UFUNCTION(BlueprintPure, Category = "CT|Color")
	static FText GetColorDisplayName(ECTColor InColor);

	// ----- 쓰기 (서버에서만 동작) -----
	void SetColors(ECTColor NewAssignedColor, ECTColor NewTargetColor);
	void SetTailCount(int32 NewTailCount);
	/** 꼬리 수를 더하거나 뺍니다. 0 아래로는 내려가지 않습니다. */
	void AddTailCount(int32 Delta);
	void SetIsGhost(bool bNewIsGhost);

	/** 값이 바뀌면 호출됩니다. (서버, 클라이언트 모두) UI 연결용 */
	UPROPERTY(BlueprintAssignable, Category = "CT|PlayerState")
	FCTPlayerStateChangedSignature OnPlayerStateChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Colors, BlueprintReadOnly, Category = "CT|PlayerState")
	ECTColor AssignedColor = ECTColor::None;

	UPROPERTY(ReplicatedUsing = OnRep_Colors, BlueprintReadOnly, Category = "CT|PlayerState")
	ECTColor TargetColor = ECTColor::None;

	UPROPERTY(ReplicatedUsing = OnRep_TailCount, BlueprintReadOnly, Category = "CT|PlayerState")
	int32 TailCount = 0;

	UPROPERTY(ReplicatedUsing = OnRep_IsGhost, BlueprintReadOnly, Category = "CT|PlayerState")
	bool bIsGhost = false;

	UFUNCTION()
	void OnRep_Colors();

	UFUNCTION()
	void OnRep_TailCount();

	UFUNCTION()
	void OnRep_IsGhost();

private:
	void NotifyChanged();
	void NotifyPawnGhostState();
};
