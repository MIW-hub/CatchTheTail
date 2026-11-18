# 로비 플레이어 목록 정리: 줄에 팀 색 칩 추가, 줄 높이 축소, 팀 변경 버튼을 오른쪽 파란 버튼으로. 에디터를 끄고 실행.
import unreal
entry = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTLobbyPlayerEntryWidget")
unreal.log("[CT] 줄 정리: " + str(unreal.CTUIBuildTools.upgrade_lobby_player_entry(entry)))
unreal.EditorAssetLibrary.save_loaded_asset(entry)
lobby = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTLobbyWidget")
unreal.log("[CT] 버튼 이동: " + str(unreal.CTUIBuildTools.move_lobby_change_team_button(lobby)))
unreal.EditorAssetLibrary.save_loaded_asset(lobby)
