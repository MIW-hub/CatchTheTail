import unreal
T = unreal.CTUIBuildTools
V = unreal.Vector2D
Z = V(0, 0)
C = V(0.5, 0.5)
def place(w, name, x, y, sx, sy):
    T.set_canvas_slot_placement(w, name, Z, Z, C, V(x, y), V(sx, sy))
h = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTTreasureHudWidget")
place(h, "Text_MyTeamScore", 222, 80, 100, 46)
place(h, "Text_EnemyTeamScore", 578, 80, 100, 46)
place(h, "Label_MyTeamScore", 202, 46, 150, 22)
place(h, "Label_EnemyTeamScore", 558, 46, 150, 22)
place(h, "Text_MyTeamWins", 52, 84, 90, 56)
place(h, "Text_EnemyTeamWins", 708, 84, 90, 56)
place(h, "Label_MyTeamWins", 52, 44, 90, 22)
place(h, "Label_EnemyTeamWins", 708, 44, 90, 22)
unreal.EditorAssetLibrary.save_loaded_asset(h)
