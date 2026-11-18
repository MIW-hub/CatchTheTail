# 로비 플레이어 패널을 키우고(왼쪽 영역 약 4 : 6), 팀 변경 버튼을 패널 오른쪽 가장자리에 붙임. 에디터를 끄고 실행.
import unreal
T = unreal.CTUIBuildTools
V = unreal.Vector2D
lobby = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTLobbyWidget")

# 영역: 플레이어 패널은 폭 0.40, 높이 0.12~0.62 / 맵 패널은 바로 아래 0.62~0.87
print(T.set_canvas_slot_placement(lobby, "Zone_Players", V(0, 0.12), V(0.40, 0.62), V(0, 0), V(16, 16), V(16, 16)))
print(T.set_canvas_slot_placement(lobby, "Zone_Map", V(0, 0.62), V(0.40, 0.87), V(0, 0), V(16, 16), V(16, 16)))

# 패널 안쪽: 폭을 680 -> 780 으로 넓히고(줄이 더 시원하게), 축소 한도도 같이
print(T.set_size_box_override(lobby, "Native_Players", 780, 580))
print(T.set_size_box_override(lobby, "Cap_Players", 780, 580))
print(T.set_canvas_slot_placement(lobby, "Image_LobbyPlayersPanel", V(0, 0), V(0, 0), V(0, 0), V(0, 0), V(780, 580)))
print(T.set_canvas_slot_placement(lobby, "Text_PlayerCount", V(0, 0), V(0, 0), V(0, 0), V(540, 2), V(200, 44)))
print(T.set_canvas_slot_placement(lobby, "Scroll_LobbyPlayers", V(0, 0), V(0, 0), V(0, 0), V(20, 88), V(740, 476)))
print(T.set_canvas_slot_placement(lobby, "Panel_TeamLists", V(0, 0), V(0, 0), V(0, 0), V(20, 88), V(740, 476)))

# 팀 변경 버튼: 플레이어 패널 영역의 오른쪽 위 가장자리에 붙임
pass
unreal.EditorAssetLibrary.save_loaded_asset(lobby)
