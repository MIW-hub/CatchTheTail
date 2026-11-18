# 보물찾기 HUD 위젯(WBP_CTTreasureHudWidget)을 기본 HUD 이미지로 다시 만듭니다 (지금 들어 있는 뼈대는 지워짐).
# 실행: UnrealEditor-Cmd.exe CatchTheTail.uproject -run=pythonscript -script=ToolsCTRebuildTreasureHud.py  (에디터를 끄고)
import unreal

asset = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTTreasureHudWidget")
unreal.log(f"[CT] 보물찾기 HUD 뼈대: {unreal.CTUIBuildTools.build_treasure_hud_design(asset)}")
unreal.EditorAssetLibrary.save_loaded_asset(asset)
