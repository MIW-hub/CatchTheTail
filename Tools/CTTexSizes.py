import unreal
out = []
for n in ["UI_Btn_Blue_L","UI_Btn_Blue_S1","UI_Btn_Blue_S2","UI_Btn_Blue_S3","UI_Btn_Blue_S4","UI_Btn_Pink_L","UI_Btn_Pink_M","UI_Btn_Pink_Slant","UI_Btn_White_L","UI_Btn_Yellow_L","UI_Btn_Yellow_M","UI_KeyButton_Small"]:
    for d in ["/Game/2D/UI/Style/","/Game/2D/UI/"]:
        t = unreal.EditorAssetLibrary.load_asset(d + n)
        if t:
            out.append(f"{n} {t.blueprint_get_size_x()}x{t.blueprint_get_size_y()}")
            break
open("C:/Unreal/CatchTheTail/Saved/tex_sizes.txt","w").write("\n".join(out))
