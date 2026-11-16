# EOS(Epic Online Services) 접속 설정과 시험 가이드

> `feature/eos` 브랜치 기준. 스팀 방식은 `feature/character-animation` 브랜치에 그대로 있습니다 (백업 태그: `backup-2026-11-16-steam-listen`).
> 에픽 계정 없이 **기기 ID**로 로그인해서 방을 만들고, 방 코드로 찾아 들어가고, EOS P2P로 접속합니다 (중계는 에픽 서버).

## 1. 구조 한눈에
| 일 | 담당 |
|---|---|
| 로그인 | 게임을 켜면 `UCTSessionSubsystem`이 자동으로 기기 ID로 EOS 로그인 (`OnlineServicesEOSGS`) |
| 방 만들기 / 방 코드로 찾기 / 입장 | EOS 방(로비). 방 코드는 검색 가능한 속성 `RoomCode` |
| 게임 접속 | `NetDriverEOS` (주소 모양 `[EOS:사용자ID]:0`). EOS에 로그인하지 않았거나 IP 주소면 일반 IP 방식으로 넘어감 |
| 음성 | 엔진 기본 음성(Null 서비스). 로그인 직후 광고하지 않는 "음성용 게임 세션"을 하나 열어 둠 |

## 2. 처음 설정하는 방법 (새 컴퓨터)
1. `feature/eos` 브랜치를 받습니다. `.uproject`에 EOS 플러그인 5개가 켜져 있습니다.
2. Epic 개발자 포털에서 만든 값 6개를 `Config/Windows/WindowsEngine.ini`에 넣습니다. **이 파일은 `.gitignore`에 있어 저장소에 올라가지 않습니다.**
```ini
[OnlineServices.EOS]
ProductId=...
SandboxId=...
DeploymentId=...
ClientId=...
ClientSecret=...
ClientEncryptionKey=...   ; 직접 만든 64자리 영문·숫자 (저장 기능을 쓰지 않아 포털 값이 아니어도 됨)
```
3. 샌드박스는 `Live` 하나만 쓰고, 클라이언트 정책은 `Peer2Peer`(Sessions, Lobbies, Voice 포함)입니다.
4. 비밀키가 채팅이나 저장소에 노출되면 포털에서 클라이언트 비밀키를 다시 만들고 이 파일의 값을 바꿉니다.

## 3. 방 구조 설정 (`Config/DefaultEngine.ini`, `[OnlineServices.Lobbies]`)
- 검색 가능한 속성은 기본 구조(`LobbyBase`)에만 둘 수 있어서, 기본 구조의 값 목록을 지우고 `RoomCode`를 더해 다시 등록합니다.
- 방은 기본 구조의 **자식 구조** `HeyChickenLobby`(비어 있음)로 만듭니다. 자식 구조는 카테고리를 가질 수 없습니다.

## 4. 음성
- 마이크 입력 `voice.MicInputGain=2.0`, 받는 목소리 `ct.VoicePlaybackGain=4.0` (직접 들어 보고 정한 값).
- 값을 바꿔 보려면 실행 파일 바로가기 끝에 `-ini:Engine:[SystemSettings]:ct.VoicePlaybackGain=8` 처럼 붙입니다.
- 음성이 되려면 `DefaultPlatformService=Null`이고, 로그인 직후 열리는 음성용 세션이 필요합니다 (`[CT-Voice] 음성용 게임 세션 열림` 로그).

## 5. 자동 시험 (에디터를 끄고 실행, `-NOSTEAM`으로 돌아감)
| 스크립트 | 확인하는 것 |
|---|---|
| `Tools/CTEosLobbyTest.ps1` | 로그인, 방 만들기, 방 코드로 찾기, 들어가기 |
| `Tools/CTEosGameTest.ps1` | + 방장 게임 서버로 실제 접속 |
| `Tools/CTEosFlowTest.ps1` | 실제 게임 코드(`UCTSessionSubsystem`)로 방 열기와 입장, 로비 맵에서 만남 |
| `Tools/CTEosStartTest.ps1` | + 로비에서 게임 시작, 게임 맵으로 함께 이동 |
| `Tools/CTEosMatchTest.ps1` | + 한 판 완주, 로비 복귀, 다시 시작 |
| `Tools/CTEosLeaveTest.ps1` | 방장이 나가면 그 방이 찾아지지 않음 |
| `Tools/CTEosVoiceTry.ps1` | 음성 직접 확인용 두 창 실행기 (마이크 필요) |
- 한 컴퓨터에서 두 사람 역할을 하려고 한쪽은 `ct.EosFreshDeviceId=1`로 기기 ID를 지우고 새로 만듭니다 (같은 컴퓨터는 기기 ID가 하나뿐이라서).

## 6. 확인되지 않은 것
- 서로 다른 네트워크에서 중계가 실제로 되는지 (친구와 직접 확인 필요).
- 게임이 비정상 종료된 뒤 남은 방이 사라지는 데 걸리는 시간.
- 출시용(Shipping) 빌드는 로그가 없어 EOS 로그인이 되는지 눈으로 확인하지 못함. 개발용 빌드에서는 확인됨.

## 7. 알려진 제약
- 에픽 계정 로그인(`OnlineSubsystemEOS`)은 쓰지 않습니다. 그쪽은 에픽 계정이 필요한 로그인만 지원해서 기기 ID 로그인이 안 됩니다.
- 기기 ID는 컴퓨터에 묶입니다. 다른 컴퓨터로 옮기면 다른 사용자가 됩니다 (이름은 게임에서 따로 저장).
- 스팀은 이 브랜치에서 쓰지 않습니다 (`DefaultPlatformService=Null`). 스팀 업적 등을 넣으려면 따로 설계해야 합니다.
