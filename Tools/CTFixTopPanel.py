import unreal
T = unreal.CTUIBuildTools
V = unreal.Vector2D
Z = V(0, 0)
w = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTTreasureHudWidget")
def place(name, x, y, sx, sy, align=Z):
    T.set_canvas_slot_placement(w, name, Z, Z, align, V(x, y), V(sx, sy))
place("Image_TopPanelFrame", 0, 22, 580, 112)
place("Image_MyWinsCell", 12, 34, 172, 88)
place("Image_EnemyWinsCell", 396, 34, 172, 88)
place("Image_MyTeamColor", 24, 78, 30, 30)
place("Image_EnemyTeamColor", 408, 78, 30, 30)
place("Label_MyTeamWins", 98, 52, 160, 22, V(0.5, 0.5))
place("Label_EnemyTeamWins", 482, 52, 160, 22, V(0.5, 0.5))
place("Text_MyTeamWins", 68, 66, 100, 48)
place("Text_EnemyTeamWins", 452, 66, 100, 48)
dark = unreal.LinearColor(0.2, 0.06, 0.24, 1.0)
for n in ["Text_MyTeamWins", "Text_EnemyTeamWins"]:
    T.set_text_appearance(w, n, dark, 34, unreal.LinearColor(0, 0, 0, 0), 0)
# 묶음 크기
g = T.set_canvas_slot_placement(w, "Group_TopPanel", V(0.5, 0), V(0.5, 0), V(0.5, 0), V(0, 20), V(580, 136))
unreal.EditorAssetLibrary.save_loaded_asset(w)
