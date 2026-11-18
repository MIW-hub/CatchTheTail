import unreal
T = unreal.CTUIBuildTools
def scale(asset, factor, minsize, contains=""):
    a = unreal.EditorAssetLibrary.load_asset("/Game/UI/" + asset)
    n = T.scale_text_sizes(a, factor, minsize, contains)
    unreal.EditorAssetLibrary.save_loaded_asset(a)
    open("C:/Unreal/CatchTheTail/Saved/scale_log.txt", "a", encoding="utf-8").write(f"{asset} {contains} x{factor}: {n}\n")
open("C:/Unreal/CatchTheTail/Saved/scale_log.txt", "w").close()
scale("WBP_CTTreasureHudWidget", 0.75, 13, "Label_")
scale("WBP_CTRoundBreakWidget", 0.85, 14)
scale("WBP_CTRoundBreakRowWidget", 0.9, 14)
scale("WBP_CTTreasureResultWidget", 0.85, 14)
scale("WBP_CTTeamSwapRequestWidget", 0.9, 14)
scale("WBP_CTLobbyWidget", 0.85, 12, "Text_MatchSettings")
