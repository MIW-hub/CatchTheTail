import unreal
T = unreal.CTUIBuildTools
V = unreal.Vector2D
Z = V(0, 0)
def place(w, name, x, y, sx, sy, align=Z):
    T.set_canvas_slot_placement(w, name, Z, Z, align, V(x, y), V(sx, sy))
h = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTTreasureHudWidget")
T.scale_text_sizes(h, 1.2, 20, "Text_RoundTimer")
place(h, "Text_RoundTimer", 290, 72, 170, 50, V(0.5, 0.5))
T.scale_text_sizes(h, 1.43, 14, "Label_TeamEnergy")
T.scale_text_sizes(h, 1.43, 14, "Label_MyEnergy")
# 에너지 칸 라벨: 아이콘과 숫자 탭 사이 가운데에 글자를 가운데 정렬
for n in ["Label_TeamEnergy", "Label_MyEnergy"]:
    T.set_text_justification_by_name(h, n, 1)
    place(h, n, 112, 36, 110, 30, V(0.5, 0.5))
unreal.EditorAssetLibrary.save_loaded_asset(h)
r = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTRoundBreakWidget")
T.scale_text_sizes(r, 1.22, 24, "Text_RoundTitle")
unreal.EditorAssetLibrary.save_loaded_asset(r)
t = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTTreasureResultWidget")
T.scale_text_sizes(t, 1.12, 24, "Text_TreasureResultTitle")
unreal.EditorAssetLibrary.save_loaded_asset(t)
