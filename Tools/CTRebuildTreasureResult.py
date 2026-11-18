import unreal
a = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTTreasureResultWidget")
unreal.CTUIBuildTools.build_treasure_result_design(a)
unreal.EditorAssetLibrary.save_loaded_asset(a)
