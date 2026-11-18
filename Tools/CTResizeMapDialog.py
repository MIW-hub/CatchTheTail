# 맵 설정 창: 경기 설정 칸 : 맵 목록 칸을 3:7 에서 4:6 으로 맞춤 (맵 목록을 줄임). 에디터를 끄고 실행.
import unreal
T = unreal.CTUIBuildTools
V = unreal.Vector2D
lobby = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTLobbyWidget")
Z = V(0, 0)
def place(name, x, y, w, h):
    print(name, T.set_canvas_slot_placement(lobby, name, Z, Z, Z, V(x, y), V(w, h)))
# 경기 설정 칸: 560 -> 720 / 맵 목록: 1200 -> 1060 (전체 폭은 그대로)
place("Image_MatchHeaderBg", 56, 138, 728, 56)
place("Label_MatchSettingsHeader", 76, 142, 700, 44)
place("Panel_MatchSettings", 60, 205, 720, 600)
place("Image_MapHeaderBg", 836, 138, 1068, 56)
place("Label_MapListHeader", 856, 142, 1040, 44)
place("Scroll_MapList", 840, 205, 1060, 530)
unreal.EditorAssetLibrary.save_loaded_asset(lobby)
