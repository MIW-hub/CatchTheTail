import unreal
a = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTRoundBreakWidget")
unreal.CTUIBuildTools.build_round_break_design(a)
unreal.EditorAssetLibrary.save_loaded_asset(a)
