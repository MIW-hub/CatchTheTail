# 로비 경기 설정의 "게임 모드" 줄을 다른 줄(경기 시간)과 같은 모양으로 맞춥니다. 에디터를 끄고 실행.
import unreal
asset = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTLobbyWidget")
unreal.log("[CT] 모드 줄 모양 맞춤: " + str(unreal.CTUIBuildTools.copy_setting_row_style(asset, "GameMinutes", "Mode")))
unreal.EditorAssetLibrary.save_loaded_asset(asset)
