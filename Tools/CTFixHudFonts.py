import unreal
T = unreal.CTUIBuildTools
V = unreal.Vector2D
Z = V(0, 0)
def place(w, name, x, y, sx, sy, align=Z):
    T.set_canvas_slot_placement(w, name, Z, Z, align, V(x, y), V(sx, sy))
h = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTTreasureHudWidget")
# 위쪽 패널: 점수와 시간을 위로, 시간 글자는 작게
T.scale_text_sizes(h, 0.75, 22, "Text_RoundTimer")
place(h, "Text_RoundTimer", 290, 66, 170, 44, V(0.5, 0.5))
place(h, "Text_MyTeamWins", 68, 54, 100, 48)
place(h, "Text_EnemyTeamWins", 452, 54, 100, 48)
place(h, "Label_MyTeamWins", 98, 42, 160, 22, V(0.5, 0.5))
place(h, "Label_EnemyTeamWins", 482, 42, 160, 22, V(0.5, 0.5))
place(h, "Image_MyTeamColor", 24, 64, 30, 30)
place(h, "Image_EnemyTeamColor", 408, 64, 30, 30)
# 에너지 칸: 라벨을 짧게(팀 / 나), 글자 작게, 가운데쪽으로
T.set_text_by_name(h, "Label_TeamEnergy", "팀")
T.set_text_by_name(h, "Label_MyEnergy", "나")
dark = unreal.LinearColor(1, 1, 1, 1)
for n in ["Group_TeamEnergy", "Group_MyEnergy"]:
    pass
place(h, "Label_TeamEnergy", 76, 22, 90, 28)
place(h, "Label_MyEnergy", 76, 22, 90, 28)
unreal.EditorAssetLibrary.save_loaded_asset(h)
# 제목 글자 줄이기
r = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTRoundBreakWidget")
T.scale_text_sizes(r, 0.8, 24, "Text_RoundTitle")
unreal.EditorAssetLibrary.save_loaded_asset(r)
t = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTTreasureResultWidget")
T.scale_text_sizes(t, 0.8, 24, "Text_TreasureResultTitle")
unreal.EditorAssetLibrary.save_loaded_asset(t)
