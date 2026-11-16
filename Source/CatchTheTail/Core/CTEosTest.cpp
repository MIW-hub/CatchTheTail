// Copyright Moon Party Games. All Rights Reserved.
// 개발용 EOS 시험 (출시 빌드에는 들어가지 않음). 에픽 계정 없이 기기 ID로 로그인하고, 방(로비)을 만들고 방 코드로 찾아 들어가는지 확인함.
//   ct.EosLogin                 기기 ID로 로그인만 시험
//   ct.EosHost <코드>           로그인 -> 방 만들기(방 코드 등록)
//   ct.EosJoin <코드>           로그인 -> 방 코드로 찾기 -> 들어가기
// 결과는 [CT-EOS] 로그로 남김. 같은 컴퓨터에서 두 사람이 되려면 실행마다 기기 ID를 새로 만들어야 해서 매번 지우고 다시 만듦.
#if !UE_BUILD_SHIPPING

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "IEOSSDKManager.h"
#include "Online/OnlineServices.h"
#include "Online/OnlineServicesRegistry.h"
#include "Online/Auth.h"
#include "Online/OnlineError.h"
#include "Online/OnlineResult.h"
#include "Online/OnlineAsyncOpHandle.h"
#include "Online/Lobbies.h"
#include "Online/OnlineIdEOSGS.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "eos_sdk.h"
#include "eos_connect.h"

namespace CTEosTestImpl
{
	using namespace UE::Online;

	// 시험 동안 붙잡아 두는 것들 (놓으면 EOS가 꺼짐)
	IEOSPlatformHandlePtr GEosTestPlatform;
	TSharedPtr<IOnlineServices> GEosTestServices;
	FAccountId GEosTestAccount;
	TFunction<void(FAccountId)> GEosAfterLogin;

	FString ErrorText(const FOnlineError& Error)
	{
		return Error.GetLogString();
	}

	// 5) Online Services 쪽 로그인 (기기 ID). 성공하면 GEosAfterLogin 호출
	void StartServicesLogin()
	{
		IAuthPtr Auth = GEosTestServices->GetAuthInterface();
		FAuthLogin::Params Params;
		Params.PlatformUserId = FPlatformMisc::GetPlatformUserForUserIndex(0);
		Params.CredentialsType = LoginCredentialsType::ExternalAuth;
		Params.CredentialsToken.Set<FExternalAuthToken>(FExternalAuthToken{ ExternalLoginType::DeviceIdAccessToken, FString() });
		Auth->Login(MoveTemp(Params)).OnComplete([](const TOnlineResult<FAuthLogin>& Result)
		{
			if (Result.IsOk())
			{
				GEosTestAccount = Result.GetOkValue().AccountInfo->AccountId;
				UE_LOG(LogTemp, Log, TEXT("[CT-EOS] 서비스 로그인 성공: %s"), *ToLogString(GEosTestAccount));
				if (GEosAfterLogin) { GEosAfterLogin(GEosTestAccount); }
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[CT-EOS] 서비스 로그인 실패: %s"), *ErrorText(Result.GetErrorValue()));
			}
		});
	}

	void OnRawLoginComplete(const EOS_Connect_LoginCallbackInfo* Data)
	{
		if (Data->ResultCode == EOS_EResult::EOS_Success)
		{
			char Buffer[EOS_PRODUCTUSERID_MAX_LENGTH + 1] = {};
			int32_t Length = sizeof(Buffer);
			EOS_ProductUserId_ToString(Data->LocalUserId, Buffer, &Length);
			UE_LOG(LogTemp, Log, TEXT("[CT-EOS] 로그인 성공: 제품 사용자 ID %s"), UTF8_TO_TCHAR(Buffer));
			if (GEosAfterLogin) { StartServicesLogin(); }
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[CT-EOS] 로그인 실패: %s"), UTF8_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
		}
	}

	void StartRawLogin(EOS_HPlatform Platform)
	{
		EOS_HConnect Connect = EOS_Platform_GetConnectInterface(Platform);

		EOS_Connect_Credentials Credentials = {};
		Credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
		Credentials.Token = nullptr; // 기기 ID 로그인은 토큰이 없어야 함
		Credentials.Type = EOS_EExternalCredentialType::EOS_ECT_DEVICEID_ACCESS_TOKEN;

		EOS_Connect_UserLoginInfo LoginInfo = {};
		LoginInfo.ApiVersion = EOS_CONNECT_USERLOGININFO_API_LATEST;
		LoginInfo.DisplayName = "Player";

		EOS_Connect_LoginOptions Options = {};
		Options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
		Options.Credentials = &Credentials;
		Options.UserLoginInfo = &LoginInfo;

		EOS_Connect_Login(Connect, &Options, nullptr, [](const EOS_Connect_LoginCallbackInfo* Data) { OnRawLoginComplete(Data); });
	}

	void CreateDeviceIdThenLogin()
	{
		EOS_HConnect Connect = EOS_Platform_GetConnectInterface(*GEosTestPlatform);
		EOS_Connect_CreateDeviceIdOptions Options = {};
		Options.ApiVersion = EOS_CONNECT_CREATEDEVICEID_API_LATEST;
		Options.DeviceModel = "PC";
		EOS_Connect_CreateDeviceId(Connect, &Options, nullptr, [](const EOS_Connect_CreateDeviceIdCallbackInfo* Data)
		{
			// 이미 만들어져 있으면 DuplicateNotAllowed 가 오는데 정상
			if (Data->ResultCode == EOS_EResult::EOS_Success || Data->ResultCode == EOS_EResult::EOS_DuplicateNotAllowed)
			{
				UE_LOG(LogTemp, Log, TEXT("[CT-EOS] 기기 ID 준비됨 (%s). 로그인 시작"), UTF8_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
				StartRawLogin(*GEosTestPlatform);
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[CT-EOS] 기기 ID 만들기 실패: %s"), UTF8_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
			}
		});
	}

	// 같은 컴퓨터의 시험 인스턴스끼리 서로 다른 사람이 되도록 기기 ID를 지우고 새로 만듦
	void DeleteDeviceIdThenCreate()
	{
		EOS_HConnect Connect = EOS_Platform_GetConnectInterface(*GEosTestPlatform);
		EOS_Connect_DeleteDeviceIdOptions Options = {};
		Options.ApiVersion = EOS_CONNECT_DELETEDEVICEID_API_LATEST;
		EOS_Connect_DeleteDeviceId(Connect, &Options, nullptr, [](const EOS_Connect_DeleteDeviceIdCallbackInfo* Data)
		{
			UE_LOG(LogTemp, Log, TEXT("[CT-EOS] 기존 기기 ID 지우기: %s"), UTF8_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
			CreateDeviceIdThenLogin();
		});
	}

	bool PrepareEos()
	{
		GEosTestServices = GetServices(EOnlineServices::Epic);
		if (!GEosTestServices)
		{
			UE_LOG(LogTemp, Log, TEXT("[CT-EOS] 서비스를 만들지 못함 (설정 값이나 플러그인 확인 필요)"));
			return false;
		}

		IEOSSDKManager* Manager = IEOSSDKManager::Get();
		TArray<IEOSPlatformHandlePtr> Platforms = Manager ? Manager->GetActivePlatforms() : TArray<IEOSPlatformHandlePtr>();
		if (Platforms.IsEmpty())
		{
			UE_LOG(LogTemp, Log, TEXT("[CT-EOS] EOS 플랫폼이 없음 (설정 값 확인 필요)"));
			return false;
		}
		GEosTestPlatform = Platforms[0];
		UE_LOG(LogTemp, Log, TEXT("[CT-EOS] EOS 플랫폼 준비됨"));
		return true;
	}

	void RunEosLoginTest(const TArray<FString>&)
	{
		GEosAfterLogin = nullptr;
		if (PrepareEos()) { CreateDeviceIdThenLogin(); }
	}

	// ---- 방 만들기 ----
	// bOpenGame 이 true 면 방을 만든 뒤 로비 맵을 열어 게임 서버(EOS 접속)를 시작함
	void RunEosHostImpl(const TArray<FString>& Args, bool bOpenGame)
	{
		const FString Code = Args.IsValidIndex(0) ? Args[0] : TEXT("TEST01");
		if (!PrepareEos()) { return; }

		GEosAfterLogin = [Code, bOpenGame](FAccountId Account)
		{
			ILobbiesPtr Lobbies = GEosTestServices->GetLobbiesInterface();
			if (!Lobbies) { UE_LOG(LogTemp, Log, TEXT("[CT-EOS] 방 서비스가 없음")); return; }

			FCreateLobby::Params Params;
			Params.LocalAccountId = Account;
			Params.LocalName = FName(TEXT("GameSession"));
			Params.SchemaId = FSchemaId(TEXT("HeyChickenLobby"));
			Params.MaxMembers = 8;
			Params.JoinPolicy = ELobbyJoinPolicy::PublicAdvertised;
			Params.Attributes.Add(FSchemaAttributeId(TEXT("RoomCode")), FSchemaVariant(Code));
			Lobbies->CreateLobby(MoveTemp(Params)).OnComplete([Code, bOpenGame](const TOnlineResult<FCreateLobby>& Result)
			{
				if (Result.IsOk())
				{
					UE_LOG(LogTemp, Log, TEXT("[CT-EOS] 방 만들기 성공: 코드 %s, 방 번호 %s"), *Code, *ToLogString(Result.GetOkValue().Lobby->LobbyId));
					if (bOpenGame && GEngine)
					{
						UE_LOG(LogTemp, Log, TEXT("[CT-EOS] 게임 서버 시작: 로비 맵 열기"));
						GEngine->Exec(GEngine->GetWorldContexts()[0].World(), TEXT("open /Game/Levels/L_Lobby?listen"));
					}
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("[CT-EOS] 방 만들기 실패: %s"), *ErrorText(Result.GetErrorValue()));
				}
			});
		};
		DeleteDeviceIdThenCreate();
	}

	void RunEosHost(const TArray<FString>& Args) { RunEosHostImpl(Args, false); }
	void RunEosGameHost(const TArray<FString>& Args) { RunEosHostImpl(Args, true); }

	// ---- 방 코드로 찾고 들어가기 ----
	// bConnectGame 이 true 면 방에 들어간 뒤 방장의 EOS 주소로 게임에 접속함
	void RunEosJoinImpl(const TArray<FString>& Args, bool bConnectGame)
	{
		const FString Code = Args.IsValidIndex(0) ? Args[0] : TEXT("TEST01");
		if (!PrepareEos()) { return; }

		GEosAfterLogin = [Code, bConnectGame](FAccountId Account)
		{
			ILobbiesPtr Lobbies = GEosTestServices->GetLobbiesInterface();
			if (!Lobbies) { UE_LOG(LogTemp, Log, TEXT("[CT-EOS] 방 서비스가 없음")); return; }

			FFindLobbies::Params Find;
			Find.LocalAccountId = Account;
			Find.Filters.Add(FFindLobbySearchFilter{ FSchemaAttributeId(TEXT("RoomCode")), ESchemaAttributeComparisonOp::Equals, FSchemaVariant(Code) });
			Lobbies->FindLobbies(MoveTemp(Find)).OnComplete([Code, Account, Lobbies, bConnectGame](const TOnlineResult<FFindLobbies>& Result)
			{
				if (!Result.IsOk())
				{
					UE_LOG(LogTemp, Log, TEXT("[CT-EOS] 방 찾기 실패: %s"), *ErrorText(Result.GetErrorValue()));
					return;
				}
				const TArray<TSharedRef<const FLobby>>& Found = Result.GetOkValue().Lobbies;
				UE_LOG(LogTemp, Log, TEXT("[CT-EOS] 방 찾기 결과: %d개"), Found.Num());
				if (Found.IsEmpty()) { return; }

				FJoinLobby::Params Join;
				Join.LocalAccountId = Account;
				Join.LocalName = FName(TEXT("GameSession"));
				Join.LobbyId = Found[0]->LobbyId;
				Lobbies->JoinLobby(MoveTemp(Join)).OnComplete([bConnectGame](const TOnlineResult<FJoinLobby>& JoinResult)
				{
					if (JoinResult.IsOk())
					{
						const TSharedPtr<const FLobby>& Lobby = JoinResult.GetOkValue().Lobby;
						UE_LOG(LogTemp, Log, TEXT("[CT-EOS] 방 들어가기 성공: 방장 %s, 인원 %d명"), *ToLogString(Lobby->OwnerAccountId), Lobby->Members.Num());
						if (bConnectGame && GEngine)
						{
							// 방장의 제품 사용자 ID로 EOS 접속 주소를 만들어 접속
							char Buffer[EOS_PRODUCTUSERID_MAX_LENGTH + 1] = {};
							int32_t Length = sizeof(Buffer);
							EOS_ProductUserId_ToString(GetProductUserIdChecked(Lobby->OwnerAccountId), Buffer, &Length);
							// EOS 세션이 쓰는 접속 주소 모양: [EOS:사용자ID]:포트 (대괄호 안에 넣어야 주소로 읽힘)
							const FString Address = FString::Printf(TEXT("[EOS:%s]:0"), UTF8_TO_TCHAR(Buffer));
							UE_LOG(LogTemp, Log, TEXT("[CT-EOS] 게임 접속 시도: %s"), *Address);
							GEngine->Exec(GEngine->GetWorldContexts()[0].World(), *FString::Printf(TEXT("open %s"), *Address));
						}
					}
					else
					{
						UE_LOG(LogTemp, Log, TEXT("[CT-EOS] 방 들어가기 실패: %s"), *ErrorText(JoinResult.GetErrorValue()));
					}
				});
			});
		};
		DeleteDeviceIdThenCreate();
	}

	void RunEosJoin(const TArray<FString>& Args) { RunEosJoinImpl(Args, false); }
	void RunEosGameJoin(const TArray<FString>& Args) { RunEosJoinImpl(Args, true); }

	FAutoConsoleCommand CmdEosLogin(TEXT("ct.EosLogin"), TEXT("EOS 기기 ID 로그인 시험 (개발용)"), FConsoleCommandWithArgsDelegate::CreateStatic(&RunEosLoginTest));
	FAutoConsoleCommand CmdEosHost(TEXT("ct.EosHost"), TEXT("EOS 방 만들기 시험: ct.EosHost <코드> (개발용)"), FConsoleCommandWithArgsDelegate::CreateStatic(&RunEosHost));
	FAutoConsoleCommand CmdEosGameHost(TEXT("ct.EosGameHost"), TEXT("EOS 방 만들고 게임 서버 시작: ct.EosGameHost <코드> (개발용)"), FConsoleCommandWithArgsDelegate::CreateStatic(&RunEosGameHost));
	FAutoConsoleCommand CmdEosGameJoin(TEXT("ct.EosGameJoin"), TEXT("EOS 방 찾아 들어가고 게임 접속: ct.EosGameJoin <코드> (개발용)"), FConsoleCommandWithArgsDelegate::CreateStatic(&RunEosGameJoin));
	FAutoConsoleCommand CmdEosJoin(TEXT("ct.EosJoin"), TEXT("EOS 방 찾아 들어가기 시험: ct.EosJoin <코드> (개발용)"), FConsoleCommandWithArgsDelegate::CreateStatic(&RunEosJoin));
}

#endif // !UE_BUILD_SHIPPING
