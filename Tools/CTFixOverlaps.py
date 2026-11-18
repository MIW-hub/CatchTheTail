import unreal
T = unreal.CTUIBuildTools
h = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTTreasureHudWidget")
T.scale_text_sizes(h, 0.82, 12, "Label_TeamEnergy")
T.scale_text_sizes(h, 0.82, 12, "Label_MyEnergy")
unreal.EditorAssetLibrary.save_loaded_asset(h)
r = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTRoundBreakWidget")
for n in ["Size_TeamColor_My", "Size_TeamColor_Enemy"]:
    T.set_horizontal_slot_padding(r, n, 0, 0, 22, 0)
for n in ["Size_Crown_My", "Size_Crown_Enemy"]:
    T.set_horizontal_slot_padding(r, n, 0, 0, 10, 0)
unreal.EditorAssetLibrary.save_loaded_asset(r)
l = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTLobbyWidget")
T.scale_text_sizes(l, 0.93, 12, "Text_MatchSettings")
unreal.EditorAssetLibrary.save_loaded_asset(l)
