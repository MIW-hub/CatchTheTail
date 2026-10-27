// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "CTSessionSubsystem.generated.h"

class FOnlineSessionSearch;

/**
 * 온라인 방(세션) 관리. 방장은 방 코드가 붙은 방을 열고, 참가자는 방 코드나 빠른 참가로 방을 찾아 들어갑니다.
 * 스팀이 켜져 있으면 스팀 로비를, 아니면 같은 네트워크(LAN) 방을 씁니다.
 */
UCLASS()
class CATCHTHETAIL_API UCTSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	// 세션 기능을 쓸 수 있는가 (스팀 또는 LAN 서브시스템이 있음)
	bool IsAvailable() const;

	// 지금 쓰는 온라인 서비스 이름 (스팀이면 "Steam", 없으면 "NULL"). 없으면 빈 문자열
	FString GetServiceName() const;

	// 방장: 방 코드가 붙은 방을 엽니다. 이미 열려 있으면 아무것도 안 함
	void CreateRoom(const FString& RoomCode);

	// 방장/참가자: 열려 있는 방을 닫고 세션을 정리
	void CloseRoom();

	// 방 코드로 찾아 입장
	void JoinByRoomCode(const FString& RoomCode);

	// 열려 있는 방 중 무작위로 입장
	void QuickJoin();

	// 찾는 중이거나 입장하는 중인가
	bool IsBusy() const { return bBusy; }

private:
	enum class EJoinMode : uint8 { None, Code, Quick };

	IOnlineSessionPtr GetSessionInterface() const;
	bool IsLan() const;
	void StartSearch(EJoinMode Mode, const FString& RoomCode);
	void ShowMessage(const FText& Message) const;
	void EndBusy();

	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	TSharedPtr<FOnlineSessionSearch> Search;
	EJoinMode PendingMode = EJoinMode::None;
	FString PendingRoomCode;
	bool bBusy = false;

	FDelegateHandle CreateHandle;
	FDelegateHandle FindHandle;
	FDelegateHandle JoinHandle;
	FDelegateHandle DestroyHandle;
};
