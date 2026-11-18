# 라운드 결과 팝업: 팀 이름 칸을 넓힘 (영어 "Enemy Team" 처럼 긴 글자가 숫자를 가리지 않게). 에디터를 끄고 실행.
import unreal
T = unreal.CTUIBuildTools
w = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTRoundBreakWidget")
for n in ["Size_TeamLabel_My", "Size_TeamLabel_Enemy"]:
    print(n, T.set_size_box_override(w, n, 230, 0))
unreal.EditorAssetLibrary.save_loaded_asset(w)
