# Hey Chicken! 기술 문서

UE 5.7 / C++ (접두사 `CT`) 리슨 서버 파티 게임. 코드를 읽고 확인한 내용만 정리했습니다.

| 문서 | 내용 |
|---|---|
| [01_Combat_Character.md](01_Combat_Character.md) | 잡기 판정, 밀기, 꼬리 시스템, 그래플(줄다리기), 캐릭터 상태·복제 |
| [02_AI_Rules.md](02_AI_Rules.md) | AI 구조·내비게이션, 꼬리잡기/보물찾기 AI, 규칙 모드 구조, 보물찾기 시스템, 매치 흐름 |
| [03_Network_UI_Tools.md](03_Network_UI_Tools.md) | 네트워크, EOS 연결, 로비, 음성, 설정·오디오, UI 구조, 환경 기믹, 자동 시험 도구 |

함께 볼 문서: `docs/EOS_Setup.md`, `docs/ModeArchitecture_Design.md`, `docs/Mode_TreasureHunt_Plan.md`.

## 문서 작성 중 발견한 코드·문서 불일치 (정리 후보)
- 마이크 증폭: ini는 `voice.MicInputGain=2.0`, 실행 중에는 `ApplyAudio`가 코드 상수 3.0으로 덮어씀.
- 팀 정원 4와 요청 유지 5초가 `CTLobbyWidget`·`CTTeamSwapRequestWidget`에 숫자로 박혀 있음 (`ct.TeamCapacity`와 어긋날 수 있음).
- 다른 플레이어가 말하는 표시는 코드에 없음 (내 마이크 상태만 표시).
- `CTRule_TreasureHunt.h` 상단 "아직 없음" 주석은 오래된 내용 (던지기·팀 에너지 복제 구현됨).
- 기획서는 AI가 "에너지 많은 쪽"으로 간다고 했으나 코드는 가장 가까운 에너지를 고름.
- 밀기는 피해자의 부쉬·대결 상태를 검사하지 않음 (의도 여부 주석 없음).
- 잡기 입력은 코드로 G키·마우스 왼쪽만 확인됨 (F키 매핑은 입력 에셋에 있음).

## 2026-11-18 이후 추가·변경 (보물찾기)
- 라운드 사이 쉬는 시간 단계(`RoundBreak`)와 결과 팝업, 전용 결과 화면, 팀 색 미리 입히기, 에너지 낙하/터짐, 잡기 연타 대결 적용, 던진 오브젝트 판정(닿으면 무효)은 `docs/Mode_TreasureHunt_Plan.md` 맨 아래 "구현 현황" 표에 정리되어 있음.
- 상체 몽타주: AnimBP에 `UpperBody` 슬롯과 뼈 단위 섞기(`Spine`부터)가 들어 있음. 도구는 `Source/CatchTheTail/Core/CTAnimTools.*` (에디터 전용).
- 2026-11-18: 들고 있는 상체 몽타주는 반복하지 않고 마지막 자세에서 멈춰 유지함(들 때 한 번 재생, 놓으면 해제).
- 2026-11-18: 큐브 시티(`L_Box`)에 NavMeshBoundsVolume 추가(`Tools/CTAddNavVolume.py`), AI 직선 이동 막힘 우회 추가(`02_AI_Rules.md` 참고).
