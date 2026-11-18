import unreal
sc = unreal.load_asset("/Game/sound/SC_CT_Sfx")
for n in ["SFX_Energy_Pick","SFX_Throw","SFX_Throw_Hit","SFX_Round_Break"]:
    a = unreal.load_asset("/Game/sound/sfx/" + n)
    a.set_editor_property("sound_class_object", sc)
    unreal.EditorAssetLibrary.save_loaded_asset(a)
    unreal.log("[CT] 음량 종류 지정: " + n)
