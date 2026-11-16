// Copyright Moon Party Games. All Rights Reserved.
#include "CTSessionSubsystem.h"
#include "CTGameInstance.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "IEOSSDKManager.h"
#include "Online/Auth.h"
#include "Online/Lobbies.h"
#include "Online/OnlineAsyncOpHandle.h"
#include "Online/OnlineError.h"
#include "Online/OnlineIdEOSGS.h"
#include "Online/OnlineResult.h"
#include "Online/OnlineServices.h"
#include "Online/OnlineServicesRegistry.h"
#include "eos_connect.h"
#include "eos_sdk.h"

using namespace UE::Online;

#if !UE_BUILD_SHIPPING
static TAutoConsoleVariable<int32> CVarFreshDevice(TEXT("ct.EosFreshDeviceId"), 0, TEXT("1이면 시작할 때 기기 ID를 지우고 새로 만듦. 한 컴퓨터에서 두 사람 역할을 시험할 때 (개발용)"), ECVF_Default);
// 개발용 자동 시험 (Tools/CTEosFlowTest.ps1): 로그인이 끝나면 바로 방을 열거나, 방 코드로 들어감
static TAutoConsoleVariable<int32> CVarAutoHost(TEXT("ct.AutoHostAfterLogin"), 0, TEXT("1이면 EOS 로그인 뒤 바로 방 만들기 (개발용)"), ECVF_Default);
static TAutoConsoleVariable<FString> CVarAutoJoin(TEXT("ct.AutoJoinCode"), TEXT(""), TEXT("비어 있지 않으면 EOS 로그인 뒤 이 방 코드로 입장 (개발용)"), ECVF_Default);
#endif

namespace
{
	// 방 코드를 담는 방 속성 이름 (DefaultEngine.ini 의 [OnlineServices.Lobbies] 에 검색 가능 속성으로 등록되어 있음)
	const TCHAR* RoomCodeAttribute = TEXT("RoomCode");
	// 방은 기본 구조의 자식 구조로 만들어야 함
	const TCHAR* LobbySchema = TEXT("HeyChickenLobby");
	const FName LobbyLocalName(TEXT("GameSession"));

	constexpr int32 MaxRoomPlayers = 8;
	constexpr uint32 MaxSearchResults = 50;

	FString NormalizeCode(const FString& Code)
	{
		return Code.TrimStartAndEnd().ToUpper();
	}

	// EOS 비동기 호출 도중 서브시스템이 사라져도 안전하게 쓰려고 약한 참조를 씀
	using FWeakSession = TWeakObjectPtr<UCTSessionSubsystem>;
}

void UCTSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 게임을 켜면 바로 EOS에 로그인해 두어서, 방을 만들 때 게임 서버(P2P)가 EOS 주소로 열리게 함
	if (!IsRunningDedicatedServer() && !IsRunningCommandlet())
	{
		StartLogin();
	}
}

void UCTSessionSubsystem::Deinitialize()
{
	AfterLogin.Reset();
	Services.Reset();
	Platform.Reset();
	Super::Deinitialize();
}

bool UCTSessionSubsystem::IsAvailable() const
{
	return bLoggedIn || bLoggingIn;
}

FString UCTSessionSubsystem::GetServiceName() const
{
	return TEXT("EOS");
}

void UCTSessionSubsystem::ShowMessage(const FText& Message) const
{
	if (UCTGameInstance* GameInstance = Cast<UCTGameInstance>(GetGameInstance()))
	{
		GameInstance->ShowMenuMessage(Message);
	}
}

void UCTSessionSubsystem::EndBusy()
{
	bBusy = false;
}

// 로그인

void UCTSessionSubsystem::RunAfterLogin(TFunction<void()> Action)
{
	if (bLoggedIn)
	{
		Action();
		return;
	}

	AfterLogin.Add(MoveTemp(Action));
	if (!bLoggingIn) { StartLogin(); }
}

void UCTSessionSubsystem::FinishLogin(bool bSuccess)
{
	bLoggingIn = false;
	bLoggedIn = bSuccess;
	bLoginFailed = !bSuccess;
	UE_LOG(LogTemp, Log, TEXT("[CT] 온라인(EOS) 로그인 %s"), bSuccess ? TEXT("성공") : TEXT("실패"));

#if !UE_BUILD_SHIPPING
	if (bSuccess)
	{
		if (UCTGameInstance* GameInstance = Cast<UCTGameInstance>(GetGameInstance()))
		{
			if (CVarAutoHost.GetValueOnGameThread() > 0) { GameInstance->HostGame(); }
			else if (!CVarAutoJoin.GetValueOnGameThread().IsEmpty()) { JoinByRoomCode(CVarAutoJoin.GetValueOnGameThread()); }
		}
	}
#endif

	TArray<TFunction<void()>> Pending = MoveTemp(AfterLogin);
	AfterLogin.Reset();
	for (TFunction<void()>& Action : Pending)
	{
		if (bSuccess) { Action(); }
	}
	if (!bSuccess && !Pending.IsEmpty())
	{
		EndBusy();
		bCreating = false;
		ShowMessage(NSLOCTEXT("CT", "Menu_OnlineUnavailable", "온라인에 연결한 뒤 사용할 수 있습니다."));
	}
}

void UCTSessionSubsystem::StartLogin()
{
	if (bLoggingIn || bLoggedIn) { return; }
	bLoggingIn = true;
	bLoginFailed = false;

	Services = GetServices(EOnlineServices::Epic);
	IEOSSDKManager* Manager = IEOSSDKManager::Get();
	TArray<IEOSPlatformHandlePtr> Platforms = Manager ? Manager->GetActivePlatforms() : TArray<IEOSPlatformHandlePtr>();
	if (!Services || Platforms.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CT] EOS 시작 실패: 서비스나 플랫폼이 없음 (접속 정보 설정을 확인하세요)"));
		FinishLogin(false);
		return;
	}
	Platform = Platforms[0];

#if !UE_BUILD_SHIPPING
	if (CVarFreshDevice.GetValueOnGameThread() > 0)
	{
		// 같은 컴퓨터의 시험 인스턴스끼리 서로 다른 사람이 되도록 기기 ID를 지우고 다시 만듦
		EOS_Connect_DeleteDeviceIdOptions DeleteOptions = {};
		DeleteOptions.ApiVersion = EOS_CONNECT_DELETEDEVICEID_API_LATEST;
		FWeakSession* WeakSelf = new FWeakSession(this);
		EOS_Connect_DeleteDeviceId(EOS_Platform_GetConnectInterface(*Platform), &DeleteOptions, WeakSelf, [](const EOS_Connect_DeleteDeviceIdCallbackInfo* Data)
		{
			FWeakSession* Weak = static_cast<FWeakSession*>(Data->ClientData);
			if (UCTSessionSubsystem* Self = Weak->Get()) { Self->ContinueLogin(); }
			delete Weak;
		});
		return;
	}
#endif

	ContinueLogin();
}

void UCTSessionSubsystem::ContinueLogin()
{
	// 1) 이 컴퓨터의 기기 ID 만들기 (이미 있으면 DuplicateNotAllowed 로 오며 정상)
	struct FLoginContext { FWeakSession Self; };
	FLoginContext* Context = new FLoginContext{ FWeakSession(this) };

	EOS_HConnect Connect = EOS_Platform_GetConnectInterface(*Platform);
	EOS_Connect_CreateDeviceIdOptions CreateOptions = {};
	CreateOptions.ApiVersion = EOS_CONNECT_CREATEDEVICEID_API_LATEST;
	CreateOptions.DeviceModel = "PC";
	EOS_Connect_CreateDeviceId(Connect, &CreateOptions, Context, [](const EOS_Connect_CreateDeviceIdCallbackInfo* Data)
	{
		FLoginContext* Ctx = static_cast<FLoginContext*>(Data->ClientData);
		UCTSessionSubsystem* Self = Ctx->Self.Get();
		const bool bOk = Data->ResultCode == EOS_EResult::EOS_Success || Data->ResultCode == EOS_EResult::EOS_DuplicateNotAllowed;
		if (!Self || !bOk)
		{
			UE_LOG(LogTemp, Warning, TEXT("[CT] EOS 기기 ID 준비 실패: %s"), UTF8_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
			if (Self) { Self->FinishLogin(false); }
			delete Ctx;
			return;
		}

		// 2) 기기 ID로 EOS 연결 로그인
		EOS_HConnect ConnectHandle = EOS_Platform_GetConnectInterface(*Self->Platform);
		EOS_Connect_Credentials Credentials = {};
		Credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
		Credentials.Token = nullptr;
		Credentials.Type = EOS_EExternalCredentialType::EOS_ECT_DEVICEID_ACCESS_TOKEN;
		EOS_Connect_UserLoginInfo LoginInfo = {};
		LoginInfo.ApiVersion = EOS_CONNECT_USERLOGININFO_API_LATEST;
		LoginInfo.DisplayName = "Player";
		EOS_Connect_LoginOptions LoginOptions = {};
		LoginOptions.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
		LoginOptions.Credentials = &Credentials;
		LoginOptions.UserLoginInfo = &LoginInfo;
		EOS_Connect_Login(ConnectHandle, &LoginOptions, Ctx, [](const EOS_Connect_LoginCallbackInfo* LoginData)
		{
			FLoginContext* LoginCtx = static_cast<FLoginContext*>(LoginData->ClientData);
			UCTSessionSubsystem* LoginSelf = LoginCtx->Self.Get();
			if (!LoginSelf || LoginData->ResultCode != EOS_EResult::EOS_Success)
			{
				UE_LOG(LogTemp, Warning, TEXT("[CT] EOS 연결 로그인 실패: %s"), UTF8_TO_TCHAR(EOS_EResult_ToString(LoginData->ResultCode)));
				if (LoginSelf) { LoginSelf->FinishLogin(false); }
				delete LoginCtx;
				return;
			}
			delete LoginCtx;

			// 3) 온라인 서비스(방 만들기와 찾기)용 로그인
			FWeakSession Weak(LoginSelf);
			FAuthLogin::Params Params;
			Params.PlatformUserId = FPlatformMisc::GetPlatformUserForUserIndex(0);
			Params.CredentialsType = LoginCredentialsType::ExternalAuth;
			Params.CredentialsToken.Set<FExternalAuthToken>(FExternalAuthToken{ ExternalLoginType::DeviceIdAccessToken, FString() });
			LoginSelf->Services->GetAuthInterface()->Login(MoveTemp(Params)).OnComplete([Weak](const TOnlineResult<FAuthLogin>& Result)
			{
				UCTSessionSubsystem* ServiceSelf = Weak.Get();
				if (!ServiceSelf) { return; }
				if (Result.IsOk())
				{
					ServiceSelf->LocalAccount = Result.GetOkValue().AccountInfo->AccountId;
					ServiceSelf->FinishLogin(true);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[CT] EOS 서비스 로그인 실패: %s"), *Result.GetErrorValue().GetLogString());
					ServiceSelf->FinishLogin(false);
				}
			});
		});
	});
}

// 방 열기 / 닫기

void UCTSessionSubsystem::CreateRoom(const FString& RoomCode)
{
	if (RoomCode.IsEmpty()) { return; }

	// 로비 복귀 후에도 같은 방 유지
	if (bHasLobby || bCreating) { return; }
	bCreating = true;

	const FString Code = NormalizeCode(RoomCode);
	RunAfterLogin([this, Code]()
	{
		ILobbiesPtr Lobbies = Services ? Services->GetLobbiesInterface() : nullptr;
		if (!Lobbies)
		{
			bCreating = false;
			UE_LOG(LogTemp, Warning, TEXT("[CT] 방 열기 실패 (EOS 방 서비스 없음)"));
			return;
		}

		FCreateLobby::Params Params;
		Params.LocalAccountId = LocalAccount;
		Params.LocalName = LobbyLocalName;
		Params.SchemaId = FSchemaId(LobbySchema);
		Params.MaxMembers = MaxRoomPlayers;
		Params.JoinPolicy = ELobbyJoinPolicy::PublicAdvertised;
		Params.Attributes.Add(FSchemaAttributeId(RoomCodeAttribute), FSchemaVariant(Code));

		FWeakSession Weak(this);
		Lobbies->CreateLobby(MoveTemp(Params)).OnComplete([Weak](const TOnlineResult<FCreateLobby>& Result)
		{
			UCTSessionSubsystem* Self = Weak.Get();
			if (!Self) { return; }
			Self->bCreating = false;
			if (Result.IsOk())
			{
				Self->CurrentLobby = Result.GetOkValue().Lobby->LobbyId;
				Self->bHasLobby = true;
				UE_LOG(LogTemp, Log, TEXT("[CT] 방 열기 성공 (EOS)"));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[CT] 방 열기 실패 (EOS): %s"), *Result.GetErrorValue().GetLogString());
			}
		});
	});
}

void UCTSessionSubsystem::CloseRoom()
{
	if (!bHasLobby || !Services) { return; }

	ILobbiesPtr Lobbies = Services->GetLobbiesInterface();
	if (!Lobbies) { return; }

	FLeaveLobby::Params Params;
	Params.LocalAccountId = LocalAccount;
	Params.LobbyId = CurrentLobby;
	bHasLobby = false;
	Lobbies->LeaveLobby(MoveTemp(Params)).OnComplete([](const TOnlineResult<FLeaveLobby>& Result)
	{
		if (!Result.IsOk())
		{
			UE_LOG(LogTemp, Warning, TEXT("[CT] 방 나가기 실패 (EOS): %s"), *Result.GetErrorValue().GetLogString());
		}
	});
}

// 찾기 / 입장

void UCTSessionSubsystem::JoinByRoomCode(const FString& RoomCode)
{
	const FString Code = NormalizeCode(RoomCode);
	if (Code.IsEmpty())
	{
		ShowMessage(NSLOCTEXT("CT", "Menu_EmptyRoomCode", "방 코드를 입력하세요."));
		return;
	}
	StartSearch(EJoinMode::Code, Code);
}

void UCTSessionSubsystem::QuickJoin()
{
	StartSearch(EJoinMode::Quick, FString());
}

void UCTSessionSubsystem::StartSearch(EJoinMode Mode, const FString& RoomCode)
{
	if (bBusy) { return; }
	bBusy = true;
	ShowMessage(NSLOCTEXT("CT", "Menu_Searching", "방을 찾는 중입니다..."));

	// 이전에 남은 방이 있으면 먼저 나감
	CloseRoom();

	RunAfterLogin([this, Mode, RoomCode]()
	{
		ILobbiesPtr Lobbies = Services ? Services->GetLobbiesInterface() : nullptr;
		if (!Lobbies)
		{
			EndBusy();
			ShowMessage(NSLOCTEXT("CT", "Menu_JoinFailed", "방에 입장하지 못했습니다."));
			return;
		}

		FFindLobbies::Params Find;
		Find.LocalAccountId = LocalAccount;
		Find.MaxResults = MaxSearchResults;
		if (Mode == EJoinMode::Code)
		{
			Find.Filters.Add(FFindLobbySearchFilter{ FSchemaAttributeId(RoomCodeAttribute), ESchemaAttributeComparisonOp::Equals, FSchemaVariant(RoomCode) });
		}

		FWeakSession Weak(this);
		Lobbies->FindLobbies(MoveTemp(Find)).OnComplete([Weak, Mode](const TOnlineResult<FFindLobbies>& Result)
		{
			UCTSessionSubsystem* Self = Weak.Get();
			if (!Self) { return; }

			// 입장 가능한 방만 (자리가 있고 내가 연 방이 아닌 것)
			TArray<TSharedRef<const FLobby>> Candidates;
			if (Result.IsOk())
			{
				for (const TSharedRef<const FLobby>& Lobby : Result.GetOkValue().Lobbies)
				{
					if (Lobby->Members.Num() >= Lobby->MaxMembers) { continue; }
					if (Lobby->OwnerAccountId == Self->LocalAccount) { continue; }
					Candidates.Add(Lobby);
				}
			}

			if (Candidates.IsEmpty())
			{
				Self->EndBusy();
				Self->ShowMessage(Mode == EJoinMode::Code
					? NSLOCTEXT("CT", "Menu_RoomNotFound", "해당 방 코드를 찾을 수 없습니다.")
					: NSLOCTEXT("CT", "Menu_NoOpenRoom", "참가할 수 있는 방이 없습니다."));
				return;
			}

			const TSharedRef<const FLobby> Chosen = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];

			FJoinLobby::Params Join;
			Join.LocalAccountId = Self->LocalAccount;
			Join.LocalName = LobbyLocalName;
			Join.LobbyId = Chosen->LobbyId;
			Self->Services->GetLobbiesInterface()->JoinLobby(MoveTemp(Join)).OnComplete([Weak](const TOnlineResult<FJoinLobby>& JoinResult)
			{
				UCTSessionSubsystem* JoinSelf = Weak.Get();
				if (!JoinSelf) { return; }
				JoinSelf->EndBusy();

				if (!JoinResult.IsOk())
				{
					UE_LOG(LogTemp, Warning, TEXT("[CT] 방 입장 실패 (EOS): %s"), *JoinResult.GetErrorValue().GetLogString());
					JoinSelf->ShowMessage(NSLOCTEXT("CT", "Menu_JoinFailed", "방에 입장하지 못했습니다."));
					return;
				}

				const TSharedPtr<const FLobby>& Lobby = JoinResult.GetOkValue().Lobby;
				JoinSelf->CurrentLobby = Lobby->LobbyId;
				JoinSelf->bHasLobby = true;

				// 방장의 EOS 주소로 게임에 접속 (EOS 세션이 쓰는 주소 모양: [EOS:사용자ID]:포트)
				char Buffer[EOS_PRODUCTUSERID_MAX_LENGTH + 1] = {};
				int32_t Length = sizeof(Buffer);
				EOS_ProductUserId_ToString(GetProductUserIdChecked(Lobby->OwnerAccountId), Buffer, &Length);
				const FString Address = FString::Printf(TEXT("[EOS:%s]:0"), UTF8_TO_TCHAR(Buffer));

				UGameInstance* GameInstance = JoinSelf->GetGameInstance();
				APlayerController* Controller = GameInstance ? GameInstance->GetFirstLocalPlayerController() : nullptr;
				if (!Controller)
				{
					JoinSelf->CloseRoom();
					JoinSelf->ShowMessage(NSLOCTEXT("CT", "Menu_JoinFailed", "방에 입장하지 못했습니다."));
					return;
				}

				// 이름은 접속 옵션으로 전달
				const UCTGameInstance* CTGameInstance = Cast<UCTGameInstance>(GameInstance);
				const FString NameOption = CTGameInstance ? CTGameInstance->MakeNameOption() : FString();
				UE_LOG(LogTemp, Log, TEXT("[CT] 방 입장: %s"), *Address);
				Controller->ClientTravel(Address + NameOption, TRAVEL_Absolute);
			});
		});
	});
}
