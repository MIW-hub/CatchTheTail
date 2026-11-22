# 메인 메뉴에 크레딧 버튼(게임 방법 아래)과 크레딧 화면(검은 바탕 + 위로 올라가는 글)을 만듭니다. 에디터를 끄고 실행.
# 글을 바꾸려면 ENTRIES 만 고쳐서 다시 실행 ("# " 큰 제목, "## " 항목 이름, "" 빈 간격). 다시 실행해도 안전함(지우고 다시 만듦).
import unreal
T = unreal.CTUIBuildTools
EAL = unreal.EditorAssetLibrary
menu = EAL.load_asset("/Game/UI/WBP_CTMainMenuWidget")

ENTRIES = [
    "# HEY CHICKEN!",
    "",
    "## 기획",
    "문인우",
    "",
    "## 프로그래밍",
    "문인우",
    "",
    "## UI 디자인",
    "문인우",
    "",
    "## 레벨 디자인",
    "문인우",
    "",
    "## 사운드",
    "문인우",
    "",
    "## 제작",
    "Moon Party Games",
    "",
    "## 게임 엔진",
    "Unreal Engine",
    "© Epic Games, Inc.",
    "",
    "## 폰트",
    "Noto Sans KR · Noto Sans JP",
    "",
    "## Special Thanks",
    "플레이해 주신 모든 분들",
    "",
    "",
    "# 플레이해 주셔서 감사합니다",
    "© 2026 Moon Party Games",
]

# 이전에 만든 크레딧 위젯 지우기
old = ["Label_CreditsLine%d" % i for i in range(1, 80)] + ["Label_Credits%d" % i for i in range(1, 12)]
old += ["Label_CreditsTitle", "Label_Button_CreditsBack", "Button_CreditsBack", "Label_Button_Credits", "Button_Credits",
        "Image_CreditsCard", "Image_CreditsBackdrop", "Panel_CreditsScroll", "Panel_Credits"]
for n in old:
    T.remove_widget_by_name(menu, n)

# 메인 버튼 줄: 크레딧이 들어갈 자리를 만들려고 간격을 줄임 (게임 방법 아래, 게임 종료 위)
V = unreal.Vector2D
C = V(0.5, 0.5)
for name, y in (("Button_Start", -60), ("Button_FindLobby", 30), ("Button_Settings", 120), ("Button_HowTo", 210), ("Button_Quit", 390)):
    T.set_canvas_slot_placement(menu, name, C, C, V(0, 0), V(-230, y), V(461, 84))

ok = T.build_credits_panel(menu, ENTRIES, "크레딧")
T.sync_widget_guids(menu)
saved = EAL.save_loaded_asset(menu)
open("C:/Unreal/CatchTheTail/Saved/credits_result.txt", "w").write("build=%s saved=%s" % (ok, saved))
