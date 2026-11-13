# 🎨 보물찾기 HUD 디자인 가이드

> 보물찾기 모드 HUD는 `WBP_CTTreasureHudWidget`에서 디자인합니다. 지금 들어 있는 위젯은 **임시 뼈대**이고, 모양은 마음대로 바꾸셔도 됩니다.
> 값을 채우는 코드는 `UI/CTTreasureHudWidget.h/.cpp`입니다. 아래 **이름**으로 위젯을 만들어 두면 자동으로 값이 채워지고, 이름이 없는 위젯은 건너뜁니다(모두 선택 사항).

## 1. 미리보기
- 게임을 실행한 뒤 콘솔에서 `CTPreviewTreasureHud`를 입력하면 보물찾기 HUD가 보이고(기본 HUD는 숨김), 한 번 더 입력하면 원래대로 돌아옵니다.
- 이 명령은 내 화면에서만 바뀌고 게임 규칙에는 영향이 없습니다. 보물찾기 모드가 아직 시작되지 않아서 값은 기본값(0, 1 / 3, 팀 색 없음)으로 보입니다.
- 모드가 실제로 보물찾기가 되면(규칙 구현 후) 같은 HUD가 자동으로 나타납니다.

## 2. 값이 채워지는 위젯 이름

| 위젯 이름 | 종류 | 채워지는 값 |
|---|---|---|
| `Text_RoundNumber` | TextBlock | `1 / 3` 형식의 라운드 번호 |
| `Text_RoundTimer` | TextBlock | 라운드 남은 시간 `1:00` |
| `Text_MyTeamWins` | TextBlock | 우리 팀 라운드 승점 |
| `Text_EnemyTeamWins` | TextBlock | 상대 팀 라운드 승점 |
| `Image_MyTeamColor` | Image | 우리 팀 색 (Color and Opacity에 색이 입혀짐) |
| `Image_EnemyTeamColor` | Image | 상대 팀 색 |
| `Text_MyEnergy` | TextBlock | 내 에너지 (본인 화면에만 값이 옴) |
| `Text_TeamEnergy` | TextBlock | 우리 팀 에너지 (지금은 `-` 표시, 구현 때 연결) |

- **상대 팀 에너지는 표시하지 않습니다.** 서버가 알려 주지 않는 값이라 위젯을 만들어도 채울 수 없습니다(기획의 "상대 꼬리 길이로 예측" 규칙).
- 그 밖의 이름(`Text_RoundLabel`, `Label_MyEnergy` 등)은 임시 뼈대에 들어 있는 고정 글자이고 코드와 상관없습니다. 자유롭게 지우거나 바꿀 수 있습니다.
- 팀 색 칩의 모양(둥근 칩)은 `Image_MyTeamColor`의 Brush로 정합니다. 색은 Color and Opacity로 입히므로 **Brush의 Tint는 흰색으로 두세요.**

## 3. 연출용 블루프린트 이벤트 (WBP 그래프에서 이벤트로 추가)
| 이벤트 | 언제 | 쓰임새 |
|---|---|---|
| `BP_OnMyEnergyChanged(NewEnergy, Delta)` | 내 에너지가 바뀔 때 | 먹었을 때 숫자 튀는 연출 (Delta > 0 먹음, Delta < 0 맞아서 떨어뜨림) |
| `BP_OnRoundWinsChanged(MyTeamWins, EnemyTeamWins)` | 라운드 승점이 바뀔 때 | 점수 올라가는 연출 |
| `BP_OnRoundChanged(NewRoundNumber)` | 새 라운드가 시작될 때 | "ROUND 2" 같은 라운드 시작 연출 |

## 4. 설정 값 (WBP의 클래스 기본값)
- `TotalRounds` (기본 3): `1 / 3`의 3. 라운드 수가 바뀌면 여기서 조정합니다.
- `FallbackTeamColor`: 팀 색을 아직 모를 때 칩에 입히는 색.

## 5. 다른 화면 (결과, 로비)에서 모드별로 나누는 방법
- 모든 `UCTUserWidget` 기반 위젯(HUD, 결과, 점수 줄 등)에서 블루프린트 함수 **`Get Mode Type`**(`ECTGameModeType`: 꼬리잡기 / 보물찾기)을 호출할 수 있습니다.
- 결과 화면(`WBP_CTResultWidget`)은 결과를 채우기 전에 **`BP_OnModeKnown(Mode)`** 이벤트를 한 번 부릅니다. 이 이벤트에서 모드에 따라 패널을 켜고 끄면 됩니다.
- 기존 꼬리잡기 HUD와 결과 화면은 건드리지 않았고, 꼬리잡기에서는 지금과 똑같이 나옵니다.

## 6. 주의
- 이 WBP 이름과 위젯 이름은 코드와 연결되어 있어서 **이름을 바꾸면 값이 채워지지 않습니다.** 바꾸고 싶으면 이름을 그대로 두고 모양만 바꾸거나 알려 주세요.
- HUD는 3/4 크기 배율(DPI 곡선)이 적용된 화면에서 보이므로, 에디터 미리보기보다 게임에서 작게 보일 수 있습니다.
