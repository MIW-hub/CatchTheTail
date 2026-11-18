# 보물찾기 HUD 위쪽 세 칸을 패널 하나로 합침. 에디터를 끄고 실행.
import unreal
w = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTTreasureHudWidget")
unreal.log("[CT] 위쪽 패널: " + str(unreal.CTUIBuildTools.replace_treasure_top_panel(w)))
unreal.EditorAssetLibrary.save_loaded_asset(w)
