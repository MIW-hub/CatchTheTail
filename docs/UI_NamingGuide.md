# UI별 위젯 이름표

UI(화면, 팝업) 하나씩 만들 수 있게 **UI마다 따로** 정리했습니다. 공용 이미지는 맨 끝 한 곳에 모았습니다.

## 규칙

- 코드는 위젯을 **이름과 종류가 모두 같을 때만** 연결합니다. 이름이 다르면 그 요소는 동작하지 않고 에러도 나지 않습니다.
- 위젯 블루프린트에서 **"변수인가" 체크**가 필요합니다. 대소문자도 같아야 합니다.
- 아래 표에 없는 위젯(장식 이미지, 제목 글자 등)은 이름 자유입니다.
- 표의 **"숨김 단위"**는 코드가 켜고 끄는 묶음입니다(아무 위젯이나 가능).

---

## A. 메인 메뉴 — `WBP_CTMainMenuWidget`

위에서 아래 순서(목업 기준)로 이름을 붙입니다.

| 목업 요소 | 이름 | 종류 |
|---|---|---|
| 이 화면 전체 묶음 | `Panel_Main` | 숨김 단위 |
| 오른쪽 위 내 이름 알약 | `Text_PlayerName` | 글자 |
| 빠른 참가 (노랑) | `Button_Start` | 버튼 |
| 방 참가 (파랑) | `Button_FindLobby` | 버튼 |
| 설정 (흰색) | `Button_Settings` | 버튼 |
| 게임 방법 (흰색) | `Button_HowTo` | 버튼 |
| 게임 종료 (분홍) | `Button_Quit` | 버튼 |
| 연결 상태 안내 | `Text_Status` | 글자 |

이 아래 B~E는 메인 메뉴 위젯 안에 **같이 들어 있는 팝업**입니다. 이 위젯 안에 만들면 됩니다.

## B. 방 참가 팝업 (메인 메뉴 안) — `Panel_Lobby`

| 목업 요소 | 이름 | 종류 |
|---|---|---|
| 팝업 전체 | `Panel_Lobby` | 숨김 단위 |
| 방 코드 입력칸 | `EditableTextBox_RoomCode` | 입력칸 |
| 코드로 입장 | `Button_JoinByCode` | 버튼 |
| 방 만들기 | `Button_Host` | 버튼 |
| 뒤로 | `Button_LobbyBack` | 버튼 |

## C. 게임 방법 팝업 (메인 메뉴 안) — `Panel_HowTo`

| 목업 요소 | 이름 | 종류 |
|---|---|---|
| 팝업 전체 | `Panel_HowTo` | 숨김 단위 |
| 닫기 | `Button_HowToBack` | 버튼 |

설명 글과 이미지는 이름 자유입니다. 내용은 [GameGuide.md](GameGuide.md)를 쓰면 됩니다.

## D. 설정 팝업 (메인 메뉴 안) — `Panel_Settings`

| 목업 요소 | 이름 | 종류 |
|---|---|---|
| 팝업 전체 | `Panel_Settings` | 숨김 단위 |
| 뒤로 | `Button_SettingsBack` | 버튼 |

설정 화면 내용은 아래 **F. 설정**을 이 안에 넣으면 됩니다.

## E. 종료 확인 팝업 (메인 메뉴 안) — `Panel_QuitConfirm`

| 목업 요소 | 이름 | 종류 |
|---|---|---|
| 팝업 전체 | `Panel_QuitConfirm` | 숨김 단위 |
| 종료 | `Button_QuitConfirm` | 버튼 |
| 취소 | `Button_QuitCancel` | 버튼 |

---

## F. 설정 — `WBP_CTSettingsWidget`

| 목업 요소 | 이름 | 종류 |
|---|---|---|
| 언어: 한국어 / English / 日本語 | `Button_LanguageKo` / `Button_LanguageEn` / `Button_LanguageJa` | 버튼 |
| 배경음 슬라이더 / "70%" | `Slider_Music` / `Text_MusicValue` | 슬라이더 / 글자 |
| 효과음 슬라이더 / "100%" | `Slider_Sfx` / `Text_SfxValue` | 슬라이더 / 글자 |
| 모든 소리 끄기 | `CheckBox_Mute` | 체크박스 |
| 마이크 사용 | `CheckBox_MicEnabled` | 체크박스 |
| 버튼 없이 말하기 | `CheckBox_MicAlwaysOn` | 체크박스 |
| 음성 볼륨 슬라이더 / 값 | `Slider_Voice` / `Text_VoiceValue` | 슬라이더 / 글자 |
| 마이크 감도 슬라이더 / 값 | `Slider_MicSensitivity` / `Text_MicSensitivityValue` | 슬라이더 / 글자 |
| 전체 화면 | `CheckBox_Fullscreen` | 체크박스 |
| 화면 흔들림 | `CheckBox_ScreenShake` | 체크박스 |
| 색 이름·도형 표시 | `CheckBox_TeamColors` | 체크박스 |
| 머리 위 표시 | `CheckBox_TeamMarker` | 체크박스 |
| 조작 방법 목록 (오른쪽) | `VerticalBox_Controls` | 세로 상자 (코드가 줄을 채움) |
| "게임 중에 쓰는 키입니다" | `Text_ControlsMessage` | 글자 |
| 내 이름 / 이름 변경 영역 | `Text_PlayerName` / `Panel_ChangeName` / `Row_NameChange` | 글자 / 숨김 단위 / 숨김 단위 |
| 이름 변경 열기 / 확인 | `Button_ChangeNameOpen` / `Button_ChangeName` | 버튼 |
| 새 이름 입력 / 결과 문구 | `EditableTextBox_NewName` / `Text_NameMessage` | 입력칸 / 글자 |
| 기본값으로 초기화 | `Button_Reset` | 버튼 |
| 닫기 | `Button_Close` | 버튼 |
| 상태 문구 | `Text_Message` | 글자 |

### F-1. 조작 키 한 줄 — `WBP_CTKeyBindEntryWidget`

| 목업 요소 | 이름 | 종류 |
|---|---|---|
| 동작 이름 ("이동", "점프") | `Text_ActionName` | 글자 |
| 바꿀 수 있는 키 배지 | `KeySelector` | 키 선택기 |
| 바꿀 수 없는 키 표시 (W A S D) | `Text_Keys` | 글자 |

---

## G. 대기 로비 — `WBP_CTLobbyWidget`

| 목업 요소 | 이름 | 종류 |
|---|---|---|
| 참가 플레이어 목록 | `Text_PlayerList` | 글자 (지금은 한 덩어리, 아래 "코드 필요" 참고) |
| 아래 "인원 1/1 · 준비 0명" | `Text_Status` | 글자 |
| 방 코드 "•••••" | `Text_RoomCode` | 글자 |
| 방 코드 보기 / 숨기기 | `Button_ToggleRoomCode` + `Text_ToggleRoomCodeLabel` | 버튼 + 글자 |
| 복사 / 복사 결과 | `Button_CopyRoomCode` + `Text_CopyResult` | 버튼 + 글자 |
| 준비 (방장은 숨김) | `Button_Ready` + `Text_ReadyLabel` | 버튼 + 글자 |
| 게임 시작 (방장) | `Button_Start` | 버튼 |
| 로비 나가기 | `Button_Leave` | 버튼 |
| 채팅 내용 스크롤 | `ScrollBox_Chat` + `Text_ChatLog` | 스크롤 상자 + 글자 |
| 채팅 입력 | `EditableTextBox_Chat` | 입력칸 |
| 선택된 맵 이미지 | `Image_MapPreview` | 이미지 |
| 선택된 맵 이름 | `Text_MapName` | 글자 |
| 맵 설명 (사용 안 하면 숨김) | `Text_MapDescription` | 글자 |
| "5분 · 라운드 60초 · 최대 8명" | `Text_MatchSettings` | 글자 |
| 맵 설정 열기 (방장) | `Button_SelectMap` | 버튼 |
| 맵 설정 팝업 전체 | `Panel_MapSelect` | 숨김 단위 (처음엔 접힘) |

**코드 필요 (이름만 맞춰서는 안 됨)**: 참가자 줄마다 왕관, 준비 상태를 보여 주려면 줄 위젯을 새로 만들어야 합니다. 참가 인원 "1 / 8" 글자도 새 칸이 필요합니다. 만드실 때 알려주시면 이름 규칙에 맞춰 코드를 추가하겠습니다.

## H. 맵 설정 팝업 (로비 위젯 안) — `Panel_MapSelect`

| 목업 요소 | 이름 | 종류 |
|---|---|---|
| 맵 목록이 들어가는 세로 상자 (스크롤 상자 안) | `Panel_MapList` | 세로 상자 |
| 닫기 | `Button_MapSelectClose` | 버튼 |

**경기 설정 줄**: 항목마다 아래 4개를 만들고, `<키>` 자리에 항목 키를 넣습니다.

| 항목 | `<키>` |
|---|---|
| 경기 시간 | `GameMinutes` |
| 라운드 시간 | `RoundSeconds` |
| 최대 인원 | `MaxPlayers` |
| 시작 꼬리 수 | `InitialTails` |
| 전환 쿨타임 | `PhoneUses` |
| 부쉬 | `Bush` |
| AI 인원 | `AiPlayers` |

| 줄의 요소 | 이름 | 종류 |
|---|---|---|
| 항목 이름 | `Label_Setting_<키>` | 글자 |
| 현재 값 | `Text_Setting_<키>` | 글자 |
| `<` 버튼 | `Button_Setting_<키>_Prev` | 버튼 |
| `>` 버튼 | `Button_Setting_<키>_Next` | 버튼 |

### H-1. 맵 항목 한 칸 — `WBP_CTMapEntryWidget`

| 목업 요소 | 이름 | 종류 |
|---|---|---|
| 칸 전체를 덮는 버튼 | `Button_Entry` | 버튼 |
| 맵 이미지 | `Image_Thumbnail` | 이미지 |
| 맵 이름 | `Text_MapName` | 글자 |
| 맵 설명 | `Text_MapDescription` | 글자 |
| 선택 표시 (노란 체크, 분홍 테두리) | `Widget_Selected` | 숨김 단위 (선택될 때만 보임) |

---

## I. ESC 메뉴 — `WBP_CTPauseMenuWidget`

| 목업 요소 | 이름 | 종류 |
|---|---|---|
| 메인 묶음 / 설정 묶음 / 확인 팝업 묶음 | `Panel_Main` / `Panel_Settings` / `Panel_Confirm` | 숨김 단위 |
| 계속하기 | `Button_Resume` | 버튼 |
| 설정 | `Button_Settings` | 버튼 |
| 기권 | `Button_Forfeit` | 버튼 |
| 기권 안내 문구 | `Text_ForfeitInfo` | 글자 |
| 게임 종료 | `Button_Quit` | 버튼 |
| 확인 팝업 제목 / 내용 | `Text_ConfirmTitle` / `Text_ConfirmBody` | 글자 |
| 확인 팝업 예 / 아니오 | `Button_ConfirmYes` / `Button_ConfirmNo` | 버튼 |

## J. 로딩 — `WBP_CTLoadingWidget`

| 목업 요소 | 이름 | 종류 |
|---|---|---|
| 참가 인원 | `Text_PlayerCount` | 글자 |
| "로딩 중" 문구 | `Text_Loading` | 글자 |
| 게임 설명 팁 | `Text_Tip` | 글자 |
| 진행 막대 | `ProgressBar_Loading` | 진행 막대 |

## K. 결과 — `WBP_CTResultWidget`

| 목업 요소 | 이름 | 종류 |
|---|---|---|
| 제목 ("결과") | `Text_ResultTitle` | 글자 |
| 승자 이름 | `Text_Winners` | 글자 |
| 순위표 줄이 들어가는 곳 | `VerticalBox_Ranking` | 세로 상자 |
| 대기 로비로 | `Button_ReturnToLobby` | 버튼 |
| 메뉴로 나가기 | `Button_LeaveToMenu` | 버튼 |
| 복귀 안내 문구 | `Text_ReturnHint` | 글자 |

## L. 언어 선택 (처음 실행) — `WBP_CTLanguageSelectWidget`

| 목업 요소 | 이름 | 종류 |
|---|---|---|
| 한국어 / English / 日本語 | `Button_LanguageKo` / `Button_LanguageEn` / `Button_LanguageJa` | 버튼 |
| 확인 | `Button_Confirm` | 버튼 |

## M. 코드로 만들어지는 UI (위젯 블루프린트 없음)

다음은 현재 코드가 모양까지 만들어서 이름표가 필요 없습니다. 같은 스타일로 바꾸려면 말씀해 주세요.

- 전화부스 팝업 (`UCTPhonePopupWidget`): 좌클릭 목표, 우클릭 헌터 확인
- 로비 마이크 상태 (`UCTMicIndicatorWidget`): 오른쪽 아래 마이크 패널

---

## 공용 이미지 (임포트 완료)

위치: `/Game/2D/UI/Style/` (아이콘은 `/Game/2D/UI/ico/`). 9-slice 마진은 위젯 이미지 브러시에서 지정합니다.

**창틀** (제목줄 + 흰 본문 한 장, 9-slice 마진 위/좌우/아래)
- `UI_Frame_Pink_L` 92/24/20 (큰 창) · `UI_Frame_Pink_M` 73/24/20 · `UI_Frame_Pink_S` 61/20/16 · `UI_Frame_Pink_XS` 51/20/14
- `UI_Frame_Blue` 52/20/14 (로비 채팅, 조작 방법)
- `UI_Frame_Outline`: 속이 투명한 외곽선 틀

**버튼, 알약**: `UI_Btn_Yellow_L/M`, `UI_Btn_Blue_L`, `UI_Btn_White_L`, `UI_Btn_Pink_L/M`, `UI_Btn_Pink_Slant`, `UI_Btn_Blue_S1~S4`, `UI_Pill_Status`, `UI_Row_Pink`, `UI_Field_Label`

**입력, 스크롤**: `UI_Input_Field`, `UI_Scroll_Track/Thumb`, `UI_Scroll_Track_Pink/Thumb_Pink`

**아이콘** (흰색 단색, 색은 위젯에서): `Ico_Gamepad`, `Ico_Group`, `Ico_Person`, `Ico_Return`, `Ico_Chat`

**아직 없음**: 핀 아이콘, 체크 표시, 메인/설정 배경, 로고 (슬라이더, 체크박스, 어두운 막은 WBP 박스로)
