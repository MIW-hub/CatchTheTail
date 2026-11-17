# 03. 네트워크 · 온라인 · UI · 환경 기믹 · 시험 도구

> 대상: Hey Chicken! (UE 5.7, 모듈 `CatchTheTail`, C++ 접두사 `CT`) / 브랜치 `feature/eos` 기준.
> 소스 코드와 `docs/EOS_Setup.md`, `Config/DefaultEngine.ini`를 읽고 작성했습니다. 접속 비밀값(ClientSecret 등)은 저장소에 없으며(`Config/Windows/WindowsEngine.ini`, `.gitignore` 대상) 이 문서에도 적지 않습니다.

---

## 1. 네트워크 구조

**목적**: 전용 서버 없이 한 명이 방장이 되어 방을 여는 리슨 서버 방식으로 3~8명이 함께 한다.

**동작 흐름**
1. 방장은 `UCTGameInstance::HostGame()`에서 `OpenLevel(L_Lobby, "listen" + ?Name=...)`으로 리슨 서버를 연다. 방장 PC가 서버이자 플레이어다 (`ACTLobbyGameMode::InitLobbyPlayer`: listen 서버의 로컬 플레이어가 방장).
2. 참가자는 EOS 로비 검색 결과의 방장 주소(`[EOS:사용자ID]:0`)로 `ClientTravel`한다 (2장).
3. 로비 → 게임 맵은 `ServerTravel` + 심리스 트래블(`bUseSeamlessTravel = true`)로 이동한다. `ACTPlayerState::CopyProperties`가 로비에서 정한 `TeamId`를 새 PlayerState로 옮긴다.

**서버 권한 원칙**
- 규칙 판단과 값 변경은 서버(GameMode, PlayerState setter)에서 한다. `ACTPlayerState`의 setter는 `if (!HasAuthority()) return;`으로 시작한다. 환경 기믹의 적중, 폭발도 `HasAuthority()`일 때만 처리한다.
- 클라이언트는 요청만 보낸다. 로비 요청은 모두 `Server, Reliable` RPC → GameMode 함수에서 재검증한다: `ServerSetReady`, `ServerStartGame`, `ServerSelectMap`, `ServerChangeMatchSetting`, `ServerChangeTeam`, `ServerAcceptTeamSwap`, `ServerSendChat` (`CTLobbyPlayerController`). 방장 전용 동작은 서버가 `IsHost()`를 다시 확인한다.
- 채팅은 서버가 다시 검사한다: 공백 제거, 최대 100자(`MaxChatLength`), 0.3초 간격(`ChatCooldownSeconds`), `FCTChatFilter::Sanitize`로 비속어 가림 후 `BroadcastChat` → 각 컨트롤러의 `ClientReceiveChat`(Client, Reliable).

**복제 방식**

| 대상 | 방식 | 비고 |
|---|---|---|
| 로비·경기 공용 상태 | `ACTGameState`의 `DOREPLIFETIME` 속성 (`Phase`, `RoomCode`, `MatchSettings`, `SelectedMapIndex`, `TeamColors`, `TeamSwapRequests` 등) | 설정과 팀 교환 요청이 GameState 복제로 모든 클라이언트에 전파 |
| 플레이어 상태 | `ACTPlayerState` 속성 + `OnRep_*` → `OnPlayerStateChanged` 델리게이트 → UI 갱신 | `NetUpdateFrequency = 10`, 서버에서 값을 바꿀 때 `ForceNetUpdate` 후 `OnRep_*`를 직접 호출해 호스트 화면도 같은 경로로 갱신 |
| 비밀 유지 | `COND_OwnerOnly`: `TargetColor`, `Energy`, `TeamEnergy` | 상대 팀 정보를 클라이언트가 알 수 없게 함 (결과 화면은 경기 후 공개되는 `ResultEnergy` 사용) |
| 시간 | `GetServerWorldTimeSeconds()`를 복제된 시각과 비교 | 카운트다운, 요청 만료, 전화 쿨타임, 통나무/차량 위치 모두 이 방식 |

**RPC 사용 원칙 (코드에서 확인되는 것)**
- 입력 → 서버 요청은 `Server, Reliable`, 서버 → 개인 알림은 `Client, Reliable`. 지속 상태는 RPC가 아니라 복제 속성으로 둔다.
- 요청 함수(`RequestXxx`)는 공개 래퍼, 실제 RPC(`ServerXxx`)는 `private`이라 UI는 래퍼만 호출한다.
- 방장이 아닌 사람의 요청, 카운트다운 중 요청은 서버에서 조용히 무시(`return false`)한다.

**접속 거절·끊김** (`UCTGameInstance`)
- `PreLogin`: 로비는 시작 확정 후 입장 → `CTJoinReject::LobbyStarting`, 현재 인원 ≥ `MatchSettings.MaxPlayers` → `LobbyFull` (`ACTLobbyGameMode`). 게임 중 입장은 `GameInProgress` (`ACTGameMode`). 클라이언트는 `HandleNetworkFailure`에서 문자열을 안내 문구로 바꾼다 (`MakeJoinFailureMessage`).
- 연결이 끊긴 참가자는 `LeaveToMainMenu("방장이 방을 닫았거나 연결이 끊어졌습니다.")`로 메인 메뉴로 간다. 방장 쪽 네트워크 오류는 무시.
- 이동 실패(`HandleTravelFailure`)는 3초 안에 거절 사유를 안내했다면 덮어쓰지 않는다.

**핵심 설정 (`Config/DefaultEngine.ini`)**
- `[/Script/Engine.GameSession] MaxPlayers=8`, `bRequiresPushToTalk=True`
- `[/Script/OnlineSubsystemUtils.IpNetDriver] NetServerMaxTickRate=60, MaxClientRate=100000, MaxInternetClientRate=100000` (빠른 이동 파티 게임이라 기본 30 대신 60)
- `net.AllowPIESeamlessTravel=1`: 에디터 PIE에서도 로비 복귀가 패키지와 같게 동작
- 로비: 시작 버튼 후 `StartDelaySeconds=3`초, `MinPlayersToStart=3` (UI용 `LobbyMinPlayers`로 복제)

**주의점**
- 방장이 나가면 방이 닫힌다 (리슨 서버의 한계, 방장 이전 없음).
- `GameInstance`가 방 코드, 선택 맵, 경기 설정, 팀 색을 저장해 두어(`GetStored*`) 게임 후 로비로 돌아와도 유지된다. 새 방(`HostGame`)과 `LeaveToMainMenu`에서는 모두 초기화한다.

---

## 2. EOS 온라인 연결

**목적**: 에픽 계정 없이 기기 ID로 로그인하고, 방(EOS Lobby)에 방 코드를 달아 두어 코드로 찾아 들어오게 한다. 게임 접속은 EOS P2P(중계는 에픽 서버)로 한다.

**구성 요소**
- 플러그인(`.uproject`): EOSShared, OnlineServices, OnlineServicesEpicCommon, OnlineServicesEOSGS, SocketSubsystemEOS. `OnlineSubsystemEOS`는 에픽 계정 로그인만 지원해 기기 ID 로그인이 안 되므로 쓰지 않는다.
- 담당 클래스: `UCTSessionSubsystem`(`UGameInstanceSubsystem`). `Initialize`에서 전용 서버와 커맨드릿이 아니면 바로 `StartLogin()`.
- 접속 정보 6개(ProductId, SandboxId, DeploymentId, ClientId, ClientSecret, ClientEncryptionKey)는 `Config/Windows/WindowsEngine.ini`의 `[OnlineServices.EOS]`에 둔다 (저장소에 올라가지 않는 파일). 샌드박스는 `Live` 하나, 클라이언트 정책은 `Peer2Peer`(Sessions, Lobbies, Voice 포함).

**로그인 흐름** (`StartLogin` → `ContinueLogin` → `FinishLogin`)
1. 활성 플랫폼 핸들(`IEOSSDKManager`)과 `UE::Online::GetServices(Epic)`를 확인. 없으면 실패.
2. `EOS_Connect_CreateDeviceId` (모델명 "PC"). 이미 있으면 `DuplicateNotAllowed`가 오며 정상으로 취급.
3. `EOS_Connect_Login` (`EOS_ECT_DEVICEID_ACCESS_TOKEN`, 표시 이름 "Player").
4. 온라인 서비스 Auth `Login` (`ExternalAuth`, `DeviceIdAccessToken`) → `LocalAccount` 저장 → `FinishLogin(true)`.
5. 성공하면 `EnsureVoiceSession()`을 호출하고 로그인 대기 중이던 동작(`AfterLogin` 큐)을 실행. 실패하면 대기 동작을 버리고 "온라인에 연결한 뒤 사용할 수 있습니다." 안내.
- 모든 EOS 콜백은 `TWeakObjectPtr`로 서브시스템을 잡아, 비동기 도중 서브시스템이 사라져도 안전하다.
- 기기 ID는 컴퓨터에 묶인다. 다른 PC로 옮기면 다른 사용자가 된다 (이름은 게임에서 따로 저장: `CTProfile` 세이브 슬롯).

**방 만들기** (`CreateRoom`, 로비 진입 시 `ACTLobbyGameMode::InitGameState`가 호출)
- 방 코드: `MakeRoomCode()`가 `ABCDEFGHJKMNPQRSTUVWXYZ23456789`(0/O, 1/I/L 제외)에서 6글자를 뽑아 `GameInstance`에 저장하고 `GameState.RoomCode`로 복제.
- `FCreateLobby`: 최대 8명(`MaxRoomPlayers`), `JoinPolicy = PublicAdvertised`, 구조 `HeyChickenLobby`, 로컬 이름 `GameSession`, 속성 `RoomCode`.
- 이미 방이 있거나(`bHasLobby`) 만드는 중(`bCreating`)이면 무시한다. 게임 후 로비로 돌아와도 같은 방을 유지한다.

**방 구조 설정** (`DefaultEngine.ini [OnlineServices.Lobbies]`)
- 검색 가능한 속성은 기본 구조 `LobbyBase`에만 둘 수 있어, 기본 구조 카테고리의 속성 목록을 지우고 `RoomCode`를 더해 다시 등록한다 (`Flags=("Public","Searchable"), MaxSize=16`).
- 방은 `LobbyBase`의 자식 구조 `HeyChickenLobby`(비어 있음)로 만든다. 자식 구조는 카테고리를 가질 수 없다.

**검색과 입장** (`JoinByRoomCode`, `QuickJoin` → `StartSearch`)
1. `bBusy` 중이면 무시. 이전에 남은 방이 있으면 `CloseRoom()`.
2. `FindLobbies` (최대 50개). 방 코드 입장은 `RoomCode == 입력값` 필터(입력은 공백 제거 + 대문자), 빠른 입장은 필터 없음.
3. 후보: 자리가 남았고 내가 연 방이 아닌 것. 없으면 "해당 방 코드를 찾을 수 없습니다." / "참가할 수 있는 방이 없습니다." 안내. 여러 개면 무작위 선택.
4. `JoinLobby` 성공 시 방장의 `ProductUserId`로 주소 `[EOS:<id>]:0`을 만들고, 이름을 접속 옵션 `?Name=...`으로 붙여 `ClientTravel(TRAVEL_Absolute)`.
5. `UCTGameInstance::QuickJoin/JoinByRoomCode`는 `IsAvailable()`(로그인 완료 또는 진행 중)이 아니면 안내만 하고 돌려보낸다.

**방 닫기** (`CloseRoom`): `LeaveLobby` 호출. `LeaveToMainMenu`, `QuitToDesktop`, 새 검색 시작 시 호출된다. 방장이 나가면 그 방이 검색되지 않는다 (`CTEosLeaveTest.ps1`로 확인).

**NetDriverEOS** (`DefaultEngine.ini [/Script/Engine.GameEngine]`)
```
+NetDriverDefinitions=(DefName="GameNetDriver", DriverClassName="/Script/SocketSubsystemEOS.NetDriverEOS", DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")
```
EOS에 로그인하지 않았거나 IP 주소(`127.0.0.1` 등)로 접속하면 `IpNetDriver`로 넘어간다. 그래서 EOS 없이 같은 PC에서 하는 시험(`CTTeamSwapTest.ps1` 등)이 가능하다. `[OnlineSubsystem] DefaultPlatformService=Null`이라 스팀은 이 브랜치에서 쓰지 않는다.

**음성용 세션** (`EnsureVoiceSession`)
- 엔진 기본 음성은 플레이어가 "게임 세션"에 등록되어야 소리를 주고받는다. 방 만들기/접속은 EOS가 하므로, 로그인 직후 기본 온라인 서비스(Null)에 **광고하지 않는 세션**을 하나 연다 (`NAME_GameSession`, 공개 연결 16, `bShouldAdvertise=false`, `bIsLANMatch=true`, presence/초대/전용서버 모두 끔). 로그: `[CT-Voice] 음성용 게임 세션 열림`.

**알려진 한계 (`EOS_Setup.md` 6~7장)**: 서로 다른 네트워크에서 중계가 실제로 되는지, 비정상 종료 뒤 남은 방이 사라지는 시간, Shipping 빌드의 EOS 로그인 육안 확인은 검증되지 않았다. 스팀 업적 등은 별도 설계가 필요하다.

---

## 3. 로비

**목적**: 모일 때까지 대기하고, 방장이 맵과 경기 설정을 정해 게임을 시작한다. 담당: `ACTLobbyGameMode`(서버 로직), `ACTLobbyPlayerController`(요청 RPC, 카메라, 로딩 화면), `UCTLobbyWidget`(화면).

**입장과 초기화**
- `PostLogin`과 `HandleSeamlessTravelPlayer` 모두 `InitLobbyPlayer`로 모은다: 방장 표시(`IsLocalController()`), 준비 해제, 색 초기화, 유령/기권 해제, 꼬리 수 `LobbyTailCount=1`. 심리스 복귀 시 이전 게임의 AI PlayerState는 제거.
- 방장은 준비가 필요 없다. 시작 가능 여부는 `ACTGameState::CanStartLobby()`: 방장 외 사람이 모두 준비, 선택된 모드가 시작 가능(`IsModePlayable`), 보물찾기는 AI가 없으면 양 팀에 한 명 이상, 참가자(사람 + 설정한 AI) 수가 `max(LobbyMinPlayers, 2)` 이상.

**시작 절차** (`TryStartGame`): 방장만, 로비 단계, 타이머 없음, `CanStartLobby()`일 때 → `SetStartCountdown(서버시각+3초)` 복제 + 타이머로 `TravelToGameMap()`(`ServerTravel`). 카운트 중에는 준비·맵·설정·팀 변경이 불가하고 신규 입장은 거절된다. 인원이 빠져 조건이 깨지면(`Logout` 0.3초 뒤 `RecheckStartCondition`) 시작이 취소된다. 컨트롤러는 시작 버튼 직후 1.5초(`StartPendingSeconds`) 동안 연타를 막고, 카운트가 끝나는 순간(마지막 남은 시간 ≤ 0.6초) 로딩 위젯(`WBP_CTLoadingWidget`, ZOrder 100)을 띄운다.

**팀 배정 (보물찾기 모드, `IsTeamMode()`)**
- 새로 들어온 사람은 사람이 적은 팀(같으면 0번 팀)에 배정된다 (`InitLobbyPlayer`). 팀 색은 `PrepareTeamColors`가 8색에서 서로 다른 2색을 무작위로 골라 `GameInstance`에 저장하므로, 로비를 다시 거치거나 경기가 시작되어도 같은 색을 쓴다.
- 팀 정원 4명(`DefaultTeamCapacity`, 개발 빌드는 `ct.TeamCapacity`로 1~4 조정). 꼬리잡기 모드에서는 일반 플레이어 목록만 사용.
- AI는 로비에서 PlayerState 없이 숫자로만 표시되며, UI가 "사람이 적은 팀부터" 채워질 팀을 미리 보여 준다.

**팀 교환 요청** (`RequestTeamChange` / `AcceptTeamSwap` / `UpdateTeamRequests`)
1. 요청자가 준비 전이고 시작 확정 전이며 팀이 0/1이어야 한다. 반대 팀에 자리가 있으면 즉시 이동.
2. 가득 찼으면 `FCTTeamSwapRequest{Requester, ExpireServerTime = 서버시각 + 5초}`를 `GameState.TeamSwapRequests`에 추가(복제). 이미 요청 중이거나 마지막 요청 후 5초(`TeamSwapRequestSeconds`)가 안 지났으면 무시.
3. 수락: 요청자가 가려는 팀(= 요청자와 반대 팀)에 있는 준비 안 한 사람만 가능 → 서로 팀을 맞바꾸고 요청 제거.
4. 0.2초마다 서버가 정리: 모드 변경, 시작 카운트, 요청자 이탈/준비, 5초 만료 시 삭제. 그 사이 자리가 생기면 요청자를 바로 이동시킨다.
- 클라이언트 표시: `UCTLobbyWidget::RefreshSwapRequests`가 요청 목록이 바뀔 때만 `UCTTeamSwapRequestWidget`을 오래된 순서로 다시 쌓는다 (요청마다 한 장). 각 패널은 `NativeTick`에서 남은 시간을 `Progress_Circle`(`UCTCircularProgress`)에 `Remaining/5`로 넣고, 수락 버튼은 수락 자격이 있을 때만 활성화한다. 요청자 본인의 "팀 변경" 버튼은 요청 중 비활성이고, 반대 팀 사람이 4명 이상이면 글자가 "팀 변경 신청"으로 바뀐다.
- 주의: 위 UI 쪽 "가득 참" 판정과 패널의 `/5.f`는 숫자가 코드에 직접 들어 있어, 서버 상수(`DefaultTeamCapacity`, `TeamSwapRequestSeconds`)나 `ct.TeamCapacity`를 바꾸면 글자와 진행 표시가 서버와 어긋날 수 있다.

**경기 설정 동기화** (`FCTMatchSettings`, `CTMatchSettings.*`)
- 방장이 `<` `>` 버튼으로 항목을 한 칸씩 이동(`ChangeMatchSetting`). 서버가 `Step`으로 선택지를 옮기고, `IsAllowed(현재 인원)`(`MaxPlayers >= 현재 인원` 그리고 `RoundSeconds <= GameMinutes*60`)를 만족할 때만 `GameState.SetMatchSettings`(복제) + `GameInstance`에 저장한다. 모든 값은 정해진 선택지 중에서만 고르며, `Sanitize`가 가장 가까운 선택지로 보정한다.

| 항목 | 선택지 | 기본값 |
|---|---|---|
| GameMinutes (경기 시간, 분) | 5, 6, 7, 8, 9, 10 | 5 |
| RoundSeconds (라운드, 초) | 30, 45, 60, 90, 120 | 60 |
| MaxPlayers (최대 인원) | 3~8 | 8 |
| InitialTails (시작 꼬리) | 1, 2, 3 | 1 |
| PhoneUses (전화 쿨타임, 초, 0 = 사용 안 함. 이름은 로비 위젯 이름 호환 때문에 유지) | 0, 15, 30, 45, 60 | 30 |
| Bush (부쉬 기믹) | 0, 1 | 1 |
| AiPlayers (AI 수) | 0~7 | 0 |
| Mode (게임 모드) | 숨김 동안 0만 | 0 (꼬리잡기) |

- 맵 선택은 `SelectMap`(방장만, 목록에 있는 활성 맵만). 처음에는 `UCTMapSettings::GetFirstEnabledIndex()`.

**모드 선택 숨김**
- 게임 모드는 꼬리잡기(0)/보물찾기(1)이나 보물찾기는 개발 중이어서 로비의 모드 줄을 숨긴다: `FCTMatchSettings::IsModeSelectVisible()` (Shipping은 항상 false, 그 외는 `ct.ShowGameModeSelect`). `UCTLobbyWidget::NativeConstruct`가 위젯 이름 `Row_Setting_Mode`를 Collapsed로 만들고(세로 상자라 아래 줄이 올라옴), `GetOptions(Mode)`도 숨김 동안 `{0}`만 돌려줘서 저장된 다른 값도 `Sanitize`에서 0으로 돌아온다.
- `IsModePlayable()`: 꼬리잡기는 항상 true. 보물찾기는 Shipping에서 false, 개발 빌드에서는 `ct.EnableTreasureHunt`(기본 1)로 결정.
- 개발용 콘솔 `CTLobbyPreview`(방장만): 로비를 보물찾기 + AI 3명으로 바꿔 팀 화면을 확인.

**화면 갱신**: `UCTLobbyWidget`은 0.2초 타이머로 `RefreshLobby()`를 호출한다. 목록 줄 수가 달라질 때만 위젯을 새로 만들고, 아니면 내용만 갱신한다(깜빡임 방지). 방 코드 보기/숨기기/복사 버튼, 채팅 로그(최대 50줄 `MaxChatLines`, Enter로 입력칸 포커스), 맵 선택 창, 전적 팝업(`WBP_CTMatchHistoryWidget`, ZOrder 200)을 포함한다. 로비는 ESC 메뉴 대신 자체 나가기 버튼을 쓴다 (`CanOpenPauseMenu() = false`).

**로비 카메라**: 로컬 전용 `ACameraActor`를 만들어 고정 각도(Pitch -50°, Yaw 0°, FOV 60°)로 캐릭터에서 750cm 떨어져 따라간다 (추적 속도 8, `VInterpTo`). 폰을 받아 회전이 바뀌면 `ClientRestart`에서 다시 적용한다.

---

## 4. 음성 채팅

**목적**: 근접 음성(거리에 따라 작아짐)과 보물찾기 팀 음성(거리 무관, 팀원만 들음)을 지원한다.

**구성 (엔진 기본 음성 + 게임 코드)**
- 설정: `[Voice] bEnabled=true`, `[OnlineSubsystem] bHasVoiceEnabled=true`, `bRequiresPushToTalk=True`, 음성 사운드 클래스 `/Game/sound/SC_CT_Voice` (`AudioSettings.VoiPSoundClass`).
- 입력: `IA_PushToTalk`(근접, 마이크 표시에 쓰는 기본 글자 V)와 `CTCodeKeys::TeamVoice`(팀 음성, 기본 T, 설정 창에서 변경 가능).

**동작 흐름**
1. 보내기 (`ACTCharacter::UpdateVoiceTransmit`, 로컬 매 틱 + 키 이벤트): 설정에서 마이크를 켠 경우에만 전송한다. 전송 조건은 `상시 말하기 || 근접 키 || 팀 키`. 보물찾기 모드이고 팀 키를 누르는 중(또는 상시 말하기이면서 근접 키를 안 누른 때)이면 "팀 음성"이다. 팀 음성 상태가 바뀌면 `ServerSetTeamVoice` → `ACTPlayerState::bTeamVoice`(전체 복제). 실제 송출은 `PlayerController->ToggleSpeaking`.
2. 받기 (`ACTCharacter::UpdateVoiceTalkerMode`, 원격 캐릭터마다): 말하는 사람의 `IsTeamVoice()`와 듣는 사람의 팀을 비교해 3상태 중 하나로 설정한다.
   - 0 근접: 캐릭터 루트에 붙여 거리 감쇠(`VoiceAttenuation`) 적용
   - 1 팀원으로서 팀 음성 듣기: 붙일 컴포넌트와 감쇠를 비워 위치와 무관하게 똑바로(2D) 재생 (다음에 말을 시작할 때부터 적용)
   - 2 상대 팀의 팀 음성: `UCTVoipTalker::SetMuted(true)`로 무음 (말하는 도중에도 즉시 적용)
3. `UCTVoipTalker`(`UVOIPTalker` 상속): `OnTalkingBegin`에서 오디오 컴포넌트를 잡고 `SetVolumeMultiplier(음소거 ? 0 : ct.VoicePlaybackGain)`. `ACTCharacter::EnsureVoiceTalker`가 `RegisterWithPlayerState`로 등록한다 (AI와 전용 서버는 제외).

**핵심 수치**

| 항목 | 값 | 위치 |
|---|---|---|
| 근접 최대 크기 반경 / 감쇠 거리 | 300cm / 1500cm (들리는 한계 1800cm) | `ACTCharacter::VoiceInnerRadius`, `VoiceFalloffDistance` |
| 받는 목소리 배수 | `ct.VoicePlaybackGain` 기본 4.0 (직접 들어 보고 정한 값) | `CTVoipTalker.cpp` |
| 마이크 입력 증폭 | `voice.MicInputGain` ini 2.0, 실행 중 `ApplyAudio`가 `CTMicInputGain = 3.0f`로 덮어씀 (`ECVF_SetByGameSetting`) | `DefaultEngine.ini [SystemSettings]`, `CTSettingsSaveGame.h` |
| 마이크 감도 | 슬라이더 0~1, 기본 0.9 → 임계값 `(1 - 감도) × 0.1` = 0.01 (`UVOIPStatics::SetMicThreshold`) | `UCTSettingsSubsystem::ApplyAudio` |
| 남의 목소리 볼륨 | 슬라이더 0~1, 기본 0.5, 실제 `값 × CTVoiceMaxGain(2.0)` → 기본 1.0. 전체 음소거면 0 | 사운드 믹스 오버라이드 |

**말하는 표시**: `UCTMicIndicatorWidget`(패널을 코드로 만든 위젯, 로비에서는 오른쪽 아래 ZOrder 5)과 HUD의 `UpdateMicIndicator`가 **내** 송출 상태(`ACTCharacter::IsVoiceTransmitting()`)를 표시한다. 평소 흰 패널 + 마젠타(#FF2783) 아이콘, 말하는 중 마젠타 패널 + 흰 파장(사인 두 개를 섞어 출렁임), 마이크 끔 회색. 아이콘 아래에 눌러서 말하기 키 이름을 보여 준다(상시 말하기면 숨김). 확인한 범위에서 다른 플레이어의 말하는 표시는 따로 만들지 않는다.

**설계 이유·주의점**
- 스팀 음성은 크기를 자동으로 맞춰 주었지만 엔진 기본 음성은 마이크 소리를 그대로 보내 작게 들려서 증폭을 넣었다. 너무 키우면 소리가 찢어진다 (`CTMicInputGain` 주석).
- 기본 `UVOIPTalker` 대신 `UCTVoipTalker`를 써서 "상대 팀의 팀 음성 무음"을 구현했다.
- ini의 `voice.MicInputGain=2.0`과 코드의 3.0이 다르다. 게임 실행 중 값은 `ApplyAudio` 호출 이후 3.0이며, `EOS_Setup.md`는 2.0으로 적혀 있다.
- 한 PC에서 두 창으로 시험하면 같은 마이크/스피커가 울릴 수 있어 헤드셋을 쓴다 (`CTEosVoiceTry.ps1` 주석).

---

## 5. 설정 · 오디오 서브시스템

### 5.1 `UCTSettingsSubsystem` (GameInstance 서브시스템)
**목적**: 사용자 설정을 로컬에 저장하고 즉시 적용한다.
- 저장: `Saved/SaveGames/CTSettings.sav` (`UCTSettingsSaveGame` → `FCTSettingsData`). 모든 `SetXxx`는 값 보정(Clamp) → 적용 → `CommitChange()`(저장 + `OnSettingsChanged` 브로드캐스트 → UI 갱신) 순서.
- 항목과 기본값: 배경음 0.7, 효과음 1.0, 전체 음소거 꺼짐, 언어(비어 있으면 처음 실행, 영어), 전체 화면 켜짐(에디터에서는 적용 안 함), 카메라 흔들림 켜짐, 색 구분 보조(이름 + 도형) 꺼짐, 머리 위 팀/색 표시 켜짐, 마이크 사용 켜짐, 상시 말하기 꺼짐, 음성 볼륨 0.5, 마이크 감도 0.9, 이모트 단축키 목록, 키 재설정 목록(`KeyOverrides`).
- 지원 언어: `ko`, `en`, `ja` (`SetLanguage(code, bConfirm)`로 미리보기/확정, `HasChosenLanguage`로 첫 언어 선택 화면 여부 판단).
- 소리 적용 (`ApplyAudio`): 마이크 감도/증폭, 배경음(`UCTAudioSubsystem::RefreshMusicVolume`), 효과음과 음성은 프로젝트 설정 `UCTSettingsConfig`의 사운드 믹스(`SettingsSoundMix`)에 `SetSoundMixClassOverride`. 월드가 없으면 건너뛰고 맵이 열릴 때 컨트롤러가 다시 호출한다.
- 키 설정: 입력 매핑 에셋의 키를 복사본(`EffectiveContexts`)으로 바꿔 쓰며, 이모트(R)/전화·유령 이동(F)/팀 음성(T)/잡기 보조(G)처럼 코드가 직접 받는 키는 `CTCodeKeys`와 `GetCodeKey`로 읽는다. 다른 행동이 쓰는 키와 겹치면 `SetKeyBinding`이 실패하고 이유를 돌려준다.
- 호환 처리: 이전 음성 볼륨 척도(`VoiceScaleVersion < 2`)는 불러올 때 기본값으로 되돌린다. 테스트용 명령줄 `CTTestMusicVolume=`, `CTTestSfxVolume=`(저장 안 함).
- 플레이어 이름은 `UCTGameInstance`가 관리한다: 2~12자, 한글·영문·숫자·`_`·`-`만 허용(접속 주소 옵션에 안전한 글자), 최초 1회 `SetPlayerName`, 이후 변경은 `ChangePlayerName`(`CanChangePlayerName()` 허용 시).

### 5.2 `UCTAudioSubsystem` (GameInstance 서브시스템)
**목적**: 배경음과 효과음 재생, UI 소리 부착.
- 사운드 에셋은 `UCTAudioSettings`(프로젝트 설정 → CT Audio)에서 지정한다. 효과음 `ECTSfx`(UI 3종, 카운트다운, 꼬리 얻음/잃음, 밀기, 점프/착지/발소리, 드럼통 도화선/폭발, 차·낙하물·통나무 피격 등), 배경음 `ECTMusic`(None/Menu/Game).
- `PlaySfx`는 2D, `PlaySfxAtLocation`은 월드 감쇠: 일반 반경 600cm + 감쇠 2500cm, 발소리는 반경 120cm + 800cm 감쇠, 볼륨 ×0.6, 피치 ×0.9, 멀수록 저역통과(최저 1800Hz). 효과음은 `SfxCache`에 캐시한다.
- `PlayMusic`: 같은 곡이면 유지, 전환 시 이전 곡 페이드아웃(1.0초) + 새 곡 페이드인, 맵 이동에도 이어지도록 `bPersistAcrossLevelTransition=true`. 곡이 끝나면 `HandleMusicFinished`가 재시작. 경기 중 배경음은 `GameMusicVolumeScale = 0.7`을 더 곱해 발소리가 묻히지 않게 한다. 설정 음소거면 0.
- 에디터 PIE 다중 창은 첫 창만 재생한다.
- `ApplyUISounds(Widget)`: 위젯 트리의 모든 `UButton`/`UCheckBox`에 호버/누름 소리를 부착한다. 버튼 이름에 `Back`, `Close`, `Cancel`, `Leave`가 들어가면 "뒤로" 소리를 쓴다. 각 UI 위젯이 `NativeConstruct`에서 호출한다.
- `[Audio] UnfocusedVolumeMultiplier=1.0`: 창이 포커스를 잃어도 소리를 유지한다 (PIE 창 전환 때 BGM이 꺼지는 것 방지).

---

## 6. UI 구조

**원칙: C++ 위젯은 로직과 이름 규칙을 제공하고, 모양(디자인)은 위젯 블루프린트(WBP)에서 정한다.** 코드로 디자인을 덮어쓰지 않고, 디자인이 필요하면 BP에 붙여넣을 수 있는 코드로 제공한다 (프로젝트 메모리의 합의). 예외: `UCTMicIndicatorWidget`은 패널을 코드로 만든다.

**계층**
- `UCTUserWidget` (Abstract): 공용 도우미. `GetCTGameState`, `GetModeType`, `GetOwningCTPlayerState`, `GetOwningCTCharacter`, 색 아이콘(`ColorIcons`)과 `ApplyColorToWidgets`, `FormatTime`(올림해 m:ss), `GetSortedPlayers`, `GetPlayerScore`(꼬리잡기는 꼬리 수, 보물찾기는 `ResultEnergy`).
- `UCTHUDWidget` (Abstract, `UCTUserWidget` 상속): 게임 중 HUD. `NativeTick`에서 `TryBindGameState/TryBindMyPlayerState`를 매 프레임 재시도해(접속 직후에는 GameState/PlayerState가 없을 수 있음) 델리게이트에 연결하고, 시간 표시, 스킬 슬롯 쿨타임(머티리얼), 점수판(0.25초 간격 동기화, `ScoreboardSyncInterval`), 방향 화살표(`UCTDirectionArrow`), 마이크 표시, 상단 안내 문구(`ShowNotice`)를 갱신한다. 연출은 `BlueprintImplementableEvent`(`BP_OnPhaseChanged`, `BP_OnRoundChanged`, `BP_OnCountdownTick`, `BP_OnMyTailCountChanged` 등)로 BP에 맡긴다.
- `UCTLobbyWidget`, `UCTTeamSwapRequestWidget` (Abstract): 3장 참조. 로비는 연출용 `BP_OnLobbyRefreshed` 이벤트를 제공한다.

**이름 규칙 (`BindWidgetOptional`)**
- 코드는 WBP 안의 위젯을 **이름과 종류가 모두 같을 때만** 연결한다. 이름이 다르면 그 요소는 조용히 동작하지 않고 에러도 나지 않는다 (`docs/UI_NamingGuide.md`). 대소문자가 같아야 하고 WBP에서 "변수인가" 체크가 필요하다. 전부 선택 사항이라 위젯이 없어도 크래시하지 않는다.
- 접두사: `Text_`(글자), `Button_`(버튼), `Image_`(이미지), `Scroll_`/`ScrollBox_`(스크롤), `Panel_`(숨김 단위 묶음), `Progress_`, `Slider_`, `CheckBox_`, `EditableTextBox_`, `Row_`, `Box_`, `Size_`, `Widget_`. 예: `Text_RequesterName`, `Button_Accept`, `Text_AcceptLabel`, `Progress_Circle` (`CTTeamSwapRequestWidget`), `Text_RoomCode`, `Button_Start` (`CTLobbyWidget`).
- 경기 설정 `< >` 줄은 설정 키 이름(`FCTMatchSettings::GetKey`)으로 이름을 조합한다 (`BindMatchSettingWidgets`): `Label_Setting_<Key>`, `Text_Setting_<Key>`, `Button_Setting_<Key>_Prev`, `Button_Setting_<Key>_Next`, 줄 전체 `Row_Setting_<Key>`(모드 줄 숨김용). 키는 `GameMinutes`, `RoundSeconds`, `MaxPlayers`, `InitialTails`, `PhoneUses`, `Bush`, `AiPlayers`, `Mode`.
- 기본 WBP 경로는 생성자에서 `ConstructorHelpers::FClassFinder`로 찾는다: `/Game/UI/WBP_CTLobbyPlayerEntryWidget`, `WBP_CTTeamSwapRequestWidget`, `WBP_CTMatchHistoryWidget`, `WBP_CTLoadingWidget`. `EditDefaultsOnly` 속성이라 BP에서 교체할 수 있다.
- 버튼 소리는 `UCTAudioSubsystem::ApplyUISounds`, 호버 효과는 `UCTButtonHoverEffect::Attach`.

**원형 진행 `UCTCircularProgress`** (`UWidget` 상속, Slate 위젯 `SCTCircularProgress`)
- 위(12시)에서 시계 방향으로 차오르는 고리. `OnPaint`에서 `MakeLines`로 배경 고리(Track) 전체를 먼저 그리고 그 위에 `Percent`만큼 채움(Fill)을 그린다. 호는 `ceil(구간 × 64)`개(최소 2) 선분으로 근사한다.
- 속성: `Percent`(0~1), `FillColor`(기본 마젠타 계열 `(1, 0.153, 0.514)`), `TrackColor`(흰색 55% 불투명), `Thickness`(기본 8px, 최소 1). `SetPercent`는 BP에서 호출 가능. 기본 크기 64×64, 생성 시 `HitTestInvisible`.
- 사용처: 팀 교환 요청 패널의 5초 카운트다운 (`Remaining / 5`). 디자이너는 WBP 팔레트에서 배치하고 크기/색을 BP에서 정한다.

**로컬라이징**: 화면 문구는 `NSLOCTEXT("CT", ...)`. 한/영/일 번역표는 `Config/Localization/Game_Translations.tsv`, 갱신은 `Tools/UpdateLocalization.bat`.

---

## 7. 환경 기믹 (`Source/CatchTheTail/World`)

공통 원칙: 피격 판정과 효과(꼬리 잃기, 날려 보내기)는 **서버에서만** 판단하고, 맞은 사람에게는 재적중 대기시간을 둔다. 꼬리 손실 설정(`bHitDropsTail`, `HitTailLoss=1`, `DroppedTailLaunchSpeed=450`, `DroppedTailUpSpeed=700`, `DroppedTailPickupDelay=1.2`초, `DroppedTailLifeSeconds=25`초)은 낙하물, 드럼통, 통나무, 차량이 같은 기본값을 쓰며, 잃은 꼬리는 아이템(`ACTTailPickup`)이 되어 누구든 주우면 되찾는다. 수치는 모두 `EditAnywhere` 속성이라 레벨에서 조정한다.

| 기믹 | 동작 | 복제 방식 | 주요 수치 (기본값) |
|---|---|---|---|
| **낙하 스폰 볼륨** `ACTFallingSpawnVolume` (+ `ACTFallingProp`) | 박스 영역 위에서 무작위 액터/메시를 일정 간격으로 떨어뜨림. 바닥 위치에 그림자 데칼(높이에 따라 진해짐) 표시. 받침이 없으면 정착(Settle) 처리. 맞으면 밀려나고 꼬리를 잃음 | 볼륨이 낙하 목록 `Entries`를 복제(`OnRep_Entries`), `NetUpdateFrequency 10`. 낙하 물체는 `bReplicates` | 스폰 간격 1.5~3.5초, 동시 최대 8개, 낙하 속도 200~350, 착지 후 8초 유지, 피격 넉백 거리 300 / 상승 400 / 최대 1200, 재적중 대기 0.6초 |
| **폭발 드럼통** `ACTExplosiveBarrel` | 플레이어가 닿으면 도화선(점점 빨라지는 깜빡임) → 폭발. 반경 내 캐릭터를 바깥쪽으로 날리고(가운데일수록 강함) 꼬리를 잃게 함. 일정 시간 뒤 다시 나타남. 경기 중(`Playing`)에만 작동. AI가 피할 수 있게 상태를 읽을 수 있음 | `State`(Idle/Burning/Exploded) 복제 `OnRep_State`, 폭발 시각 복제 | 도화선 3초, 재등장 15초(0이면 안 나옴), 반경 450cm, 넉백 1300 (가장자리 50%), 상승 700, 깜빡임 2→10회/초 |
| **굴러가는 통나무** `ACTRollingLog` | 시작점에서 끝점(뷰포트 마름모 점)까지 지형을 따라 굴러가고, 도착 후 쉬는 동안은 보이지 않음. 맞으면 옆으로 튕김 | 복제 안 함(`bReplicates=false`). `GameState`의 서버 시각으로 위치를 계산해 모든 PC가 같은 위치를 로컬로 그림. 적중은 서버만 | 속도 700cm/s, 대기 5초, 반지름 110, 길이 450, 재적중 1.2초, 옆 900 / 상승 500, 시작 시점 어긋남 `PhaseOffsetSeconds` |
| **도로 차량** `ACTRoadVehicle` | 스플라인(닫힌 고리) 경로를 따라 계속 달림. 맞으면 앞/옆으로 튕김. 같은 경로에 여러 대는 `PhaseOffsetSeconds`로 간격을 줌. 기본은 경기 외에도 달리고(`bDriveOutsideMatch`) 끄면 경기 중에만 | 복제 안 함, 서버 시각 기반 로컬 이동, 적중은 서버만 | 속도 900cm/s, 재적중 1.5초, 앞 700 / 옆 600 / 상승 550, 적중 상자 중심 높이 100 |
| **전화 부스** `ACTPhoneBooth` | 부스 안에서 전화 키(F)를 누르면 2초 통화(이동 불가) 후 가장 가까운 헌터와의 거리를 10m 단위로 안내하고 방향 화살표를 표시. 쿨타임은 경기 설정 `PhoneUses`(초, 0이면 사용 안 함, 설정을 못 읽으면 30초). 쿨타임 종료 시각은 `PlayerState.PhoneReadyServerTime`으로 복제 | `bReplicates`, 서버가 `ServerRequestCall`에서 검증(게임 중, 유령 아님, 색 배정됨) | 통화 2초, 거리 단위 10m, 안내/화살표 5초, 진입 안내 "F: 전화" 3초 |
| **부쉬** `ACTBush` | 안에 들어가면 다른 사람 화면에서 몸이 가려짐. 3초 이상 숨으면 풀이 크게 흔들려 들킴(`bAlerted`). 경기 설정 `Bush`가 꺼져 있으면 무동작 | 서버가 0.1초 간격으로 점유자 갱신, `bAlerted` 복제(`OnRep_Alerted`) | 들킴 3초, 평소 흔들림 0.5회/s 1.5°, 들킴 4회/s 6° |

주의점
- 통나무와 차량처럼 **서버 시각 + 경로로 위치를 계산**하면 위치를 네트워크로 보내지 않아 트래픽이 없고 모두 비슷하게 보이지만, 시각 동기(`GetServerWorldTimeSeconds`)에 의존한다.
- AI가 피하도록 통나무, 차량, 드럼통은 현재 위치와 경로/상태를 읽는 함수를 공개한다 (각 헤더의 "AI가 피하기 위해 읽는 값").

---

## 8. 자동 시험 도구

스크립트는 모두 **에디터를 끄고** 실행한다 (`UnrealEditor` 프로세스가 있으면 종료 코드 1). 엔진의 `UnrealEditor.exe`를 `-game -windowed -nosound -NOSTEAM -log`로 직접 띄워 방장/참가자 2개 인스턴스를 만들고, 로그의 `[CT]`, `[CT-Test]`, `[CT-EOS]` 문구로 판정한다. 결과 로그는 `docs/baseline/`에 저장된다. 사용법: `powershell -File Tools\<이름>.ps1`.

| 스크립트 | 목적 / 확인하는 것 |
|---|---|
| `CTBaseline.ps1` | 꼬리잡기 회귀 기준선. AI 3명, 경기 45초, 라운드 18초 같은 짧은 설정으로 경기 맵을 직접 실행해 `[CT]` 흐름 로그(`<Name>.events.txt`)를 모음. `-CompareTo`로 이전 기준선과 비교, `-Mode 1`(보물찾기), `-Packaged`(패키징된 exe) |
| `CTBuildAndBaseline.ps1` | 에디터 빌드(`Build.bat`, Development) 성공 시에만 기준선과 비교 (실패 시 종료 코드 4) |
| `CTTreasureCheck.ps1` | 보물찾기 한 판(AI 3 + 사람 1)을 `CTBaseline.ps1 -Mode 1`로 돌려 팀 배정, 에너지 100개 배치(라운드마다), 무작위 스폰, 라운드 정산 3회, 종료, 던지기 오브젝트 20개 배치 등 로그 횟수를 검사 |
| `CTTeamSwapTest.ps1` | 로비 팀 교환 요청/수락. IP(127.0.0.1)로 접속하고 `ct.ShowGameModeSelect 1, ct.TeamCapacity 1, ct.TeamSwapTest 1, CTLobbyPreview`로 정원을 1명으로 줄여, 방장 요청 → 클라이언트 수락 → 팀이 맞바뀜을 확인 |
| `CTThrowSyncTest.ps1` | 보물찾기 던지기 동기화. 방장이 오브젝트를 집어 클라이언트에게 던질 때 서버/클라이언트 로그를 확인 (`-CTMode=1`, `ct.ThrowSyncTest=1`) |
| `CTEosLobbyTest.ps1` | EOS 로그인 → 방 만들기 → 방 코드로 찾기 → 들어가기 (`ct.EosHost`/`ct.EosJoin`) |
| `CTEosGameTest.ps1` | 위 + 방장 게임 서버로 실제 P2P 접속 (`ct.EosGameHost`/`ct.EosGameJoin`) |
| `CTEosFlowTest.ps1` | 실제 게임 코드(`UCTSessionSubsystem`)로 방 열기와 입장, 로비 맵에서 만남 (`ct.AutoHostAfterLogin`, `ct.AutoJoinCode`) |
| `CTEosStartTest.ps1` | + 로비에서 준비, AI 추가, 시작, 게임 맵으로 함께 이동 (`ct.AutoLobbyStart`) |
| `CTEosMatchTest.ps1` | + 짧게 줄인 한 판 완주(`ct.TestMatchSeconds`), 로비 복귀, 다시 시작 |
| `CTEosLeaveTest.ps1` | 방장이 8초 뒤 나가고(`ct.AutoLeaveSeconds=8`), 나간 뒤 12초 후 같은 코드로 검색해 방이 남아 있지 않음을 확인 |
| `CTEosVoiceTry.ps1` | 음성 직접 확인용 두 창 실행기 (마이크 필요, 자동 판정 없음, 창은 직접 닫음) |
| `PackageGame.bat` | Windows 패키징 (`BuildCookRun`, 기본 Development, `Shipping` 인자로 출시 빌드, 출력 `Packaged\Windows`) |
| `UpdateLocalization.bat`, `ApplyTranslations.pl` | 번역 문구 수집 → `Game_Translations.tsv` 반영 → 컴파일 |

- 같은 컴퓨터에서 두 사람 역할을 하면 기기 ID가 하나뿐이라, 한쪽에 `ct.EosFreshDeviceId=1`을 주어 기기 ID를 지우고 새로 만든다.
- CVar는 `-ini:Engine:[SystemSettings]:<이름>=<값>` 형태로 명령줄에 전달하거나, `-ExecCmds="..."`로 콘솔 명령을 실행한다.

**개발용 콘솔 변수와 명령** (`UE_BUILD_SHIPPING`에서는 제외되거나 무시되는 것, 표시된 `ct.VoicePlaybackGain`만 예외)

| 이름 | 기본값 | 용도 |
|---|---|---|
| `ct.EosFreshDeviceId` | 0 | 1이면 시작 시 EOS 기기 ID 삭제 후 재생성 |
| `ct.AutoHostAfterLogin` | 0 | 1이면 EOS 로그인 뒤 바로 방 만들기 (`HostGame`) |
| `ct.AutoJoinCode` | (빈 값) | 값이 있으면 로그인 뒤 그 방 코드로 입장 |
| `ct.AutoLeaveSeconds` | 0 | 0보다 크면 방을 연 지 이 초 뒤 방장이 메인 메뉴로 나감 |
| `ct.AutoLobbyStart` | 0 | 참가자는 4초 후 자동 준비, 방장은 6초 후 AI 추가 → 시작 가능하면 게임 시작 |
| `ct.TeamSwapTest` | 0 | 방장은 15초 뒤 팀 변경 요청, 클라이언트는 요청을 자동 수락 |
| `ct.TeamCapacity` | 4 | 로비 한 팀 최대 인원 (1~4) |
| `ct.ShowGameModeSelect` | 0 | 1이면 로비에 게임 모드 선택 줄 표시 |
| `ct.EnableTreasureHunt` | 1 | 개발 빌드에서 보물찾기 시작 허용 (Shipping은 항상 불가) |
| `ct.TestMatchSeconds` | 0 | 0보다 크면 경기 길이를 이 초로 줄임 |
| `ct.ThrowSyncTest` | 0 | 던지기 동기화 시험 모드 (시작 대기를 30초 이상으로 늘림) |
| `ct.VoicePlaybackGain` | 4.0 | 받는 목소리 크기 배수. 개발용이 아니라 출시 빌드에서도 유효 |
| `ct.EosLogin`, `ct.EosHost <코드>`, `ct.EosJoin <코드>`, `ct.EosGameHost <코드>`, `ct.EosGameJoin <코드>` | 명령 | EOS 로그인/방/접속 단독 시험 (`CTEosTest.cpp`, `!UE_BUILD_SHIPPING`) |
| `CTHost`, `CTJoin <코드>` | Exec | 콘솔로 방 만들기/코드 입장 (`UCTGameInstance`) |
| `CTResetPlayerName` | Exec | 저장된 이름 삭제 (Shipping 제외) |
| `CTLobbyPreview` | Exec | 로비를 보물찾기 + AI 3명으로 (방장만) |
| `CTPreviewTreasureHud`, `CTStoreShots <개수>` | Exec | 보물찾기 HUD 디자인 확인 / 상점 이미지용 HighResShot 연속 촬영 |
| 명령줄 `-CTMode=1`, `CTTestMusicVolume=`, `CTTestSfxVolume=` | - | 로비 없이 보물찾기 시작(개발), 음량 시험(저장 안 함) |
| `set CTGameMode DebugAIPlayerCount / GameDurationSeconds / RoundDurationSeconds` | - | `CTBaseline.ps1`이 `-ExecCmds`로 설정해 AI 수와 경기/라운드 길이를 줄임 |

**주의점**
- 기준선(`docs/baseline/*.events.txt`)과 시험 판정은 로그 문구(`[CT] ...`)를 정규식으로 센다. 로그 문구를 바꾸면 시험이 깨진다. 특히 `[CT] 방 코드: ...`는 EOS 시험 스크립트가 방장의 방 코드를 읽는 데 쓴다.
- EOS 시험은 EOS 접속 정보와 인터넷 연결이 필요하다. 서로 다른 네트워크에서의 중계는 같은 PC 2인스턴스 시험으로는 검증되지 않는다.
