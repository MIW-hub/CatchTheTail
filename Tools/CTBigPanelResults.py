import unreal
T = unreal.CTUIBuildTools
for n, fn in [("WBP_CTRoundBreakWidget", T.build_round_break_design), ("WBP_CTTreasureResultWidget", T.build_treasure_result_design)]:
    a = unreal.EditorAssetLibrary.load_asset("/Game/UI/" + n)
    fn(a)
    unreal.EditorAssetLibrary.save_loaded_asset(a)
