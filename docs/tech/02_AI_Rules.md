# 02. AI 플레이어와 게임 규칙 (Hey Chicken!)

> 기준: `Source/CatchTheTail` 코드 (UE 5.7, 접두사 CT). 수치는 코드에서 확인한 기본값이며, UPROPERTY 항목은 블루프린트에서 바뀔 수 있습니다.
> AI와 규칙은 **서버에서만** 동작합니다. 클라이언트는 복제된 값만 봅니다.

---

## 1. AI 플레이어 구조

**목적**: 사람이 모자랄 때 로비 설정(`AiPlayers`)만큼 채워 넣는 가짜 플레이어. 사람과 같은 캐릭터(`ACTCharacter`)와 PlayerState를 쓰고, 조종만 `ACTAIController`가 한다.

**생성**: `ACTGameMode::SpawnAIPlayers()`가 `StartGame()`에서 한 번만(`bAIPlayersSpawned`) 호출된다. 개수 = `Clamp(요청 수, 0, 최대인원 - 사람 수)`, 최대인원은 `min(Settings.MaxPlayers, CTColorCount=8)`. 로비 설정이 없으면 `DebugAIPlayerCount`를 쓴다. 컨트롤러를 스폰하고 `SetIsABot(true)`, 이름 "AI n"을 붙이고 `RestartPlayer`로 폰을 세운다. 로비 복귀(`ReturnToLobby`) 전에 AI 컨트롤러/폰/PlayerState를 모두 파괴한다(로비 목록에 쌓이는 것 방지).

**동작 흐름**
- `Tick`: 매 프레임(`TickInterval=0`) 돌지만 판단은 `ThinkInterval` 간격이다. 매 프레임 하는 일은 (a) 대결/잡힘/넘어짐 몽타주 중 정지, (b) 직선 이동 입력 주입.
- 일시정지: `IsGrappling()/IsPlayingCaughtMontage()/IsPlayingKnockdownMontage()` 동안 `PausedUntil = 지금 + GrappledPauseSeconds`로 갱신, 이동 중단과 달리기 해제.
- `Think()` 우선순위: 경기 중 아님/전화부스 잠김이면 정지 → 모드가 보물찾기면 `ThinkTreasure` → 유령이면 꼬리 아이템 탐색/배회 → **위험물 회피** → 감지 후 1) 잡기 2) 꼬리 아이템 3) 도망/밀기 4) 추적 5) 무작위 밀기/배회.
- 감지 `Senses()`: 발소리(움직이는 상대가 범위 안이면 벽 너머도) 또는 시야(거리, 부채꼴, `ECC_Visibility` 시선 트레이스). 부쉬 안의 상대는 `IsAlerted()`(오래 숨어 흔들림)일 때만 보인다.

**난이도 요소**: 별도의 "난이도 등급" 변수는 코드에 없다. 아래 값이 사실상 난이도를 정하며 모두 BP에서 조절할 수 있다.

| 항목 | 기본값 | 의미 |
|---|---|---|
| ThinkInterval | 0.12 s | 판단 주기 (클수록 둔함) |
| SightRange / SightHalfAngleDegrees | 2200 cm / 100도 | 시야 거리 / 정면 기준 반각 |
| CloseSenseRadius | 300 cm | 뒤쪽이어도 보이는 근접 반경 |
| HearingRadius / HearingMinSpeed | 0(=오디오 설정 `FootstepInnerRadius+FootstepFalloffDistance`) / 120 cm/s | 발소리 범위 / 최소 속도 |
| EyeHeight | 70 cm | 시선 트레이스 높이 |
| ChaseMemorySeconds | 5 s | 놓친 먹잇감 기억 |
| FleeStartDistance / RunDistance | 1300 / 800 cm | 도망 시작 / 달리기 전환 |
| SuspectRange / SuspectClosingSpeed / SuspectSeconds | 1300 cm / 180 cm/s / 0.7 s | 추격자 의심 (낮을수록 겁 많음) |
| RandomPushChance | 0.08 | 판단당 무작위 밀기 확률 |
| RunMinHoldSeconds | 1.5 s | 달리기 최소 유지 |
| GrappledPauseSeconds | 0.6 s | 대결/잡힘 후 추가 휴식 |

**관련 파일/함수**: `Player/CTAIController.h/.cpp` (`Tick`, `Think`, `Senses`, `UpdateSuspicion`), `CTCharacter::AIGrab/AIPush/AIThrow/AISetRun`(서버 RPC의 `_Implementation`을 직접 호출), `CTGameMode::SpawnAIPlayers`.

**설계 이유와 주의점**
- AI도 PlayerState를 가진다(`bWantsPlayerState = true`). 점수판, 색 배정, 잡기 판정이 모두 PlayerState 기준이기 때문이다.
- **정보 공정성**: AI는 "내 목표 색"은 알지만 "나를 노리는 사람"은 모른다. 그래서 `UpdateSuspicion`으로 "나에게 계속 똑바로 다가오는 상대"를 의심한다(사람과 같은 정보 조건).
- 달리기 깜빡임 방지: 켠 직후 판단이 바뀌어 바로 끄면 "일찍 멈춤" 쿨타임에 걸리므로 `SetAIRun`이 `RunMinHoldSeconds` 동안 끄기를 무시한다.
- `Think`는 `GameState/PlayerState/Pawn` 중 하나라도 없으면 아무것도 하지 않는다.

---

## 2. AI 내비게이션

**목적**: NavMesh 위에서 목적지로 이동하되 위험물을 피하고, NavMesh가 없는 맵에서도 멈추지 않게 한다.

**동작 흐름**
1. `MoveToPoint/MoveToTarget`가 `MoveToLocation/MoveToActor`(길찾기 사용, 목적지를 NavMesh에 투영)를 호출한다. 결과가 `Failed`면 `bDirectMove=true`로 바꾸고 `Tick`에서 `AddMovementInput`으로 직선 이동한다(목적지 60 cm 이내에서 종료).
2. 이동 명령은 매 판단마다 내리지 않는다. `NextMoveCommandTime`(도망 0.5 s, 추적 0.25 s, 아이템/에너지 0.2 s, 위험 회피 0.3 s)이 지났거나 `GetMoveStatus()==Idle`일 때만 낸다.
3. 배회 목적지 `PickWanderPoint`: `GetRandomReachablePointInRadius`로 8회 뽑아 점수 = `-가운데까지 거리/CenterRoamRadius * WanderCenterBias + 난수 * (1-WanderCenterBias)`가 최대인 곳. 위험물 경로 근처(`IsNearHazardPath`)는 제외. 가운데에서 `CenterPullDistance`보다 멀면 가운데 구역(`CenterRoamRadius`)에서 먼저 고른다.
4. 도망 목적지 `PickFleePoint`: 10회 샘플, 점수 = `추격자와의 거리 + 400 * (반대 방향 내적) - FleeCenterWeight * 가운데까지 거리`. NavMesh가 없으면 `반대 방향 * FleeSampleRadius * 0.6` 직선.
5. 놀이 구역 중심 `GetPlayAreaCenter`: `bUseCenterOverride`면 지정값, 아니면 `ANavMeshBoundsVolume` 전체를 합친 상자의 중심, 볼륨도 없으면 `APlayerStart` 위치 평균(캐시).

| 항목 | 기본값 |
|---|---|
| WanderRadius | 2500 cm |
| FleeSampleRadius | 1800 cm |
| CenterRoamRadius / CenterPullDistance | 3000 / 3500 cm |
| WanderCenterBias / FleeCenterWeight | 0.7 / 0.5 |
| bPreferMapCenter | true |

**위험물 회피 `AvoidHazards`** (모든 행동보다 우선, 가장 급한 위험물 1개만 피함)
- 대상: `ACTRoadVehicle`, `ACTRollingLog`(굴러가는 동안만), `ACTExplosiveBarrel`, `ACTFallingProp`. 목록은 0.4초마다 갱신(`RefreshHazardList`). 속도는 이전 위치와의 차로 계산(`HazardPrevLocations`)하므로 처음 관측한 프레임은 건너뛴다.
- 차/통나무: 진행 방향 기준 `Along`(앞뒤)과 `Lateral`(옆)로 분해. `Reach = 속도 * HazardLookAheadSeconds + HazardAvoidRadius`. 이미 지나갔거나(`Along < -0.6*Radius`) 멀거나(`> Reach`) 옆으로 충분히 비껴가면(`Lateral >= Radius`) 무시. 급함 = `1 + (1-Along/Reach) + (1-Lateral/Radius)`. 이미 있는 쪽 옆으로 비키고, 정면이면 진행 방향의 오른쪽. 속도 50 cm/s 미만의 정지 물체는 `0.7*Radius` 안일 때만 떨어진다.
- 드럼통: `Burning`이면 `ExplosionRadius+150`, `Idle`이면 `TouchRadius+80` 밖으로. 급함 `1 + (Danger-거리)/Danger`.
- 낙하물: `Velocity.Z < -150`이고 머리 위 3000 cm 이내, 수평 380 cm 이내이면 옆으로 비킨다(급함 1.5).
- 회피 이동: `Halt` 후 달리기, 목적지 = `현재 위치 + 탈출 방향 * (반경 + 300)`을 NavMesh(허용 범위 300,300,400)에 투영, 허용 반경 40 cm.
- 목적지 선정 시 `IsNearHazardPath`가 도로 스플라인(차), 통나무 시작-끝 선분, 드럼통(`TouchRadius+150`)에서 `HazardPathClearance` 안을 제외한다.

| 위험물 항목 | 기본값 |
|---|---|
| HazardAvoidRadius | 420 cm |
| HazardLookAheadSeconds | 1.8 s |
| HazardPathClearance | 450 cm |

**막힘/낙하 처리**: AI 컨트롤러에는 **전용 막힘 감지와 낙하 복구 로직이 없다**(`Source` 전체에서 StuckCheck/OutOfWorld 계열 검색 결과 없음). 막힘은 (1) 이동 상태가 `Idle`이 되면 다음 판단에서 새 목적지를 뽑고, (2) 추적 중 기억 지점 150 cm 이내에 도착하면 포기하며, (3) 배회는 일정 간격으로 목적지를 다시 뽑는 구조로 자연스럽게 풀린다.

**설계 이유와 주의점**
- 위험물 목록을 0.4초마다만 모으는 것은 매 판단(0.12초)마다 `TActorIterator`로 전체를 순회하는 비용을 피하기 위해서다. 대신 새로 생긴 위험물은 최대 0.4초 늦게 인식한다.
- 유령은 위험물에 맞지 않으므로 `GoToTailPickup(bIgnoreHazards=true)`로 도로 위 아이템도 먹으러 간다.
- NavMesh가 없는 맵의 직선 이동은 벽에 막힐 수 있다(대체 수단일 뿐).

---

## 3. 꼬리잡기 AI 판단

**목적**: 내 목표 색 상대의 꼬리를 잡고, 나를 노리는 상대에게서 도망친다.

**동작 흐름** (`Think`, 위험물 회피 이후)
1. 모든 PlayerState를 순회(유령, 기권, 관전자 제외, 폰 필요)하며 `Senses`를 통과한 상대를 분류한다.
   - 목표 색(`Other->GetAssignedColor() == MyTarget`): **Prey**(가장 가까운 1명)
   - 그 외 중 `UpdateSuspicion`이 참: **Hunter**(가장 가까운 1명)
   - 나머지: **Neutral**
2. Prey를 감지하면 위치와 시각을 기억한다(`RememberedPrey`, 5초).
3. 우선순위:
   1. Prey 거리 ≤ `GrabReach = GrabDistance*0.85`(기본 170*0.85)이고 (Hunter 없음 또는 Hunter 거리 > 350)이면 정지, 바라보고 `AIGrab()`.
   2. Hunter가 없거나 `PickupAbortHunterDistance`(600)보다 멀면 `GoToTailPickup`(보이는 꼬리 아이템 최우선). 시야 2500 cm, 놓친 뒤 3초 기억, 도로 경로 위 아이템은 살아 있을 땐 포기, 350 cm 초과면 달림, 도착 허용 10 cm.
   3. Hunter 거리 ≤ `FleeStartDistance`: `PushReach = PushDistance*0.85` 이내이고 밀기 준비(`Now - LastPushTime > 쿨타임 + 0.2`)면 정지, 바라보고 `AIPush()`. 아니면 `RunDistance` 이내일 때 달리며 `PickFleePoint`로 이동.
   4. Prey가 있거나 기억 중이면 추적: 기억 지점 150 cm 이내인데 없으면 포기. 달리기 조건은 `거리 ≤ RunDistance*2 && 거리 > 350`. 직접 보이면 `MoveToTarget(허용 GrabReach*0.6)`, 기억이면 `MoveToPoint(…, 100)`.
   5. 그 외: Neutral이 `PushReach` 안, 정면 60도 안, 밀기 준비 상태이면 `RandomPushChance`(8%)로 밀기. 아니면 배회(0.5~2.5초 간격으로 재선택).

**추격자 의심 `UpdateSuspicion`**: 상대 속도 > 80 cm/s, `SuspectRange`(1300) 안, 나를 향한 접근 속도 ≥ `SuspectClosingSpeed`(180), 정렬도(접근 속도/속력) ≥ 0.8이면 점수 += dt(거리 400 미만이면 2배). 아니면 dt*0.6씩 감소. 점수 ≥ `SuspectSeconds`(0.7)이면 추격자로 본다. 5초 넘게 안 보이면 기록을 지운다.

**관련 파일/함수**: `CTAIController::Think/GoToTailPickup/UpdateSuspicion/PickFleePoint`, `Core/CTRule_TailChase.cpp::HandleGrab`.

**설계 이유와 주의점**
- 잡기 판정은 서버 규칙(`UCTRule_TailChase::HandleGrab`)이 한다. AI는 "잡기 입력"만 낸다. 대결(F 연타)이 켜져 있으면 `UCTGrappleComponent::BeginGrapple`로 진입한다.
- 유령 AI는 꼬리 아이템이 보이면 먹으러 가고(먹으면 부활), 없으면 0.3~1.5초 간격으로 배회한다.

---

## 4. 보물찾기 AI (`ThinkTreasure`)

**목적**: 에너지를 모으고, 오브젝트를 집어 상대 팀에게 던진다. 목표 색, 추격자 개념은 쓰지 않는다.

**동작 흐름** (위험물 회피 우선. 에너지와 오브젝트 목록은 0.5초마다 갱신)
1. **오브젝트를 들고 있을 때** (`HoldStartTime` 기록)
   - 후보: 다른 팀, 기권/관전 아님, 폰 있음, 수평 거리 < 2500, 그리고 (들고 3초 넘게 경과 **또는** `SeesLocation`으로 시야 확인) 그리고 `FriendlyInLine` 거짓.
   - **아군 오사 회피 `FriendlyInLine`**: 나→대상 선분에서 같은 팀 팀원(나 제외)까지의 최단 수평 거리가 150 cm 미만이면 던지지 않는다.
   - **꼬리 길이 기반 타겟**: 점수 = `거리 - 120 * 상대 TailCount`가 최소인 상대. 상대의 에너지는 알 수 없으므로(개인 에너지는 `COND_OwnerOnly` 복제) 사람과 같은 정보인 꼬리 길이로 짐작한다.
   - 대상이 있고 들고 0.7초가 지났으면 정지, 달리기 해제, 대상 쪽으로 몸을 돌린 뒤 `NextThrowTime`(1.0초 간격)이 되면 `AIThrow()`. 대상이 없으면 계속 들고 다닌다.
2. **오브젝트가 없을 때 근접 잡기**: `NextThrowTime` 경과, 내가 부스 잠김 아님이면 다른 팀이면서 부스 잠김이 아닌 상대가 `GrabDistance*1.05` 이내일 때 `NextThrowTime += 1.5`, 정지 후 바라보고 `AIGrab()`. 서버 규칙이 2초 멈춤을 걸고 쿨타임은 서버가 검사한다.
3. **이동 목표**: 수평 거리가 가장 가까운 에너지. 단, 오브젝트가 없고 `CanBePickedUp()`인 오브젝트가 900 cm 이내이면서 가장 가까운 에너지보다 가까우면 그쪽으로 간다. 450 cm 미만이면 정지, 바라보고 `AIGrab()`(집기), 아니면 걸어서 접근(허용 250). 에너지는 350 cm 초과면 달리고 허용 반경 10 cm.
4. 에너지가 없으면 배회(0.3~1.5초).

| 항목 | 값 |
|---|---|
| 던질 후보 거리 | < 2500 cm |
| 시야 확인 면제 | 들고 3 s 후 |
| 던지기 전 최소 보유 시간 | 0.7 s |
| 던지기 간격 | 1.0 s |
| 근접 잡기 후 재시도 | 1.5 s |
| 아군 오사 판정 폭 | 150 cm |
| 꼬리 가중치 | 120 cm / 꼬리 1개 |
| 오브젝트 탐색 반경 / 집기 거리 | 900 / 450 cm |

**관련 파일/함수**: `CTAIController::ThinkTreasure`, `SeesLocation`, `CTCharacter::AIThrow/AIGrab`.

**설계 이유와 주의점**
- 아군 피해가 있는 규칙(아군이 맞아도 에너지 손실)이라 사선 확인이 필요하다. 다만 확인은 선분 기준 수평 거리라 높이는 보지 않는다.
- 던지는 방향은 캐릭터 정면(서버 규칙)이라 던지기 전에 `FaceLocation`으로 대상을 향해 돈다. 수평 방향만 맞추며 거리별 포물선 보정은 없다(`ThrowSpeed` 고정).
- 위험물 회피가 먼저라 도로 근처에서는 에너지 줍기가 중단될 수 있다.
- 기획서(`Mode_TreasureHunt_Plan.md`)의 "에너지가 많은 쪽으로 이동"과 달리 코드는 **가장 가까운 에너지**를 고른다.

---

## 5. 게임 규칙 모드 아키텍처

**목적**: 접속, 카운트다운, 시간, AI 생성, 로비 복귀 같은 **공통 흐름**은 `ACTGameMode`가, 모드마다 다른 **규칙**은 `UCTGameRule` 파생 객체가 맡도록 분리한다.

**구조**: `UCTGameRule`(Abstract UObject, 서버 전용, 비복제)이 가상 함수를 정의한다.

| 함수 | 역할 | 기본 |
|---|---|---|
| `GetModeType` | 모드 식별 | 순수 가상 |
| `OnMatchStart` | 경기 시작 초기화 | 빈 구현 |
| `OnRoundStart(n)` | 라운드 시작(시간 설정 전) | 빈 구현 |
| `ComputeResult` | 승자/동점 계산 | 빈 구현 |
| `HandleGrab` / `FinalizeGrab` | 잡기 시도 / 성립 후 점수·유령 | false |
| `GetGameDurationSeconds` | 경기 전체 시간 | 기본값 반환 |
| `OnRoundTimeUp` | 라운드 종료 처리, true면 경기 종료 | false |
| `OnGameEnding` | 마지막 정산 | 빈 구현 |
| `OnEnergyPicked` / `OnThrowableHit` | 에너지 / 던지기 피격 | false / 빈 구현 |
| `UsesTargetColors` | 목표 색 규칙 사용 여부 | true |

- 구현: `UCTRule_TailChase`(꼬리잡기), `UCTRule_TreasureHunt`(보물찾기, `UsesTargetColors()=false`).
- **모드 선택**: `ACTGameMode::InitGameState()`가 `UCTGameInstance`에 저장된 로비 설정(`FCTMatchSettings::Mode`)을 읽어 `CreateRule()`을 호출한다. 설정이 없으면 꼬리잡기. 개발 빌드(`!UE_BUILD_SHIPPING`)에서는 명령줄 `-CTMode=<번호>`로 강제할 수 있다. `GetRule()`은 규칙이 없으면 꼬리잡기를 만든다. 생성 시 `GameState->SetModeType()`으로 복제해 클라이언트 UI가 모드를 알게 한다.
- `ECTGameModeType`: `TailChase`, `TreasureHunt`(+`Count`). `FCTMatchSettings::GetMode()`는 값이 TreasureHunt가 아니면 모두 꼬리잡기.
- 노출 제어: `IsModePlayable()`은 꼬리잡기는 항상 true, 보물찾기는 Shipping 빌드에서 항상 false, 그 외엔 CVar `ct.EnableTreasureHunt`(기본 1). 로비의 모드 선택 줄은 `IsModeSelectVisible()`로 Shipping에서 false, 그 외엔 CVar `ct.ShowGameModeSelect`(기본 0).

**꼬리잡기 규칙 요약** (`CTRule_TailChase.cpp`)
- `OnMatchStart`: 색 None, 꼬리 = `InitialTailCount`(1), 유령 해제, 전화 쿨타임 0.
- `OnRoundStart`: 맵의 꼬리 아이템을 모두 제거(라운드 끝에 일부러 맞아 떨어뜨린 아이템을 새 라운드에 먹는 꼼수 방지) → 유령 부활 → 꼬리가 `MinTailCountAtRoundStart`(1) 미만이면 보충 → `AssignColorsAndTargets`.
- 색 배정: 8색(`CTColorCount`) 풀을 Fisher-Yates로 섞고, 최대 32회 시도해 직전 색과 충돌이 가장 적은 배치를 고른다. `bTargetFromActiveColorsOnly`(true)이면 사용 중인 색을 정렬해 **다음 색이 목표**(마지막은 처음으로, 혼자면 목표 없음). 사람이 색보다 많으면 초과자는 색 없음.
- `HandleGrab`: 경기 중, 자기 자신/유령 제외, `Victim 색 == Grabber 목표 색`이어야 한다. `bUseGrappleStruggle`(true)이면 `BeginGrapple()`로 F 연타 대결, 아니면 `FinalizeGrab`.
- `FinalizeGrab`: `bTakeAllTailsOnGrab`(true)이면 피해자 꼬리 전부(최소 1), 아니면 1개를 잡은 쪽이 얻고 피해자는 유령이 된다. `MulticastPlayAction(Caught)`, `MulticastPlayCatchVfx`, BP 이벤트 `OnTailGrabbed`.
- `ComputeResult`: 꼬리 최다 플레이어 전원이 승자, 2명 이상이면 `bOutTie`.

**설계 이유와 주의점**
- 규칙 객체는 비복제라 클라이언트는 규칙을 직접 볼 수 없다. 클라이언트가 알아야 할 정보(모드, 팀 색, 라운드 승점)는 `ACTGameState`로 복제한다.
- `ACTGameMode`는 두 규칙 클래스를 `friend`로 선언해 `CollectPlayers()`, `InitialTailCount` 등 보호 멤버를 직접 쓰게 한다.
- `CTRule_TreasureHunt.h` 상단 주석의 "아직 없음: 오브젝트 던지기, 우리 팀 에너지 복제"는 **오래된 주석**이다. 실제 코드에는 둘 다 구현되어 있다(6장).
- 모드 추가 절차: `ECTGameModeType`에 값 추가 → `UCTGameRule` 파생 → `CreateRule` 분기 → 필요 시 `GameState` 복제값 추가.

---

## 6. 보물찾기 시스템

**목적**: 팀 대 팀(최대 4:4)이 맵의 에너지를 모으고, 오브젝트를 던져 상대의 에너지를 떨어뜨린다.

### 6-1. 팀 배정과 팀 색 (`OnMatchStart`)
- 로비에서 정해진 팀(0/1)은 유지한다. 미배정은 **사람 먼저, AI 다음** 순으로 인원이 적은 팀(같으면 0번)에 넣는다.
- 팀 색: `UCTGameInstance::GetStoredTeamColors()`가 서로 다른 2색이면 사용, 아니면 8색 중 서로 다른 2색을 무작위로 뽑는다. 같은 팀은 같은 색. `SetColors(팀 색, None)`로 목표 색은 쓰지 않는다. `GameState`에 `SetTeamColors`, `SetTeamRoundWins({0,0})`.
- 에너지, 결과 에너지, 꼬리는 0. 유령 없음.

### 6-2. 라운드 준비 (`OnRoundStart`)
순서: 개인 에너지와 꼬리를 0으로 → 기존 에너지 제거 → 기존 오브젝트 제거 → 에너지 배치 → 오브젝트 배치 → 플레이어 무작위 스폰.

| 항목 | 값 | 위치 |
|---|---|---|
| 총 라운드 | 3 | `TotalRounds` |
| 라운드당 에너지 | 100개 | `EnergyPerRound` |
| 꼬리 1개당 에너지 | 10 | `EnergyPerTail` |
| 잡기 정지 시간 | 2 s | `GrabFreezeSeconds` |
| 스폰 최소 간격 | 800 cm | `SpawnMinDistance` |
| 에너지 간 최소 거리 | 150 cm | `SpawnEnergies` |
| 오브젝트 간 최소 거리 | 600 cm | `SpawnThrowables` |
| 오브젝트 개수 | 20 | GameMode `TreasureThrowableCount` |
| 경기 전체 시간 | `RoundDurationSeconds * 3` | `GetGameDurationSeconds` |

- `PickClearPoint`: PlayerStart 중 하나를 기준으로 NavMesh `GetRandomReachablePointInRadius`(반경 30000 cm)로 뽑고, 캡슐 겹침 검사(`OverlapBlockingTestByChannel`, `ECC_Pawn`)로 벽/오브젝트에 끼는 자리를 제외한다. **NavMesh가 없는 맵**은 반경 `min(Radius, 3000)` 안 임의 지점에서 아래로 트레이스해 바닥을 찾는다(경사 `ImpactNormal.Z < 0.85`이거나 시작점보다 300 cm 넘게 높으면 제외).
- 시도 횟수: 에너지 `EnergyPerRound*30`, 오브젝트 `Wanted*60`. 못 채우면 채운 만큼만 배치하고 개수를 로그에 남긴다.
- 플레이어 스폰: 8 m 이상 떨어진 자리를 최대 400회 시도하며 80회마다 간격을 0.75배로 줄인다. 끝내 못 찾으면 그 자리에 둔다. `StopMovementImmediately` 후 `TeleportTo`.
- 오브젝트 모양 우선순위: 프로젝트 설정 `CT Maps`의 맵 항목(`TreasureThrowable`) > GameMode의 `TreasureThrowableClass` > 기본 공.

### 6-3. 에너지 픽업 (`ACTEnergyPickup`)
- 트리거 구(반경 90 cm, `ECC_Pawn` Overlap). 서버에서 겹치면 `GameMode->HandleEnergyPickup` → 규칙 `OnEnergyPicked`가 true를 돌려주면 소멸(`bConsumed`). 놓이는 순간 이미 겹친 캐릭터는 바로 먹는다. 경기 중(`Playing`)에만 유효.
- `OnEnergyPicked`: 기권/관전자 제외, `SetPlayerEnergy(에너지 + Amount)`, 총 수집량 `CollectedTotal` 누적(던져서 잃은 양은 빼지 않음, 결과 점수용).
- `SetPlayerEnergy`: `Energy` 설정 후 `TailCount = Energy / 10`, 같은 팀 전원에게 **팀 합계**를 `SetTeamEnergy`로 기록한다.

### 6-4. 던지기 오브젝트 `ACTThrowable`
**물리**: 루트 `USphereComponent`(반경 `BodyRadius` 22.5 cm, `PhysicsActor` 프로필, CCD 사용). 시각 메시는 충돌 없음(`VisualScale` 0.45). 질량 3 kg, 반발 0.65(결합 모드 Max), 마찰 0.6, 선형 감쇠 0.05, 각 감쇠 0.4. 이동 복제 사용, 갱신 주기 30 Hz. `OnComponentHit`는 서버에서만 바인딩한다.

**들기/던지기 흐름**
1. 잡기 키: 꼬리 우선(`ACTTail::TryTouch`), 못 잡으면 보물찾기에서만 `TryPickupThrowable`. 높이 차 250 cm 이하, 거리 ≤ `ThrowPickupDistance`(600), 120 cm 초과면 정면 ±`ThrowPickupHalfAngleDegrees`(80도) 안, 가장 가까운 것. `CanBePickedUp()` = 들고 있지 않고 위험(`bDangerous`) 상태가 아님.
2. 들기 `AttachToHolder`: `HeldBy` 복제(`OnRep_HeldBy` → `ApplyHeldState`). 물리와 충돌을 끄고 캐릭터 루트에 부착(`HeldOffset` = (0,0,135)).
3. 던지기 `ServerThrow`(서버): 경기 중, 부스 잠김/유령/대결 중이 아니어야 한다. 방향 = **캐릭터 정면(카메라 아님)** 에서 `ThrowUpAngleDegrees`(8도) 위로, 속도 `ThrowSpeed`(1500 cm/s), 놓는 위치 = 몸 앞 `ThrowReleaseOffset`(60, 0, 10). `Throw`가 손에서 해제하고 위치를 옮기며 각속도(Y축 720도/s)를 주고 `Grab` 애니를 재생한다.
4. 위험 상태: `bDangerous=true`, `DangerSeconds`(2.5 s)가 지나거나 0.6 s 후 속도 < 150 cm/s이면 해제하고 `HitVictims`/`Thrower`를 초기화한다. 이후엔 평범한 물체.

**충돌과 자기 제외**
- 던진 직후 `ThrowerSafeSeconds`(0.4 s) 동안 `IgnoreComponentWhenMoving`으로 던진 사람의 루트를 무시한다.
- `HandleHit`(서버, 위험 상태일 때만): `Victim == Thrower`면 무시(튕겨 돌아온 공에도 피해 없음), 같은 던지기에서 이미 맞은 대상(`HitVictims`)도 무시한다. **아군은 피해를 받는다.** 통과하면 `GameMode->HandleThrowableHit` → 규칙 `OnThrowableHit`.
- `OnThrowableHit`: 맞은 연출 `PlayKnockedDown(Velocity)`은 에너지가 없어도 재생. 손실 = 에너지 ≤ 0이면 0, 아니면 `max(에너지/2, 1)`. `SetPlayerEnergy`로 감소(꼬리도 비례 감소) 후 `DropEnergies`가 피해자 주변 반경 170~320 cm 임의 위치의 바닥(위 200 ~ 아래 600 트레이스)에 에너지 `Loss`개를 생성한다.

**미리보기** (`ACTCharacter::UpdateThrowPreview`, 로컬): 오브젝트를 들고 있고 부스 잠김이 아닐 때 점 `ThrowPreviewDotCount`(14)개를 `SetOnlyOwnerSee(true)`로 던진 본인에게만 보여준다. 서버와 같은 방향, 속도, 놓는 위치로 `ThrowPreviewStepSeconds`(0.09 s) 간격의 포물선(`GetGravityZ` 적용)을 계산하고, `ECC_Visibility` 라인 트레이스에 막히면 그 지점에 점을 찍고 이후 점은 숨긴다. 점 크기 `ThrowPreviewDotScale` 0.07. 점은 처음 필요할 때 한 번만 만든다.

### 6-5. 잡기 (보물찾기)
`HandleGrab`: 경기 중, 다른 팀만(아군 불가), 피해자 폰이 있고 이미 멈춘 상태(`IsBoothLocked`)가 아니어야 한다. 피해자를 `SetBoothLocked(true)`로 2초 정지시키고(`Caught` 애니), `CreateWeakLambda` 타이머로 해제한다. **꼬리를 빼앗지 않고 대결(F 연타)도 쓰지 않는다.** 정지 중에도 무적이 아니다.

### 6-6. 팀 에너지 복제와 정보 은닉
`ACTPlayerState` 복제 조건:

| 변수 | 조건 | 의미 |
|---|---|---|
| `Energy` | `COND_OwnerOnly` | 개인 에너지는 본인만 |
| `TeamEnergy` | `COND_OwnerOnly` | 각 팀원이 자기 PlayerState로 우리 팀 합계를 받음. 상대 팀은 알 수 없음 |
| `ResultEnergy` | 모두 | 경기 종료 후 개인 수집 총량 공개 |
| `TeamId`, `AssignedColor`, `TailCount` | 모두 | 팀, 색, 꼬리 길이는 보임 |

따라서 상대 에너지는 클라이언트에서 알 수 없고 꼬리 길이(`TailCount`)로만 짐작해야 한다(AI도 동일).

### 6-7. 결과 판정
- `ScoreRound`(라운드당 1회, `bRoundScored`): 팀 에너지 합을 비교해 큰 팀 승점 +1, **동점이면 양 팀 +1**. `GameState->SetTeamRoundWins`로 복제.
- `OnRoundTimeUp(n)`: 정산 후 `n >= 3`이면 true(경기 종료). `OnGameEnding`: 마지막 라운드가 정산되지 않았다면 정산, 맵의 에너지 제거, 모든 플레이어에게 `ResultEnergy`(총 수집량) 설정.
- `ComputeResult`: 승점이 같으면 `bOutTie`이며 전원 공동 승자, 아니면 이긴 팀 전원이 승자.

**설계 이유와 주의점**
- 던지기 오브젝트는 서버 물리 + 이동 복제 + `HeldBy` 복제로 동기화한다. 개발용 시험 `ct.ThrowSyncTest`(`Tools/CTThrowSyncTest.ps1`)가 이를 자동 검증한다.
- 미리보기는 서버의 던지기 계산을 복제해 쓰므로 `ThrowUpAngleDegrees`, `ThrowSpeed`, `ThrowReleaseOffset`을 바꾸면 양쪽이 함께 바뀐다(같은 변수 사용). 단 미리보기는 라인 트레이스라 구의 반지름과 반발은 반영하지 않는다.
- 보물찾기는 Shipping 빌드에서 플레이할 수 없다(`IsModePlayable`).

---

## 7. 라운드, 점수, 매치 흐름

**목적**: 로비 → 카운트다운 → 경기(라운드 반복) → 결과 → 로비 복귀를 서버가 진행하고 `ACTGameState`로 복제한다.

**단계 `ECTGamePhase`**: `WaitingForPlayers`, `Playing`, `Finished`, `Countdown`(시작 전), `Lobby`.

**흐름**
1. 접속: `PreLogin`이 `Playing`/`Finished` 중 입장을 거절한다(`CTJoinReject::GameInProgress`). `PostLogin`/`HandleSeamlessTravelPlayer` → `InitPlayerForGame`(시작 꼬리 부여, 로컬 컨트롤러면 방장 지정). `bAutoStartForTesting`(기본 true)이면 접속마다 `BeginStartCountdown`을 다시 건다.
2. 시작 카운트다운: `GameStartCountdownSeconds`(3 s) 후 `StartGame`. `SetStartCountdown`을 먼저, `SetPhase(Countdown)`을 나중에 호출해 클라이언트의 남은 시간이 정확하게 한다.
3. `StartGame`: 이전 결과와 기권 기록 초기화 → `SpawnAIPlayers` → `Rule->OnMatchStart` → 경기 종료 시각 = `지금 + GetGameDurationSeconds(GameDurationSeconds)`, 기권 해제 시각 = `지금 + 경기시간 * ForfeitAvailableRatio(0.5)` → `Playing` → `OnGameStarted` → `StartNewRound` → `RefreshAllChaseBuffs` → `TickGameFlow` 0.2 s 반복 타이머.
4. `TickGameFlow`(0.2초): 경기 종료 시각 도달 → `EndGame`. 아니면 라운드 종료 시각 도달 → `Rule->OnRoundTimeUp(round)`가 true면 `EndGame`, 아니면 `StartNewRound`.
5. `StartNewRound`: 라운드 번호 +1, `Rule->OnRoundStart`, 라운드 종료 = `min(지금 + RoundDurationSeconds, 경기 종료 시각)`, `OnRoundStarted`.
6. `EndGame`: 타이머 해제 → `OnGameEnding` → `ComputeResult` → **`SetGameResult`를 먼저, `SetPhase(Finished)`를 나중에**(결과 화면에 승자가 동시에 도착) → 추격 버프 해제 → `ReturnToLobbyDelaySeconds`(20 s) 뒤 자동 `ReturnToLobby`(복귀 시각 복제) → `OnGameFinished`.
7. `ReturnToLobby`: 로비 맵 이름이 짧으면 `SearchForPackageOnDisk`로 전체 패키지 경로를 얻는다(실패 시 `/Game/Levels/` 접두사). AI를 정리한 뒤 `ServerTravel`(Seamless Travel). 방장은 `RequestReturnToLobby`로 조기 복귀할 수 있다(로컬 컨트롤러만, `Finished`에서만).

**중도 이탈/기권**
- `Logout` → `HandlePlayerLeftDuringGame`: 월드 해체 중이면 무시, `Playing`이 아니면 무시, 남은 인원 < 2이면 `EndGame`, `UsesTargetColors()` 모드는 남은 색끼리 목표를 다시 잇는다.
- `HandleForfeit`: 경기 중, 기권 개방 후, 1회만. 꼬리 0, 색 None, 유령, `bForfeited` 설정 → `CollectPlayers`에서 제외(색 배정, 순위, 승자 계산에서 빠짐), 이탈과 같은 처리.
- `HandleGhostRelocate`: 유령이 F로 무작위 PlayerStart로 이동. 유령 상태당 1회(`HasUsedGhostRelocate`), 다른 캐릭터가 `GhostRelocateClearRadius`(200 cm) 안에 있는 시작점은 우선 제외. 부활은 다음 라운드 시작 때.

| 항목 | 기본값 (GameMode) |
|---|---|
| GameDurationSeconds | 300 s (로비 설정 `GameMinutes*60`이 있으면 덮어씀) |
| RoundDurationSeconds | 60 s (로비 `RoundSeconds`) |
| CountdownSeconds (라운드 전환 표시) | 5 s |
| GameStartCountdownSeconds | 3 s |
| InitialTailCount | 1 (로비 `InitialTails`) |
| MinTailCountAtRoundStart | 1 |
| PhoneCooldownSeconds | 30 s (로비 `PhoneUses`) |
| ForfeitAvailableRatio | 0.5 |
| ReturnToLobbyDelaySeconds | 20 s |
| bUseGrappleStruggle / bTakeAllTailsOnGrab | true / true |
| bTargetFromActiveColorsOnly | true |
| TreasureThrowableCount | 20 |

**GameState 복제 항목**: `Phase`, `RoundNumber`(둘 다 `OnRep`에서 `OnPhaseChanged`/`OnRoundChanged` 델리게이트), `StartServerTime`, `RoundEndServerTime`, `GameEndServerTime`, `ForfeitUnlockServerTime`, `ReturnToLobbyServerTime`, `RoundDurationSeconds`, `CountdownSeconds`, `ModeType`, `TeamColors`, `TeamRoundWins`, `TeamSwapRequests`, `Winners`, `bIsTie`, 로비 항목(`MatchSettings`, `SelectedMapIndex`, `RoomCode`, `LobbyMinPlayers`). 남은 시간은 모두 서버 월드 시각 차(`GetServerWorldTimeSeconds`)로 계산해 클라이언트 시계 차이를 피한다.
- 라운드 전환 카운트다운: `IsCountdownActive` = (`Countdown` 단계이고 시작까지 남은 시간 > 0) 또는 (`Playing`이고 라운드 남은 시간이 0 초과, `CountdownSeconds` 이하). `GetCountdownNumber`는 올림한 정수.

**개발용 CVar** (`!UE_BUILD_SHIPPING`)
- `ct.TestMatchSeconds`: 0보다 크면 경기 길이를 그 초로, 라운드 길이를 `max(10, 경기/3)`으로 줄인다(로비 설정이 저장된 경우에만 적용).
- `ct.ThrowSyncTest`: 던지기 동기화 자동 시험(시작 카운트다운을 최소 30 s로 연장, `DevThrowSyncTest` 0.5초 타이머).
- `ct.EnableTreasureHunt`(기본 1), `ct.ShowGameModeSelect`(기본 0).

**설계 이유와 주의점**
- 결과/시각 값을 먼저 쓰고 단계를 나중에 바꾸는 순서는 `OnRep_Phase` 시점에 값이 이미 도착해 있게 하기 위해서다.
- 보물찾기는 `GetGameDurationSeconds`가 `RoundDurationSeconds*3`을 돌려주므로 로비의 "경기 시간"보다 **라운드 길이**가 경기 길이를 정한다.
- 마지막 라운드 종료와 경기 종료 시각이 겹치면 `EndGame` → `OnGameEnding`에서 정산하며, `bRoundScored`가 이중 정산을 막는다.
- `HandlePlayerLeftDuringGame`의 "남은 인원 < 2이면 종료"는 AI도 인원으로 센다(`CollectPlayers`는 관전자와 기권자만 제외).
