// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "../Type/CTTypes.h"
#include "CTPlayerState.generated.h"

class ACTPlayerState;

// 값 변경 알림
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCTPlayerStateChangedSignature, ACTPlayerState*, ChangedPlayerState);

UCLASS()
class CATCHTHETAIL_API ACTPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ACTPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 읽기
	UFUNCTION(BlueprintPure, Category = "CT|PlayerState")
	ECTColor GetAssignedColor() const { return AssignedColor; }

	UFUNCTION(BlueprintPure, Category = "CT|PlayerState")
	ECTColor GetTargetColor() const { return TargetColor; }

	UFUNCTION(BlueprintPure, Category = "CT|PlayerState")
	int32 GetTailCount() const { return TailCount; }

	UFUNCTION(BlueprintPure, Category = "CT|PlayerState")
	bool IsGhost() const { return bIsGhost; }

	// 대기 로비: 준비 완료
	UFUNCTION(BlueprintPure, Category = "CT|PlayerState")
	bool IsReady() const { return bReady; }

	// 대기 로비: 방장 (listen 서버를 연 사람)
	UFUNCTION(BlueprintPure, Category = "CT|PlayerState")
	bool IsHost() const { return bIsHost; }

	// 색 → 실제 색값
	UFUNCTION(BlueprintPure, Category = "CT|Color")
	static FLinearColor GetLinearColorOf(ECTColor InColor);

	// 색 → 표시 이름
	UFUNCTION(BlueprintPure, Category = "CT|Color")
	static FText GetColorDisplayName(ECTColor InColor);

	// 쓰기
	void SetColors(ECTColor NewAssignedColor, ECTColor NewTargetColor);
	void SetTailCount(int32 NewTailCount);
	// 꼬리 수 증감, 0 미만 불가
	void AddTailCount(int32 Delta);
	void SetIsGhost(bool bNewIsGhost);
	void SetReady(bool bNewReady);

	// 전화부스 남은 사용 횟수 (서버가 관리, 복제)
	UFUNCTION(BlueprintPure, Category = "CT|PlayerState")
	int32 GetPhoneUsesLeft() const { return PhoneUsesLeft; }

	void SetPhoneUsesLeft(int32 NewUses);

	// 한 번 사용. 남은 횟수가 없으면 false
	bool ConsumePhoneUse();
	void SetIsHost(bool bNewIsHost);

	// 값 변경 시 호출 (UI용)
	UPROPERTY(BlueprintAssignable, Category = "CT|PlayerState")
	FCTPlayerStateChangedSignature OnPlayerStateChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Colors, BlueprintReadOnly, Category = "CT|PlayerState")
	ECTColor AssignedColor = ECTColor::None;

	UPROPERTY(ReplicatedUsing = OnRep_Colors, BlueprintReadOnly, Category = "CT|PlayerState")
	ECTColor TargetColor = ECTColor::None;

	UPROPERTY(ReplicatedUsing = OnRep_TailCount, BlueprintReadOnly, Category = "CT|PlayerState")
	int32 TailCount = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "CT|PlayerState")
	int32 PhoneUsesLeft = 0;

	UPROPERTY(ReplicatedUsing = OnRep_IsGhost, BlueprintReadOnly, Category = "CT|PlayerState")
	bool bIsGhost = false;

	UPROPERTY(ReplicatedUsing = OnRep_Lobby, BlueprintReadOnly, Category = "CT|PlayerState")
	bool bReady = false;

	UPROPERTY(ReplicatedUsing = OnRep_Lobby, BlueprintReadOnly, Category = "CT|PlayerState")
	bool bIsHost = false;

	UFUNCTION()
	void OnRep_Lobby();

	UFUNCTION()
	void OnRep_Colors();

	UFUNCTION()
	void OnRep_TailCount();

	UFUNCTION()
	void OnRep_IsGhost();

private:
	void NotifyChanged();

	// 소리용 이전 값 (늘어났는지, 유령에서 돌아왔는지 판단)
	int32 LastTailCountForSound = 0;
	bool bLastGhostForSound = false;

	// 이 PlayerState가 이 컴퓨터에서 조종하는 플레이어인가
	bool IsLocalPlayerForSound() const;
	void NotifyPawnGhostState();
};
