// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/EngineBaseTypes.h"
#include "CTMatchSettings.h"
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

	// 메인 메뉴로, Message는 메뉴 표시
	UFUNCTION(BlueprintCallable, Category = "CT|Menu")
	void LeaveToMainMenu(FText Message);

	UFUNCTION(BlueprintCallable, Category = "CT|Menu")
	void QuitToDesktop();

	// 플레이어 이름
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

	// 이름 저장 시 호출 (UI 갱신용)
	UPROPERTY(BlueprintAssignable, Category = "CT|Menu")
	FCTPlayerNameChangedSignature OnPlayerNameChanged;

	// 온라인 세션 사용 가능 여부 (빠른 참가, 방 코드 입장)
	UFUNCTION(BlueprintPure, Category = "CT|Online")
	bool IsOnlineSessionAvailable() const { return bOnlineSessionAvailable; }

	// 메뉴 안내 문구 표시 (다음 화면에도 유지)
	void ShowMenuMessage(const FText& Message);

	// 접속 옵션 "?Name=..." (이름이 없으면 빈 문자열)
	FString MakeNameOption() const;

	// 무작위 방 입장, 온라인 불가면 안내만
	UFUNCTION(BlueprintCallable, Category = "CT|Online")
	void QuickJoin();

	// 방 코드 입장, 온라인 불가면 안내만
	UFUNCTION(BlueprintCallable, Category = "CT|Online")
	void JoinByRoomCode(const FString& RoomCode);

	// 게임 후 돌아갈 로비 맵 이름
	UFUNCTION(BlueprintPure, Category = "CT|Menu")
	FName GetLobbyMapName() const { return LobbyMapName; }

	// 로비 복귀 후에도 유지할 값 (방장 컴퓨터만)
	// 마지막 선택 맵 번호
	int32 GetStoredMapIndex() const { return StoredMapIndex; }
	void SetStoredMapIndex(int32 NewMapIndex) { StoredMapIndex = NewMapIndex; }

	// 로비에서 정한 경기 설정
	const FCTMatchSettings& GetStoredMatchSettings() const { return StoredMatchSettings; }
	void SetStoredMatchSettings(const FCTMatchSettings& NewSettings) { StoredMatchSettings = NewSettings; bHasStoredMatchSettings = true; }
	// 로비를 거쳐 설정이 정해졌는가
	bool HasStoredMatchSettings() const { return bHasStoredMatchSettings; }

	const FString& GetStoredRoomCode() const { return StoredRoomCode; }
	void SetStoredRoomCode(const FString& NewRoomCode) { StoredRoomCode = NewRoomCode; }

	// 로비가 게임을 시작할 때 이동할 맵 이름
	UFUNCTION(BlueprintPure, Category = "CT|Menu")
	FName GetGameMapName() const { return GameMapName; }

protected:
	// 온라인 세션(스팀) 사용 여부, 연결 전까지 끔
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Online")
	bool bOnlineSessionAvailable = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Menu")
	FName MainMenuMapName = TEXT("L_Mainmenu");

	// HostGame이 처음 여는 로비 맵, 비면 게임 맵
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

	// 입장 거절 코드를 안내 문구로 변환
	FText MakeJoinFailureMessage(const FString& ErrorString) const;

	// 마지막 입장 거절 안내 시각, 일반 안내 덮어쓰기 방지
	double LastJoinRejectTime = -1000.0;

	bool IsInMainMenuMap() const;

	void LoadProfile();
	void SaveProfile() const;

	bool StoreValidatedName(const FString& NewName, FText& OutError);

	FString PlayerName;

	FString StoredRoomCode;
	int32 StoredMapIndex = INDEX_NONE;

	FCTMatchSettings StoredMatchSettings;
	bool bHasStoredMatchSettings = false;

	FText PendingMessage;
	bool bIntroShown = false;
};
