// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "CTSessionSubsystem.generated.h"

class FOnlineSessionSearch;

// 온라인 방(세션) 관리
UCLASS()
class CATCHTHETAIL_API UCTSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	// 세션 사용 가능 여부
	bool IsAvailable() const;

	// 온라인 서비스 이름. 없으면 빈 문자열
	FString GetServiceName() const;

	// 방장: 방 코드 방 열기. 이미 열려 있으면 무시
	void CreateRoom(const FString& RoomCode);

	// 열린 방 닫고 세션 정리
	void CloseRoom();

	// 방 코드로 찾아 입장
	void JoinByRoomCode(const FString& RoomCode);

	// 무작위 방 입장
	void QuickJoin();

	// 찾는 중이거나 입장 중인가
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
