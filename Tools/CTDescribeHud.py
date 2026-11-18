import unreal
a = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTTreasureHudWidget")
open("C:/Unreal/CatchTheTail/Saved/hud_tree.txt","w",encoding="utf-8").write(unreal.CTUIBuildTools.describe_widget_tree(a))
