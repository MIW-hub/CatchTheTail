# 메인 메뉴 게임 방법 패널을 8줄로 늘리고 뒤로가기 버튼에 아래 여백을 줌, 키 설정 버튼(키 이름 칸)을 넓혀 글자가 잘리지 않게 함. 에디터를 끄고 실행.
import unreal
T = unreal.CTUIBuildTools
V = unreal.Vector2D
EAL = unreal.EditorAssetLibrary

# 게임 방법
menu = EAL.load_asset("/Game/UI/WBP_CTMainMenuWidget")
LINES = [
    "01   내 색상과 목표 색상을 확인하세요",
    "02   목표 색상 플레이어의 꼬리를 노리세요",
    "03   잡은 뒤에는 연타해서 꼬리를 차지하세요",
    "04   달리기와 밀치기 게이지를 확인하세요",
    "05   보물찾기는 4대4 팀전, 에너지를 많이 모으세요",
    "06   오브젝트를 던져 상대의 에너지를 흩뜨리세요",
    "07   자동차, 통나무, 드럼통을 조심하세요",
    "08   꼬리 수와 실시간 순위를 확인하세요",
]
C = V(0.5, 0.5)
for i, text in enumerate(LINES):
    name = "Label_HowTo%d" % (i + 1)
    if i < 4:
        print(name, T.set_text_by_name(menu, name, text))
    else:
        print(name, T.clone_text_block(menu, "Label_HowTo1", name, text))
    print(" place", T.set_canvas_slot_placement(menu, name, C, C, V(0.5, 0.0), V(0, -262 + 56 * i), V(900, 52)))
# 뒤로가기 버튼: 카드 아래 끝(350)에서 58 띄움
print("back", T.set_canvas_slot_placement(menu, "Button_HowToBack", C, C, V(0, 0), V(-230, 200), V(460, 92)))
EAL.save_loaded_asset(menu)

# 키 설정 줄: 키 이름 버튼을 넓히고 줄 안(600)에 들어오게 함
entry = EAL.load_asset("/Game/UI/WBP_CTKeyBindEntryWidget")
Z = V(0, 0)
print("selector", T.set_canvas_slot_placement(entry, "KeySelector", Z, Z, Z, V(296, 7), V(290, 50)))
print("name", T.set_canvas_slot_placement(entry, "Text_ActionName", Z, Z, Z, V(20, 8), V(270, 36)))
EAL.save_loaded_asset(entry)
unreal.log("[CT] MENU_UI_DONE")
