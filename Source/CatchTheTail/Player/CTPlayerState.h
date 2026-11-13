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

	// 맵 이동(Seamless Travel) 때 로비에서 정한 팀 번호를 새 PlayerState 로 옮김
	virtual void CopyProperties(APlayerState* NewPlayerState) override;

	// 읽기
	UFUNCTION(BlueprintPure, Category = "CT|PlayerState")
	ECTColor GetAssignedColor() const { return AssignedColor; }

	UFUNCTION(BlueprintPure, Category = "CT|PlayerState")
	ECTColor GetTargetColor() const { return TargetColor; }

	UFUNCTION(BlueprintPure, Category = "CT|PlayerState")
	int32 GetTailCount() const { return TailCount; }

	UFUNCTION(BlueprintPure, Category = "CT|PlayerState")
	bool IsGhost() const { return bIsGhost; }

	// 팀 번호 (팀전 모드에서만 사용, 없으면 -1). 모두에게 복제
	UFUNCTION(BlueprintPure, Category = "CT|PlayerState")
	int32 GetTeamId() const { return TeamId; }

	// 내가 모은 에너지 (보물찾기). 본인 화면에만 복제됨
	UFUNCTION(BlueprintPure, Category = "CT|PlayerState")
	int32 GetEnergy() const { return Energy; }

	// 경기가 끝난 뒤 모두에게 공개되는 개인 에너지 점수 (결과 화면용)
	UFUNCTION(BlueprintPure, Category = "CT|PlayerState")
	int32 GetResultEnergy() const { return ResultEnergy; }

	// 우리 팀 에너지 합계 (보물찾기). 본인 팀원에게만 복제되어 상대 팀은 알 수 없음
	UFUNCTION(BlueprintPure, Category = "CT|PlayerState")
	int32 GetTeamEnergy() const { return TeamEnergy; }

	// 이번 경기에서 기권함. 순위와 라운드 진행에서 빠지고 유령으로 관전만 함
	UFUNCTION(BlueprintPure, Category = "CT|PlayerState")
	bool IsForfeited() const { return bForfeited; }

	// 대기 로비: 준비 완료
	UFUNCTION(BlueprintPure, Category = "CT|PlayerState")
	bool IsReady() const { return bReady; }

	// 방장 (listen 서버 호스트)
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

	// 서버 전용 쓰기: 팀 번호, 에너지 (0 미만 불가), 결과 공개용 에너지 점수
	void SetTeamId(int32 NewTeamId);
	void SetEnergy(int32 NewEnergy);
	void AddEnergy(int32 Delta);
	void SetResultEnergy(int32 NewResultEnergy);
	void SetTeamEnergy(int32 NewTeamEnergy);

	// 팀 음성으로 말하는 중인가 (보물찾기). 모두에게 복제되어 듣는 쪽이 목소리 처리를 정함
	bool IsTeamVoice() const { return bTeamVoice; }
	void SetTeamVoice(bool bNewTeamVoice);

	// 서버 전용: 이번 유령 상태에서 F키 이동을 이미 썼는가 (유령이 될 때마다 초기화)
	bool HasUsedGhostRelocate() const { return bGhostRelocateUsed; }
	void MarkGhostRelocateUsed() { bGhostRelocateUsed = true; }

	void SetForfeited(bool bNewForfeited);
	void SetReady(bool bNewReady);

	// 전화를 다시 걸 수 있게 되는 서버 시각 (GameState 서버 시간 기준, 서버가 관리, 복제)
	UFUNCTION(BlueprintPure, Category = "CT|PlayerState")
	double GetPhoneReadyServerTime() const { return PhoneReadyServerTime; }

	void SetPhoneReadyServerTime(double NewReadyTime);
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
	double PhoneReadyServerTime = 0.0;

	UPROPERTY(ReplicatedUsing = OnRep_Lobby, BlueprintReadOnly, Category = "CT|PlayerState")
	int32 TeamId = INDEX_NONE;

	UPROPERTY(ReplicatedUsing = OnRep_Energy, BlueprintReadOnly, Category = "CT|PlayerState")
	int32 Energy = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Energy, BlueprintReadOnly, Category = "CT|PlayerState")
	int32 ResultEnergy = 0;

	UPROPERTY(Replicated)
	int32 TeamEnergy = 0;

	UPROPERTY(Replicated)
	bool bTeamVoice = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsGhost, BlueprintReadOnly, Category = "CT|PlayerState")
	bool bIsGhost = false;

	// 서버만 사용, 복제 안 함
	bool bGhostRelocateUsed = false;

	UPROPERTY(ReplicatedUsing = OnRep_Lobby, BlueprintReadOnly, Category = "CT|PlayerState")
	bool bForfeited = false;

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
	void OnRep_Energy();

	UFUNCTION()
	void OnRep_IsGhost();

private:
	void NotifyChanged();

	// 소리 판단용 이전 꼬리 수
	int32 LastTailCountForSound = 0;
	bool bLastGhostForSound = false;

	// 이 PlayerState가 이 컴퓨터에서 조종하는 플레이어인가
	bool IsLocalPlayerForSound() const;
	void NotifyPawnGhostState();
};
