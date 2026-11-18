# 로비 경기 설정 줄: 값 칸을 넓히고 화살표 버튼도 넓게(언어가 바뀌어 글자가 길어져도 들어가게). 에디터를 끄고 실행.
import unreal
T = unreal.CTUIBuildTools
lobby = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTLobbyWidget")
for key in ["GameMinutes", "RoundSeconds", "MaxPlayers", "InitialTails", "PhoneUses", "Bush", "AiPlayers", "Mode"]:
    print(key, T.set_size_box_override(lobby, f"Size_Text_Setting_{key}", 250, 56),
          T.set_size_box_override(lobby, f"Size_Button_Setting_{key}_Prev", 72, 56),
          T.set_size_box_override(lobby, f"Size_Button_Setting_{key}_Next", 72, 56))
unreal.EditorAssetLibrary.save_loaded_asset(lobby)
