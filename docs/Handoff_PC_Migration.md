# 인수인계 문서 (PC 이전용, 2026-11-22)

새 PC / 새 Claude 대화에서 바로 이어 가기 위한 문서입니다. 이 프로젝트는 UE 5.7 C++ 파티 게임 **Hey Chicken!** (코드 접두사 `CT`, 프로젝트 폴더 `CatchTheTail`)입니다. 한국어로 소통합니다.

## 1. 지금 상태 한눈에

- **브랜치**: `feature/eos`에서 개발하고, 끝나면 `main`을 `feature/eos`와 같은 커밋으로 맞춥니다(빨리 감기). 마지막 커밋 `58094eb`(메인 메뉴 크레딧 화면)까지 두 브랜치가 같습니다.
- **모드**: 꼬리잡기, 보물찾기(4:4 팀전). 축구 모드는 보류(`docs/Mode_Football_Plan.md`).
- **온라인**: EOS P2P(기기 ID 로그인, 방 코드로 입장). 기본 온라인 서비스는 `Null`. 스팀 소켓은 쓰지 않음.
- **백업 이름표**: 깃 태그 `backup-2026-11-16-steam-listen` (스팀 리슨 서버 시점). 받으려면 `git fetch --tags`.
- **다음 계획**: 로블록스 이식(새 대화에서 시작). `docs/Roblox_Port_Handoff.md` 참고.

## 2. 새 PC에서 환경 만들기

1. UE **5.7**(Epic Games Launcher), Visual Studio(C++ 게임 개발 워크로드), Git, **Git LFS**를 설치합니다.
2. 프로젝트를 **`C:\Unreal\CatchTheTail`** 에 둡니다(경로를 같게 하면 대화 이어 가기와 스크립트가 그대로 동작합니다).
   ```powershell
   git clone https://github.com/MIW-hub/CatchTheTail.git C:\Unreal\CatchTheTail
   cd C:\Unreal\CatchTheTail
   git lfs pull
   ```
3. **깃에 없는 것**을 클라우드 백업에서 같은 위치에 덮어씁니다.
   - `Plugins\` (128MB)
   - `Content\Fab\Classic_British_Red_Telephone_Booth_-_3D_Model\`, `..._00\`
   - `docs\portfolio\`, `docs\store_art\`, `docs\ui_capture\`, `docs\baseline\`의 새 파일들, `docs\Mode_Football_Plan.md`, `docs\Roblox_Port_Handoff.md`
   - 이전부터 **커밋 안 한 변경**: `.gitignore`, `CatchTheTail.uproject`, `Content\Gimmick\BP_CTPhoneBooth.uasset`, `Content\Levels\TypeA.umap`, `Content\UI\WBP_CTTreasureResultWidget.uasset`, `Source\CatchTheTail\Animation\CTAnimInstance.cpp` (이 파일들은 의도한 변경인지 한 번 확인한 뒤 커밋하세요)
4. `CatchTheTail.uproject` 를 우클릭 → **Generate Visual Studio project files** → 에디터 빌드(`CatchTheTailEditor Win64 Development`).
5. Claude Code 대화와 기억은 `C:\Users\<사용자>\.claude\projects\C--Unreal-CatchTheTail\` 에 붙여 넣습니다(`memory` 폴더가 중요). `/resume` 으로 이어 갈 수 있지만 이번 대화 파일이 약 400MB라 느릴 수 있습니다. 그때는 `memory` 와 이 문서만으로 새 대화를 시작하세요.

## 3. 자주 쓰는 명령

모두 에디터를 끄고 실행합니다.

| 하는 일 | 명령 |
|---|---|
| 에디터 빌드 | `"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" CatchTheTailEditor Win64 Development -Project="C:\Unreal\CatchTheTail\CatchTheTail.uproject" -WaitMutex` |
| 출시(Shipping) 패키징 | `Tools\PackageGame.bat Shipping` (결과: `Packaged\Windows`) |
| 번역 갱신 | 번역표 `Config\Localization\Game_Translations.tsv` 에 `원문<TAB>영어<TAB>일본어` 줄 추가 → `Tools\UpdateLocalization.bat` (perl 필요: Git for Windows 의 `usr\bin\perl.exe`) |
| 위젯 편집 스크립트 | `UnrealEditor-Cmd.exe CatchTheTail.uproject -run=pythonscript -script=Tools\<이름>.py -unattended -nosplash -nop4` |

- 제출용 폴더는 `Packaged\Windows` 에서 `Manifest_*.txt`, `steam_appid.txt`, `*.pdb`, `*.log`, `Saved` 폴더와 `CatchTheTail\Binaries\Win64\HeyChicken.exe`(오래된 개발용 exe)를 뺀 복사본입니다. 이전에 이렇게 54개 파일, 약 768MB로 만들었습니다.
- 실행 파일 이름은 `HeyChicken.exe`, 게임 이름 표시는 "Hey Chicken!" 입니다.

## 4. 이번에 만든 것 (11월 20~22일)

- **중계 촬영(개발용)**: AI 8명이 실제 경기를 하고 로컬 플레이어는 관전자가 되어 카메라로 촬영합니다. 플레이 컷 편집과 공중 비행 두 종류.
  - 명령줄 `-CTBroadcast=1|2`, `-CTAI=8`, `-CTBroadcastScale=`(공중 촬영 고도 배율), `-CTBroadcastHud`, `-CTFxTest`
  - 스크립트 `Tools\CTRenderBroadcast.ps1`, 일괄 `Tools\CTRenderBroadcastAll.ps1`(3개 맵 x 2개 모드 x 2종류 = 12편, 1분씩)
  - 결과 영상은 `Saved\Broadcast\`(5.2GB, 깃에 없음). 직접 재생해서 확인하지는 못함.
- **폭발 드럼통 효과**: `ACTExplosionFx`(불덩이, 충격파, 불꽃, 연기), 머티리얼 `M_CT_ExplosionFx`. 드럼통의 `bSpawnDefaultExplosionFx` 로 끌 수 있음.
- **UI 수정**: 키 설정 버튼 글자 잘림 해결, 게임 방법 8줄(영/일 번역 포함), 뒤로가기 버튼 아래 여백.
- **크레딧 화면**: 메인 메뉴 `게임 방법` 아래에 버튼. 누르면 검은 화면이 0.6초에 걸쳐 나타나고 크레딧이 올라감. 클릭이나 키 입력으로 닫힘, 끝까지 흐르면 4초 뒤 자동 복귀.
  - 내용은 `Tools\CTBuildCredits.py` 의 `ENTRIES` 에서 고치고 다시 실행(`# 큰 제목`, `## 항목`, 빈 문자열은 간격).
  - 코드: `UCTMainMenuWidget`(`Panel_Credits`, `Panel_CreditsScroll`, `Button_Credits`), 편집 도구 `UCTUIBuildTools::BuildCreditsPanel`
- **인트로 시네마틱**(전날): `L_Cinematic` 맵과 `ACTCinematicDirector`. 흰 닭과 초록 닭이 쫓고 잡히는 장면.
- **외곽선 후처리**: `M_PP_SoftOutline`을 큐브 시티, 마을, 빌딩, 로비에 적용.

## 5. 지켜야 할 규칙 (기억 파일 요약)

- **UI 디자인은 BP로**: 코드로 디자인을 덮어쓰지 말고, 위젯 BP는 편집 스크립트로 바꾸거나 붙여넣기 코드로 제공합니다.
- **로비는 8명**(4:4)이 한 목록에 보이고 팀 머리글은 두지 않으며 줄 색으로 팀을 표시합니다.
- **에셋 출처**: 3D 에셋은 유료 구매와 에픽 에셋이라 표기 불필요, BGM은 AI 제작, 효과음은 Claude 제작. **크레딧에 AI 관련 문구는 쓰지 않습니다.** 크레딧의 사운드 항목은 제작자 이름으로 되어 있음.
- 잡기 대결은 F 연타(줄다리기). 보물찾기에서 잡으면 상대를 2초 멈추게만 하고 에너지는 뺏지 않습니다.
- 클래스 이름이나 파일 이름을 포트폴리오 같은 외부 문서에 쓰지 않습니다.

## 6. 남은 일

- 패키징한 `HeyChicken.exe` 를 **직접 실행해서 확인**: 메인 메뉴 버튼 줄(크레딧 위치), 크레딧 화면과 키 입력 닫기, 게임 방법 화면, 키 설정 버튼, 폭발 효과(실제 드럼통이 터질 때)
- 서로 다른 **PC 두 대로 방 접속 시험**(같은 PC 두 개는 EOS 기기 ID가 같아 불가)
- 촬영 영상 12편 재생 확인(빌딩 맵 공중 촬영 배율 1.5는 시험 없이 넣은 값)
- 축구 모드(보류), 로블록스 이식(새 대화)
- 대화·프로젝트 클라우드 백업과 위 3번 항목의 커밋 안 한 변경 정리

## 7. 알아 두면 좋은 것

- 에디터가 켜져 있으면 패키징, 촬영, 번역 스크립트가 실패합니다. 먼저 끄세요.
- `Saved\`, `Intermediate\`, `Binaries\` 는 새 PC에서 다시 만들어지므로 옮기지 않아도 됩니다.
- 위젯 BP를 파이썬으로 바꾼 뒤 저장이 안 되면 `Tools\...` 스크립트 마지막의 `sync_widget_guids` 단계를 확인하세요(위젯 변수 GUID 오류).
- 직접 PowerShell 에서 도구를 쓸 때는 `-unattended -nosplash -nop4` 를 붙입니다.
