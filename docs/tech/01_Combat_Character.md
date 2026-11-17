# 01. 전투·캐릭터 (잡기 / 밀기 / 꼬리 / 혀·그래플 / 이동)

> 대상: UE 5.7, 접두사 `CT`, 게임 "Hey Chicken!". 아래 수치는 모두 `Source/CatchTheTail/` 코드의 기본값이다.
> 많은 값이 `UPROPERTY(EditDefaultsOnly)`라서 캐릭터 BP(또는 GameMode BP)가 덮어쓸 수 있다. 문서의 값은 "C++ 기본값"이다.

관련 파일 요약

| 파일 | 역할 |
|---|---|
| `Player/CTCharacter.h/.cpp` | 이동, 달리기, 잡기/밀기 서버 판정, 꼬리 생성, 몽타주 |
| `Player/CTGrappleComponent.h/.cpp` | 꼬리를 잡은 뒤의 F 연타 줄다리기(대결) |
| `Tail/CTTail.h/.cpp` | 캐릭터 뒤에 붙는 꼬리 액터(시각, 닿음 판정) |
| `Tongue/CTTongueComponent.h/.cpp` | 혀 메시 흔들림 연출 (연출 전용) |
| `Player/CTCharacterMovementComponent.*` | 가속 곡선, 낙하 중력 배율 |
| `Core/CTRule_TailChase.cpp` | 꼬리잡기 규칙: 잡기 허용 조건, 꼬리 이동, 색/목표 배정 |
| `Core/CTGameMode.cpp` | `HandleTailGrabbed` / `FinalizeTailGrabbed`를 규칙 객체에 위임 |
| `World/CTTailPickup.cpp` | 꼬리 아이템(뽑혀 나온 꼬리를 다시 먹기) |

---

## 1. 잡기 판정

### 목적
내 "목표 색" 플레이어의 꼬리를 잡으면 대결(줄다리기)이 시작되고, 이기면 그 사람의 꼬리를 가져온다. 판정은 항상 서버가 한다.

### 동작 흐름

```
[로컬] 잡기 키(GrabAction, 또는 설정의 보조키 GrabAlt=G) 입력
   OnGrabPressed()
     ├ 이미 대결 중(IsGrappling) → Grapple->LocalMash() 로 연타 입력 (§4)
     ├ CanUseActions() 아니면 중단
     ├ 물건을 들고 있으면(보물찾기) → ServerThrow (쿨타임 없음)
     ├ 로컬 쿨타임(GrabCooldownSeconds) 안이면 중단
     └ 몽타주 선재생 + ServerGrab() (Server, Reliable)
[서버] ServerGrab_Implementation
     ├ 검사: 유령/부쉬/전화부스/대결중/Playing 페이즈 아님 → 거절
     ├ 동작 중 재시작 금지 (bGrabActive, GrabLockEndTime, 시작 타이머)
     ├ 서버 쿨타임 검사 (GrabCooldownSeconds * 0.9)
     ├ bRequireTailTouch(기본 true) → 잡기 구간(Grab Window) 개방
     │     └ 구간 중 0.03초마다 TickGrabWindow(): 모든 ACTTail 에 TryTouch
     │           └ ACTTail::TryTouch → GameMode->HandleTailGrabbed
     │                 └ UCTRule_TailChase::HandleGrab
     │                       ├ bUseGrappleStruggle(기본 true) → BeginGrapple (대결 시작)
     │                       └ false 면 FinalizeGrab (즉시 꼬리 이동)
     └ bRequireTailTouch=false → TryGrabByCone (정면 부채꼴 즉시 판정)
   MulticastPlayAction(Grab) 로 전원 화면에 연출 (성공 여부와 무관)
```

### 후보 탐색과 조건
- 꼬리 접촉 방식(기본): 잡기 구간 동안 월드의 모든 `ACTTail`을 `TActorIterator`로 돌며 `TryTouch(this)`.
  `TryTouch` 조건(`CTTail.cpp`): 서버일 것, 이미 `bCaught`가 아닐 것, 주인이 있고 본인이 아닐 것, `Toucher->IsGrabActive()`일 것,
  잡는 캐릭터 위치와 꼬리 끝(`GetTouchLocation()`: 캡 메시의 Bounds 중심, 몸통이 없으면 받침) 거리가 `TouchRadius + 캡슐 반지름` 이하일 것.
  통과하면 `GameMode->HandleTailGrabbed(잡은PS, 당한PS)`.
- 규칙 검사(`UCTRule_TailChase::HandleGrab`): Playing 페이즈, 둘 다 유령 아님, 잡는 쪽 `TargetColor != None`,
  상대의 `AssignedColor == 내 TargetColor` (목표 색만 가능).
- 부채꼴 방식(`TryGrabByCone`, `bRequireTailTouch=false`일 때): 목표 색이고 유령이 아닌 플레이어 중 `IsInFrontCone(상대, GrabDistance, GrabHalfAngleDegrees)`인 첫 사람.
  `IsInFrontCone`은 높이(Z)를 무시한 2D 거리와, 정면 벡터와의 각도(`cos` 비교)로 판단하고 거의 겹치면(거리 < `KINDA_SMALL_NUMBER`) 성공으로 본다.
- 잡기 구간 개방 방식 3가지(`ServerGrab`):
  1. `bUseMontageGrabWindow`(기본 true)이고 `GrabMontage`에 `UCTAnimNotifyState_GrabWindow`가 있으면, 노티파이 Begin/End가 서버에서 `OnGrabNotifyBegin/End`를 불러 구간을 연다
     (`StartGrabWindow(GrabWindowMaxSeconds)`; 최대 시간은 안전장치). 서버 메시도 몽타주를 진행하도록 `VisibilityBasedAnimTickOption = AlwaysTickPose`.
  2. 노티파이가 없고 `GrabWindowDelaySeconds > 0`이면 타이머로 `BeginGrabWindow`(지연 후 `GrabActiveSeconds` 동안).
  3. 지연이 0이면 즉시 `BeginGrabWindow`.
- `StartGrabWindow`는 시작 즉시 한 번 `TickGrabWindow()`를 불러 이미 닿아 있는 꼬리를 바로 확인한다.
  `TickGrabWindow`는 구간 종료 시각 초과, 유령, 페이즈 이탈이면 구간을 닫고, 닿기에 성공하면(꼬리가 먼저) 구간을 종료한다.

### 서버 권한 / 쿨다운
- 입력은 클라이언트가 받고(`Server, Reliable` RPC), 최종 판정·쿨다운·상태는 서버만 쓴다. `AIGrab()`도 `ServerGrab_Implementation()`을 그대로 호출해 사람과 같은 규칙을 거친다.
- 쿨다운은 이중이다. 로컬(`LocalLastGrabTime`)은 UI·입력 거절용이고, 서버(`LastGrabTime`)가 실제 기준이다.
  서버는 시계 오차를 고려해 `GrabCooldownSeconds * 0.9`로 검사한다(코드 주석: "쿨타임, 시계 오차로 90%").
  쿨타임은 "입력 순간부터" 센다(`GrabCooldownSeconds` 주석).
- 몽타주는 로컬이 선재생하고, 서버 `MulticastPlayAction`에서 본인(`IsLocallyControlled`)은 다시 재생하지 않는다.

### 잡기 대결 F 연타 규칙
§4-2(`UCTGrappleComponent`) 참고.

### 핵심 수치표 (`CTCharacter.h`)

| 항목 | 기본값 | 변수 |
|---|---|---|
| 잡기 거리 | 170 cm | `GrabDistance` (부채꼴 방식, AI가 거리 잴 때도 사용) |
| 부채꼴 반각 | 60도 | `GrabHalfAngleDegrees` |
| 꼬리 접촉 필요 | true | `bRequireTailTouch` |
| 몽타주 Grab Window 사용 | true | `bUseMontageGrabWindow` |
| 판정 최대 시간(안전장치) | 1.5 s | `GrabWindowMaxSeconds` |
| 타이머 방식 지연 / 지속 | 0.1 s / 0.35 s | `GrabWindowDelaySeconds` / `GrabActiveSeconds` |
| 잡기 쿨타임 | 1.5 s (서버는 x0.9) | `GrabCooldownSeconds` |
| 꼬리 끝 접촉 반경 | 105 cm + 캡슐 반지름 | `ACTTail::TouchRadius` |
| 구간 판정 주기 | 0.03 s | `StartGrabWindow`의 타이머 |

### 관련 함수
`ACTCharacter::OnGrabPressed / ServerGrab_Implementation / TryGrabByCone / StartGrabWindow / TickGrabWindow / EndGrabWindow / OnGrabNotifyBegin / OnGrabNotifyEnd / IsInFrontCone`,
`ACTTail::TryTouch`, `ACTGameMode::HandleTailGrabbed`, `UCTRule_TailChase::HandleGrab / FinalizeGrab`.

### 설계 이유 / 주의점
- 꼬리 접촉 방식은 성공 여부가 미정이라("성공 여부 미정이라 false") 구간을 열고 `MulticastPlayAction(Grab, false)`를 먼저 보낸다. 연출은 성공 여부와 무관하다.
- 동작 중에는 재시작하지 않는다(연타 방지). 유령이 되면 `EndGrabWindow()`로 구간을 즉시 닫는다.
- `CanUseActions()`는 유령, 대결 중, 부쉬 안, 전화부스 사용 중, Playing이 아닌 페이즈에서 false.
- 부채꼴/꼬리 접촉 두 방식 모두 최종 허용은 규칙(`HandleGrab`)이 다시 검사한다(목표 색, 유령, 페이즈).

---

## 2. 밀기 (푸시)

### 목적
정면의 상대를 밀어 넘어뜨리고 날려 보낸다(방해용). 목표 색 제한이 없다.

### 동작 흐름
```
[로컬] PushAction → OnPushPressed
   ├ CanUseActions() 아니면 중단
   ├ 로컬 쿨타임(PushCooldownSeconds) 안이면 중단
   └ 몽타주 선재생 + ServerPush()
[서버] ServerPush_Implementation
   ├ 검사: 유령 / 부쉬 / 전화부스 / 대결중 / Playing 페이즈
   ├ 서버 쿨타임: Now - LastPushTime < PushCooldownSeconds → 거절 (잡기와 달리 0.9 보정 없음)
   ├ MulticastPlayAction(Push)  (전원 화면: 몽타주 + 밀기 효과음)
   └ GameState->PlayerArray 순회
        ├ 건너뜀: 자기 자신, 유령, 정면 부채꼴(PushDistance, PushHalfAngleDegrees) 밖
        ├ 방향 = (상대 - 나) 수평 정규화
        ├ PushSpeed 역산 (아래 식)
        ├ OtherCharacter->LaunchCharacter(방향*PushSpeed + (0,0,PushUpSpeed), XY 덮어쓰기, Z 덮어쓰기)
        └ OtherCharacter->PlayKnockedDown(방향)
```

### 충격량(속도) 계산
"키만큼 밀리게 속도 역산"(코드 주석). 피해자의 키 `Height = GetSimpleCollisionHalfHeight()*2`일 때

```
AirTime   = 2 * PushUpSpeed / |중력Z|
BrakingK  = max(GroundFriction * BrakingFrictionFactor, 1)
PushSpeed = Height * PushDistanceInHeights / (AirTime + 1 / BrakingK)
LaunchVelocity = Direction * PushSpeed + (0, 0, PushUpSpeed)
```
공중에 떠 있는 시간(`AirTime`) 동안 이동한 거리와 착지 후 마찰로 미끄러지는 거리(`1/BrakingK` 항)를 합쳐 약 "키 x 배수"만큼 밀려나도록 한 식이다. `PushUpSpeed <= 0`이면 `PushSpeed = 0`이다.

### 기절/넉백
- 쓰러지는 연출은 `PlayKnockedDown(KnockDirection)`: 날아가는 방향과 피해자가 보는 방향의 내적이 0 이상이면 `Pushed`(앞으로 엎어짐), 아니면 `PushedBack`(뒤로 넘어짐). 서버가 `MulticastPlayAction`으로 전원에게 알린다. `PushedBackMontage`가 없으면 `PushedMontage`를 쓴다.
- 쓰러짐~일어나기 동안은 입력이 잠긴다: `CanMoveNow()`가 `GrabMontage, PushMontage, PushedMontage, PushedBackMontage, GetUpMontage, GetUpBackMontage` 중 재생 중인 것이 있으면 false
  (코드 주석: 입력만 막고 감속은 이동 컴포넌트가 처리). 별도의 "기절 타이머"는 없고 몽타주 길이가 곧 경직 시간이다.
- 넘어진 몽타주가 끝나면(중단 아님, 유령 아님) `HandlePushedMontageEnded`가 앞/뒤에 맞는 일어나기 몽타주를 이어 재생한다. 몽타주가 없으면 바로 풀린다.
- 같은 `PlayKnockedDown`/`LaunchCharacter`는 줄다리기에서 뿌리쳤을 때(§4)도 쓰인다.

### 핵심 수치표

| 항목 | 기본값 | 변수 |
|---|---|---|
| 밀기 거리 | 150 cm | `PushDistance` |
| 부채꼴 반각 | 60도 | `PushHalfAngleDegrees` |
| 밀림 거리(피해자 키 배수) | 1.0 | `PushDistanceInHeights` |
| 상승 속도 | 200 cm/s | `PushUpSpeed` |
| 밀기 쿨타임 | 3.0 s | `PushCooldownSeconds` |
| 밀기 몽타주 재생 속도 | 2.0 | `PushMontagePlayRate` (잡기 등은 `ActionMontagePlayRate`=1.0) |

### 서버-클라 처리
- 판정, `LaunchCharacter`, 쿨다운은 서버. 클라이언트는 입력 시 로컬 쿨다운 확인 + 몽타주 선재생 + RPC. `LaunchCharacter`로 바뀐 속도는 캐릭터 이동 복제로 전파된다.
- `GetPushReadyPercent / GetPushCooldownRemainingSeconds`는 로컬 기록(`LocalLastPushTime`) 기반이라 "로컬만 정확"(헤더 주석).

### 관련 함수
`ACTCharacter::OnPushPressed / ServerPush_Implementation / PlayKnockedDown / MulticastPlayAction_Implementation / PlayActionMontage / HandlePushedMontageEnded / CanMoveNow / AIPush`.

### 주의점
- 밀기에는 "목표 색"·"꼬리" 조건이 없다. 유령만 대상에서 제외된다.
- 코드상 밀기 대상 필터에는 피해자가 부쉬 안인지 / 대결 중인지 검사가 없다(검사하는 것은 미는 쪽의 상태뿐). 이것이 의도인지는 코드 주석에 없다.

---

## 3. 꼬리 시스템

### 목적
꼬리는 플레이어의 점수이자 생존 수단이다. 꼬리 수(`TailCount`)가 많은 사람이 승리하고(`ComputeResult`), 꼬리를 뺏기면 유령이 된다.
화면에는 `ACTTail` 액터가 캐릭터 허리(Hips 본)에 붙어 "충전형 꼬리"(몸통 마디가 개수만큼 쌓이는 모양)로 표시된다.

### 상태의 소유자
- 꼬리 개수의 진실은 `ACTPlayerState::TailCount`(복제, 서버만 `SetTailCount/AddTailCount`, 0 미만 불가, `ForceNetUpdate` 후 서버에서도 `OnRep_TailCount` 직접 호출).
- `ACTTail`은 개수를 저장하지 않고, 매번 주인의 PlayerState 값을 읽어 마디 수를 맞춘다(`RefreshSegments`).
- 꼬리 액터가 직접 복제하는 값은 `bCaught` 하나뿐(`DOREPLIFETIME(ACTTail, bCaught)`).

### 동작 흐름
```
[서버] ACTCharacter::BeginPlay → SpawnTail()
   ├ TailClass 로 ACTTail 스폰 (Owner = 캐릭터), OwnedTail(복제) 에 기록
   ├ GetMesh() 의 TailSocketName(기본 "Hips") 본에 AttachToComponent (SnapToTargetNotIncludingScale)
   ├ ApplyTailOffset(): 본 기준 자세를 역변환해 TailOffset(위치/회전/스케일)대로 놓음
   └ OwnedTail->RefreshVisual()
[클라] OnRep_OwnedTail → ApplyTailOffset, RefreshBodyColor, RefreshTailVisual
       ACTTail::OnRep_AttachmentReplication → 주인의 ApplyTailOffset 재호출 + RefreshVisual
꼬리 수 변화(PlayerState 복제) → 주인 캐릭터 RefreshTailVisual → ACTTail::RefreshVisual
   RefreshVisual: 주인 색 계산 → RefreshSegments(마디 증감) → 머티리얼 갱신(ApplyLook)
```

### 구성/길이 규칙 (`CTTail`)
- 구성: 받침 `BaseMesh`(tail_03, 항상 표시) + 몸통 마디 `BodyMeshes`(tail_02) + 끝 캡 `CapMesh`(tail_01, 몸통이 있을 때만 표시).
- 마디 수 = `Clamp(주인 TailCount, 0, MaxBodySegments)`. 마디는 +Z로 `SegmentSpacing`씩 쌓고 캡은 맨 위 마디 위치로 옮긴다.
  증가는 새 마디가 `SegmentPopSeconds` 동안 스케일 0.3에서 1.0으로 커지는 연출, 감소는 끝 마디부터 제거.
- 꼬리가 0개이면 받침만 보인다.

### 잡힘/뽑힘(`bCaught`)과 색
- 유령이 되면(`OnGhostStateChanged(true)`, 서버) `OwnedTail->SetCaught(true)`; 부활하면 false. `SetCaught`는 서버 전용이며 `ForceNetUpdate` 후 서버에서도 `OnRep_Caught`를 직접 호출해 같은 연출을 낸다.
- `bCaught`이면 `LightAmount` 0(색 `CaughtColor` 회색, 발광 0), 아니면 1(주인 색, 발광 `LitEmissiveStrength`). `LightFadeSeconds` 동안 서서히 전환하며 변화 중에만 액터 Tick을 켠다.
  주인 색은 `PlayerState::GetAssignedColor()`로 결정(색 없음이면 `NoOwnerColor`). 머티리얼 파라미터는 `TailColor`, `EmissiveStrength`.
- 연출 전용: 모든 파트 `NoCollision`, 오버랩 이벤트 없음. 닿음은 충돌이 아니라 §1의 거리 계산으로 한다.

### 꼬리의 이동 규칙 (`UCTRule_TailChase`)
- 매치 시작: 모두 색 없음, `TailCount = InitialTailCount`(기본 1), 유령 해제.
- 라운드 시작(`OnRoundStart`): (1) 맵에 남은 `ACTTailPickup` 전부 제거 (주석: 라운드 끝 전에 일부러 기믹에 맞아 꼬리를 떨어뜨리고 보충분과 함께 먹는 꼼수 방지) (2) 유령 부활 (3) 꼬리가 `MinTailCountAtRoundStart`(기본 1)보다 적으면 그 수로 보충 (4) 색/목표 재배정.
- 잡기 성공(`FinalizeGrab`): `Taken = bTakeAllTailsOnGrab(기본 true) ? max(피해자 꼬리 수, 1) : 1`. 잡은 쪽 `AddTailCount(Taken)`, 피해자 `SetTailCount(max(꼬리-Taken, 0))` 후 `SetIsGhost(true)`.
  이어서 피해자에게 `MulticastPlayAction(Caught)`와 `MulticastPlayCatchVfx()`(꼬리 끝 별 효과, 크기 배율 `CatchVfxScale`) 호출, 마지막에 BP 이벤트 `OnTailGrabbed`.
- 색/목표 배정(`AssignColorsAndTargets`): 플레이어를 색 수(`CTColorCount`)까지만 배정. 색을 최대 32번 셔플하며 직전 색과 겹침이 적은 배치를 선택.
  `bTargetFromActiveColorsOnly`(기본 true)이면 사용 중인 색을 정렬해 "다음 색"이 목표(혼자면 목표 없음), 아니면 색 순서 다음(분홍 다음은 빨강).
- 승자(`ComputeResult`): 꼬리 최다. 동수가 둘 이상이면 `bOutTie = true`.

### 꼬리 아이템 (`ACTTailPickup`)
- 기믹(낙하물, 폭발 통, 도로 차량, 구르는 통나무)에 맞으면 `ACTTailPickup::DropTailsFromCharacter(Character, TailLoss, ...)`가 실제 잃은 만큼 `AddTailCount(-Lost)`.
  꼬리가 0이 되면 유령이 되고, 잃은 개수만큼 아이템이 머리 위 +90cm에서 방향 기준 -70~70도로 흩어져 튕겨 나온다.
- 아이템은 `SpawnActorDeferred` + `LaunchAway` 후 `FinishSpawning`으로 만들어, 생성 순간 겹친 캐릭터가 바로 먹지 못하게 대기 시간을 정한다(코드 주석). 기믹 클래스들의 `DroppedTailPickupDelay` 기본값은 1.2초.
- 먹기(`TryPickup`, 서버): Playing 페이즈, 기권자 아님. 일반 플레이어는 `AddTailCount(TailAmount)`, 유령은(`bAllowGhostPickup`일 때) 그 자리에서 부활하고 꼬리는 최소 `TailAmount`개로 하며 `RefreshChaseBuff()` 호출.
  이동 복제는 켜져 있고(`SetReplicatingMovement(true)`) 회전/흔들림은 각 화면 로컬 연출.

### 핵심 수치표

| 항목 | 기본값 | 위치 |
|---|---|---|
| 몸통 최대 마디 수 | 7 (에디터 범위 0~30) | `ACTTail::MaxBodySegments` |
| 마디 간격 | 94 cm (모델 높이 97, 주석: 97보다 작으면 겹침) | `SegmentSpacing` |
| 마디 성장 시간 | 0.2 s (시작 스케일 0.3) | `SegmentPopSeconds` |
| 잡힘 불빛 전환 | 0.25 s | `LightFadeSeconds` |
| 켜짐 발광 세기 | 4.0 | `LitEmissiveStrength` |
| 잡힘 색 | (0.12, 0.12, 0.12) | `CaughtColor` |
| 붙이는 본 | `Hips` | `TailSocketName` |
| 꼬리 오프셋 위치 / 회전 / 크기 | (0,-60,29) / Roll 90 / 0.5 | `TailOffsetLocation / Rotation / Scale` |
| 시작 꼬리 수 | 1 | `ACTGameMode::InitialTailCount` |
| 라운드 시작 최소 꼬리 | 1 (0이면 보충 안 함) | `MinTailCountAtRoundStart` |
| 잡으면 전부 뺏기 | true (false면 1개) | `bTakeAllTailsOnGrab` |

### 복제
- 서버가 꼬리를 스폰, `ACTCharacter::OwnedTail`(`ReplicatedUsing=OnRep_OwnedTail`)로 클라에 전달. 꼬리 액터는 `SetReplicatingMovement(false)`(캐릭터에 붙어 따라감).
- 개수는 PlayerState 복제, 잡힘 여부는 `bCaught` 복제. 마디와 색은 각 화면이 복제값으로 계산하는 순수 연출이다.

### 설계 이유 / 주의점 (코드 주석)
- "복제 스케일이 소수 2자리로 양자화돼 0이 되므로 주인이 오프셋 재계산": 부착 복제 때 스케일 0.5가 깨질 수 있어 `OnRep_OwnedTail`, `OnRep_AttachmentReplication`에서 `ApplyTailOffset()`을 다시 호출한다.
- "부착 후에야 주인 색/꼬리 수 확인 가능": 그래서 `OnRep_AttachmentReplication`에서도 `RefreshVisual()`을 호출한다.
- "BP 값은 생성자 이후 적용": 메시 에셋은 `BeginPlay`/`RefreshVisual`의 `ApplyMeshAssets()`에서 1회 적용.
- 캐릭터가 사라질 때 서버는 `OwnedTail->Destroy()`.

---

## 4. 혀와 그래플(꼬리 줄다리기)

### 4-1. 혀 (`UCTTongueComponent`) - 연출 전용
- `UPoseableMeshComponent` 상속, 충돌/오버랩 없음. 캐릭터 `Head` 본에 붙는다. 기본 메시 `/Game/Character/Socket/TONGUE`.
- 게임 로직과 무관하다. 전용 서버는 `TickComponent`에서 바로 리턴한다. 캐릭터 속도에 비례해 본 체인(루트 제외)을 `WaveAxis`(기본 Z) 주위로 사인파로 흔든다.
  위상은 본마다 `PhasePerBoneRadians`씩 지연, 끝으로 갈수록 진폭이 커진다(`AmplitudeExponent`).
- `BeginPlay`에서 `CompensateAttachBone()`이 붙은 본의 기준 자세를 역변환해 상쇄(혀 메시가 캐릭터와 같은 모델 좌표라서).

| 항목 | 기본값 |
|---|---|
| 정지/최대속도 각도 | 4도 / 25도 (`IdleAmplitudeDegrees / MoveAmplitudeDegrees`) |
| 정지/최대속도 주파수 | 1.2 / 4.5 회/초 |
| 본당 위상 지연 | 0.9 rad |
| 최대 흔들림 속도 | 800 cm/s (`FullWaveSpeed`) |
| 속도 반응 보간 | 6 (`SpeedResponse`) |

### 4-2. 그래플 (`UCTGrappleComponent`) - 잡은 뒤의 F 연타 줄다리기

#### 목적
꼬리를 잡자마자 뺏지 않고, 서버가 연타 횟수를 세어 게이지(0~1, 1이면 잡은 쪽 승리)를 움직이는 줄다리기로 결판낸다.
잡은 쪽이 유리하게 시작하고, 같은 상대를 짧은 시간 안에 다시 잡으면 그 유리함이 줄어든다(헤더 주석).
GameMode 옵션 `bUseGrappleStruggle`(기본 true)로 끄면 예전처럼 즉시 뺏는다.

#### 동작 흐름
```
UCTRule_TailChase::HandleGrab → 잡은쪽Grapple->BeginGrapple(피해자)   [서버]
  ├ 둘 중 하나라도 이미 대결 중이면 false
  ├ 시작 게이지 = clamp(max(MinStartGauge, StartGauge - RematchStartStep * 재도전횟수))
  │     (같은 잡은이를 상대로, 마지막 뿌리침 후 RematchResetSeconds 이내일 때만 재도전 횟수 사용)
  ├ 기준 축 GrappleAxis = 잡은 사람이 보는 수평 방향 (없으면 둘 사이 방향)
  └ SetState(Grabber/Victim): 달리기 해제, StopMovementImmediately + DisableMovement,
        둘 다 기준 축 방향으로 회전, 몽타주/UI 반영

연타: 로컬 F/G 또는 마우스 왼쪽 클릭 → LocalMash → ServerMash (Server, Unreliable)
  → ApplyMash (서버)
       ├ 초당 MaxMashPerSecond(12) 초과분은 무시 (최근 1초 기록)
       ├ Δ = 잡은 쪽이면 +GrabberGainPerMash, 잡힌 쪽이면 -VictimGainPerMash → 게이지 clamp(0,1)
       ├ 양쪽 컴포넌트의 Gauge 를 같은 값으로 갱신
       └ >=1 이면 Finish(true), <=0 이면 Finish(false)

매 틱 (서버, 잡은 쪽 컴포넌트):
  UpdatePullDrift : 이기는 쪽 위치로 진 쪽을 끌어당김
  UpdateFormation : 둘 다 같은 방향을 보게 하고 잡힌 쪽을 잡은 사람 바로 앞 일직선에 맞춤
  AI(사람 컨트롤러 아님): 반응 시간 후 AIMashRate(초당) 로 자동 ApplyMash
  시간 종료(EndServerTime) : Finish(Gauge >= 0.5)
  중단 조건: 상대 소멸, 한쪽 유령, 대결 상태 불일치, Playing 아님 → 양쪽 ClearState

Finish(bGrabberWon)
  ├ 양쪽 ClearState (이동 모드를 MOVE_Walking 으로 복구)
  ├ 승: 재도전 기록 삭제 → GameMode->FinalizeTailGrabbed (꼬리 이동, 피해자 유령; §3)
  └ 패(뿌리침): 재도전 횟수 기록(같은 사슬이면 +1, 아니면 1), 잡은 쪽을 EscapeKnockback 으로 날리고 PlayKnockedDown
```

#### 입력 규칙
- 대결 중 `OnGrabPressed()`는 잡기 대신 `LocalMash()`를 부른다. 대결 중에는 `CanUseActions()`가 false이므로 다른 행동은 불가하다.
- 연타로 인정되는 입력: 잡기 입력(`GrabAction`; 헤더 주석은 "F 연타"), 설정의 보조키 `CTCodeKeys::GrabAlt`(기본 `G`, `Tick`에서 `WasInputKeyJustPressed`로 폴링), 마우스 왼쪽 클릭(`OnGrappleMouseClick`, 대결 중에만 의미).
  게이지 UI는 "G"와 마우스 아이콘을 보여 준다. `GrabAction`의 실제 키(F) 매핑은 입력 에셋에 있어 C++ 코드로는 확인되지 않는다.
- `ServerMash`는 `Server, Unreliable`: 연타 한두 번 유실돼도 문제 없는 설계. 서버가 초당 12회까지만 인정해 매크로/과한 연타를 막는다.

#### 핵심 수치표 (`CTGrappleComponent.h`)

| 항목 | 기본값 | 변수 |
|---|---|---|
| 대결 제한 시간 | 3.5 s | `GrappleSeconds` |
| 잡은 쪽 시작 게이지 | 0.6 (60:40) | `StartGauge` |
| 재도전마다 시작 게이지 감소 | 0.05 (60 -> 55 -> 50) | `RematchStartStep` |
| 시작 게이지 하한 | 0.5 | `MinStartGauge` |
| 재도전 기억 유지 | 10 s | `RematchResetSeconds` |
| 연타 1회당 게이지 | 잡은 쪽 +0.05 / 잡힌 쪽 -0.04 | `GrabberGainPerMash / VictimGainPerMash` |
| 최대 인정 연타 | 12 회/초 | `MaxMashPerSecond` |
| AI 연타 속도 | 3~5 회/초 (대결마다 무작위) | `AIMashPerSecondMin / Max` |
| AI 반응 시간 | 0.35 s (x0.7~1.3 무작위) | `AIMashStartDelaySeconds` |
| 뿌리침 시 잡은 쪽 넉백 | 700 cm/s, 위로 250 | `EscapeKnockbackSpeed / EscapeKnockUpSpeed` |
| 끌려오는 속도 | 1초에 거리의 10% | `PullFractionPerSecond` |
| 끌려와도 유지하는 최소 거리 | 85 cm | `MinPullDistance` |
| 대결 시작 시 붙는 거리 | 100 cm | `GrappleHoldDistance` |
| 꼬리가 손을 향하는 속도 | 14 | `TailAimInterpSpeed` |
| 게이지 UI 표시 | true (사람이 조종하는 로컬 화면만) | `bShowGaugeUI` |

#### 연출
- 잡은 쪽: `GrabberMontage`가 없으면 `Grapple_Pull` 애니메이션을 동적 몽타주로 반복 재생. 게이지가 낮을수록 팔이 더 늘어난다(`SetArmStretchTarget(Lerp(1, ArmStretchGrabScale(1.5), 1 - Gauge))`).
- 잡힌 쪽: `Grapple_Flee`. 진행도(`GetMyProgress()`)에 따라 재생 속도가 0.8~1.8배. 꼬리는 잡은 쪽 두 손(`LeftHand`/`RightHand`) 가운데를 향해 보간(`UpdateTailAim`), 대결이 끝나면 `ApplyTailOffset()`으로 원위치.
- 몽타주가 끊기면 Tick에서 다시 재생해 대결이 끝날 때까지 반복(모든 화면).
- 게이지 UI: Slate 위젯(알약 막대, 남은 시간, 아이콘)을 `AddViewportWidgetContent`로 띄움. 이기고 있으면 노랑, 밀리면 분홍. AI끼리의 대결은 방장 화면에도 안 보이게 `APlayerController` 여부까지 확인(주석).
  이 UI는 코드로 만든 위젯이다(프로젝트 원칙은 UI 디자인을 BP에서 하는 것이므로 개선 후보).

#### 복제
`Role`(`ReplicatedUsing=OnRep_Role`), `Partner`, `Gauge`, `EndServerTime`을 복제한다. 종료 시각은 서버 시계(`GetServerWorldTimeSeconds`) 기준이라 남은 시간(`GetRemainingSeconds`)이 모든 화면에서 일치한다.
`OnRep_Role`이 몽타주/UI/팔 늘리기 갱신(`ApplyRoleEffects`)을 호출하며, 서버는 `SetState/ClearState` 안에서 직접 호출한다(서버/클라 공통 함수).

#### 설계 이유 / 주의점
- 결판 권한은 "잡은 쪽 컴포넌트" 하나에만 있다(`Finish`는 `Role == Grabber`일 때만, 시간 종료와 끌림도 잡은 쪽이 처리). 이중 처리를 막기 위한 구조.
- 재도전 기록은 "잡힌 쪽 컴포넌트"에 저장(`LastGrabber, RematchCount, LastEscapeTime`). 같은 사람에게 계속 매달려 이득 보는 것을 줄인다.
- 대결 중 한쪽이 사라지면(`EndPlay`) 상대의 상태도 풀어 준다. `SetState`와 `ClearState`는 "둘 다 호출해야 함"(헤더 주석).
- 시간 종료 시 `Gauge >= 0.5`이면 잡은 쪽 승리: 재도전으로 시작 게이지가 0.5까지 내려간 상태에서 동점이면 잡은 쪽이 이긴다.

### 관련 함수
`UCTGrappleComponent::BeginGrapple / SetState / ClearState / LocalMash / ServerMash_Implementation / ApplyMash / Finish / TickComponent / UpdatePullDrift / UpdateFormation / UpdateTailAim / ApplyRoleEffects / StartMontage / ShowGaugeUI`,
`ACTCharacter::GetGrapple / IsGrappling / OnGrabPressed / OnGrappleMouseClick`, `ACTGameMode::FinalizeTailGrabbed`, `UCTRule_TailChase::FinalizeGrab`.

---

## 5. 이동·점프 등 캐릭터 핵심 상태와 복제

### 목적
3인칭 이동, 달리기(게이지/쿨타임), 속도 버프, 점프, 유령 상태 등 캐릭터의 기본 규칙과 그 네트워크 동기화.

### 이동 가능 조건 (`CanMoveNow`)
```
전화부스 사용 중(bBoothLocked)                           → 불가
잡기/밀기/넘어짐/일어나기 몽타주 재생 중                  → 불가
Phase == Lobby                                           → 가능 (대기 로비는 자유 이동)
GameState 없음 또는 Phase != Playing                     → 불가
AssignedColor == None                                    → 불가 (색 배정 전 이동/점프 불가)
```
- `Move`는 `CanMoveNow()`가 false이면 입력을 무시한다(회전 `Look`은 가능). 이동 방향은 카메라(컨트롤러 Yaw) 기준이고 유령도 이동한다.
- `CanJumpInternal_Implementation`: 유령이면 점프 불가, `CanMoveNow()` false이면 불가, 나머지는 엔진 규칙.
- 대결 중에는 `DisableMovement()`로 이동 자체가 꺼진다(§4).

### 속도와 달리기 (서버가 결정, 복제)
```
MaxWalkSpeed = BaseWalkSpeed(600) * (1 + 버프 합),  최소 0
버프: Run +0.5 / Chase +0.2 / Ghost -0.5   (이름 FName 으로 구분, 같은 이름이면 값 갱신)
Chase: Playing 페이즈이고 유령이 아닐 때만 (RefreshChaseBuff)
```
- `SpeedBuffs`(`ReplicatedUsing=OnRep_SpeedBuffs`) 목록을 복제하고, 서버와 클라이언트 양쪽이 `MaxWalkSpeed`를 직접 계산한다(주석: "버프 목록 복제, 양쪽 계산").
- 달리기: `ServerSetRunInput(bPressed)` RPC → `TryStartRun`: 유령/부스/Idle 아님이면 거절. `Run` 버프 부여, `RunState = Running`, 종료 시각 = 서버 시간 + `RunDurationSeconds`.
  시간이 끝나거나(자연 종료) 일찍 떼면(`bCooldownOnEarlyRelease`=true일 때) `Cooldown`으로 가서 `RunCooldownSeconds` 후 `Idle`. 일찍 떼고 이 값이 false이면 곧바로 `Idle`.
  `GetRunGaugePercent`는 Running일 땐 남은 비율, Cooldown일 땐 차오르는 비율, Idle은 1.
- `RunState`, `RunStateEndTime`은 복제되어 HUD가 서버 시간 기준 남은 시간을 계산한다.
- 유령이 되면(`OnGhostStateChanged`) `ForceResetRun()`, `Ghost` 버프 부여, 부쉬/부스 해제, 꼬리 `SetCaught(true)`, 잡기 구간 종료, 점프 중지, 잡기/밀기 몽타주 중단(잡힘 몽타주는 유지).

| 항목 | 기본값 | 변수 |
|---|---|---|
| 기본 걷기 속도 | 600 | `BaseWalkSpeed` |
| 달리기 / 추격 / 유령 버프 | +0.5 / +0.2 / -0.5 | `RunBuffValue / ChaseBuffValue / GhostBuffValue` |
| 달리기 지속 | 3 s | `RunDurationSeconds` |
| 달리기 쿨타임 | 5 s | `RunCooldownSeconds` |
| 일찍 뗐을 때도 쿨타임 | true | `bCooldownOnEarlyRelease` |
| 회전 속도 | Yaw 540 | 생성자 `RotationRate` |
| 공중 제어 | 0.35 | `AirControl` |
| 걷기 제동 감속 | 2000 | `BrakingDecelerationWalking` |

### 점프와 중력
- 생성자 기본값 `JumpZVelocity=700`, `JumpMaxCount=1`(공중 점프 방지)이지만 `ApplyCameraSettings()`(BeginPlay)가 BP 값으로 덮어쓴다(주석: "BP 값은 생성자 이후 적용"): `JumpZVelocity = JumpVelocity(860)`, `GravityScale = JumpGravityScale(1.5)`.
- 점프 높이 = 속도^2 / (2 x 980 x 중력배율) (헤더 주석). 예전 값은 속도 700, 배율 1(높이 약 250 cm, 체공 1.4초). 지금은 속도와 중력을 함께 올려 체공시간이 길지 않게 했다.
- `UCTCharacterMovementComponent::GetGravityZ`: 낙하 중(`IsFalling() && Velocity.Z < 0`)에만 `FallGravityMultiplier(2.0)`배. 점프 높이는 그대로 두고 체공 시간만 줄인다.
- `GetMaxAcceleration`: `bUseAccelerationRamp`(기본 true)이고 지상일 때, 가속 = 기본 x `Lerp(StartAccelerationFactor(0.55), 1, (현재 2D 속도/최대 속도)^AccelerationRampExponent(1.2))`.
  느리게 출발해 점점 빨라지는 느낌. 서버와 클라가 같은 값으로 계산하므로 어긋나지 않는다(헤더 주석). 되돌리려면 BP에서 `bUseAccelerationRamp`를 끈다.
- 입력은 Enhanced Input: Jump는 Started에 `Jump`, Completed에 `StopJumping`.

### 복제되는 캐릭터 상태 (`GetLifetimeReplicatedProps`)

| 변수 | 의미 | 콜백 |
|---|---|---|
| `SpeedBuffs` | 속도 버프 목록 | `OnRep_SpeedBuffs` -> 최대속도 재계산 |
| `RunState` / `RunStateEndTime` | 달리기 상태 / 종료 서버 시각 | - |
| `OwnedTail` | 꼬리 액터 | `OnRep_OwnedTail` |
| `CurrentBush` | 들어가 있는 부쉬 | `OnRep_CurrentBush` |
| `bBoothLocked` | 전화부스 사용 중 | - |
| `HeldThrowable` | 들고 있는 던지기 오브젝트(보물찾기) | - |

그 밖에 유령 여부, 색, 꼬리 수는 `ACTPlayerState`가 복제하고 캐릭터는 `IsGhost()` 등으로 PlayerState를 읽는다.
`OnRep_PlayerState`는 늦게 도착한 PlayerState에 맞춰 색/꼬리/유령 상태를 다시 맞춘다(주석: "늦게 온 PlayerState 유령 상태 동기화", "유령 알림 선행 시 누락 보정").

### RPC 요약

| 함수 | 종류 | 설명 |
|---|---|---|
| `ServerSetRunInput` | Server, Reliable | 달리기 시작/중지 |
| `ServerGrab` / `ServerPush` | Server, Reliable | 잡기 / 밀기 요청 |
| `ServerThrow` | Server, Reliable | 보물찾기 오브젝트 던지기 (보는 방향, 위 8도, `ThrowSpeed`=1500 고정) |
| `ServerPlayEmote` -> `MulticastPlayEmote` | Server / NetMulticast, Reliable | 이모트 |
| `MulticastPlayAction` | NetMulticast, Unreliable | 잡기/밀기/잡힘/넘어짐 연출 |
| `MulticastPlayCatchVfx`, `MulticastPlayHitSfx` | NetMulticast, Unreliable | 효과 |
| `ClientShowNotice`, `ClientShowPhoneResult` | Client, Reliable | 안내 문구 / 전화 결과 |
| `ServerMash` (Grapple 컴포넌트) | Server, Unreliable | 대결 연타 |

### AI 조종
`AIGrab / AIPush / AISetRun / AIThrow`는 서버에서만 동작하며 사람 입력과 같은 서버 함수(`ServerGrab_Implementation` 등)를 직접 호출해 쿨타임/상태 검사를 그대로 받는다.
`GetGrabDistance / GetPushDistance / IsInFrontOf`는 AI가 거리를 잴 때 쓴다.

### 설계 이유 / 주의점 (코드 주석)
- 몽타주 중 이동 잠금은 "입력만 막고 감속은 이동 컴포넌트가 처리"한다. 속도를 0으로 강제하지 않는다.
- 서버 메시도 몽타주를 진행해야 하는 이유: 잡기 노티파이(`GrabWindow`)를 서버에서 받기 위해(`AlwaysTickPose`).
- 전용 서버에서는 연출(혀, 달리기/유령 걷기 몽타주, 효과음 등)을 생략한다(`NM_DedicatedServer` 검사).
- 시계: `GetServerTime()`은 `GameStateBase::GetServerWorldTimeSeconds()`를 쓴다. 서버 쿨다운 검사는 잡기에 한해 0.9 여유를 둔다.
- 꼬리 잡기 대결, 부쉬, 전화부스 상태는 모두 `CanUseActions()`를 통해 잡기/밀기를 막는다.

### 관련 함수
`ACTCharacter::CanMoveNow / CanUseActions / CanJumpInternal_Implementation / Move / Look / AddSpeedBuff / RemoveSpeedBuff / RecalculateMaxWalkSpeed / RefreshChaseBuff / OnGhostStateChanged / TryStartRun / StopRun / ForceResetRun / ApplyCameraSettings / OnRep_*`,
`UCTCharacterMovementComponent::GetGravityZ / GetMaxAcceleration`.
