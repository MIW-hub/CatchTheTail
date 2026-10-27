// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/EngineBaseTypes.h"
#include "CTGameInstance.generated.h"

class UNetDriver;

// 메뉴 안내 문구 변경
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCTMenuMessageSignature, FText, Message);

// 플레이어 이름이 바뀜 (설정 창 등)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCTPlayerNameChangedSignature);

UCLASS()
class CATCHTHETAIL_API UCTGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	// listen 서버로 방 만들기
	UFUNCTION(BlueprintCallable, Category = "CT|Menu")
	void HostGame();

	// 메인 메뉴로. Message는 메뉴에 표시
	UFUNCTION(BlueprintCallable, Category = "CT|Menu")
	void LeaveToMainMenu(FText Message);

	UFUNCTION(BlueprintCallable, Category = "CT|Menu")
	void QuitToDesktop();

	// 플레이어 이름
	// 없으면 빈 문자열
	UFUNCTION(BlueprintPure, Category = "CT|Profile")
	FString GetPlayerName() const { return PlayerName; }

	UFUNCTION(BlueprintPure, Category = "CT|Profile")
	bool HasPlayerName() const { return !PlayerName.IsEmpty(); }

	UFUNCTION(BlueprintPure, Category = "CT|Profile")
	int32 GetMinNameLength() const { return MinNameLength; }

	UFUNCTION(BlueprintPure, Category = "CT|Profile")
	int32 GetMaxNameLength() const { return MaxNameLength; }

	// 이름 규칙 검사. 실패 시 OutError
	UFUNCTION(BlueprintPure, Category = "CT|Profile")
	bool ValidatePlayerName(const FString& Name, FText& OutError) const;

	// 개발용. 저장된 이름 삭제 (Shipping 제외)
	UFUNCTION(Exec)
	void CTResetPlayerName();

	// 검사 후 저장. 최초 1회만
	UFUNCTION(BlueprintCallable, Category = "CT|Profile")
	bool SetPlayerName(const FString& NewName, FText& OutError);

	// 이름 변경. Allow Name Change일 때만 호출
	UFUNCTION(BlueprintCallable, Category = "CT|Profile")
	bool ChangePlayerName(const FString& NewName, FText& OutError);

	// 인트로 표시 여부. 메뉴 복귀해도 유지
	UFUNCTION(BlueprintPure, Category = "CT|Menu")
	bool HasShownIntro() const { return bIntroShown; }

	UFUNCTION(BlueprintCallable, Category = "CT|Menu")
	void MarkIntroShown() { bIntroShown = true; }

	// 메뉴에 띄울 마지막 문구
	UFUNCTION(BlueprintPure, Category = "CT|Menu")
	FText GetPendingMessage() const { return PendingMessage; }

	// 문구 비우기
	UFUNCTION(BlueprintCallable, Category = "CT|Menu")
	void ClearPendingMessage() { PendingMessage = FText::GetEmpty(); }

	UPROPERTY(BlueprintAssignable, Category = "CT|Menu")
	FCTMenuMessageSignature OnMenuMessage;

	// 이름이 저장될 때마다 호출. 화면의 이름 표시를 갱신할 때 씁니다.
	UPROPERTY(BlueprintAssignable, Category = "CT|Menu")
	FCTPlayerNameChangedSignature OnPlayerNameChanged;

	// 스팀 등 온라인 세션 기능을 쓸 수 있는가. 빠른 참가, 방 코드 입장이 이 값에 따라 막힙니다.
	UFUNCTION(BlueprintPure, Category = "CT|Online")
	bool IsOnlineSessionAvailable() const { return bOnlineSessionAvailable; }

	// 메뉴에 안내 문구를 띄움 (다음 메뉴 화면에도 남김)
	void ShowMenuMessage(const FText& Message);

	// 접속 옵션 "?Name=..." (이름이 없으면 빈 문자열)
	FString MakeNameOption() const;

	// 열려 있는 방을 찾아 무작위로 입장. 온라인 기능이 막혀 있으면 안내만 띄움
	UFUNCTION(BlueprintCallable, Category = "CT|Online")
	void QuickJoin();

	// 방 코드로 입장. 온라인 기능이 막혀 있으면 안내만 띄움
	UFUNCTION(BlueprintCallable, Category = "CT|Online")
	void JoinByRoomCode(const FString& RoomCode);

	// 게임이 끝난 뒤 돌아갈 대기 로비 맵 이름. 비어 있으면 None
	UFUNCTION(BlueprintPure, Category = "CT|Menu")
	FName GetLobbyMapName() const { return LobbyMapName; }

	// 이 방의 방 코드. 게임을 하고 로비로 돌아와도 같은 코드를 유지하려고 보관 (방장 컴퓨터에서만 사용)
	// 마지막으로 선택한 맵 번호. 로비로 돌아와도 같은 맵을 유지하려고 보관 (방장 컴퓨터에서만 사용)
	int32 GetStoredMapIndex() const { return StoredMapIndex; }
	void SetStoredMapIndex(int32 NewMapIndex) { StoredMapIndex = NewMapIndex; }

	const FString& GetStoredRoomCode() const { return StoredRoomCode; }
	void SetStoredRoomCode(const FString& NewRoomCode) { StoredRoomCode = NewRoomCode; }

	// 로비가 게임을 시작할 때 이동할 맵 이름
	UFUNCTION(BlueprintPure, Category = "CT|Menu")
	FName GetGameMapName() const { return GameMapName; }

protected:
	/**
	 * 온라인 세션(스팀) 기능 사용 가능 여부. 스팀 연결을 만들기 전까지 끕니다.
	 * 켜면 빠른 참가와 방 코드 입장이 열립니다. (아직 세션 코드가 없으면 준비 중 안내만 나옴)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Online")
	bool bOnlineSessionAvailable = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Menu")
	FName MainMenuMapName = TEXT("L_Mainmenu");

	// HostGame이 가장 먼저 여는 대기 로비 맵. 비우면 게임 맵을 바로 엽니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Menu")
	FName LobbyMapName = TEXT("L_Lobby");

	// HostGame으로 여는 맵
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Menu")
	FName GameMapName = TEXT("TypeA");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Profile", meta = (ClampMin = "1"))
	int32 MinNameLength = 2;

	// 점수판 표시 길이 한도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Profile", meta = (ClampMin = "1"))
	int32 MaxNameLength = 12;

private:
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

	// 입장 거절 사유 코드를 안내 문구로 바꿈
	FText MakeJoinFailureMessage(const FString& ErrorString) const;

	// 입장 거절 안내를 마지막으로 띄운 시각. 같은 실패에 대해 일반 안내가 덮어쓰지 않게 함
	double LastJoinRejectTime = -1000.0;

	bool IsInMainMenuMap() const;

	void LoadProfile();
	void SaveProfile() const;

	bool StoreValidatedName(const FString& NewName, FText& OutError);

	FString PlayerName;

	FString StoredRoomCode;
	int32 StoredMapIndex = INDEX_NONE;

	FText PendingMessage;
	bool bIntroShown = false;
};
