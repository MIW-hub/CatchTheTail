# 세 맵의 후처리 볼륨에 M_PP_SoftOutline 을 넣습니다 (볼륨이 없으면 맵 전체용 볼륨을 새로 만듦). 에디터를 끄고 실행.
# 되돌리려면 REMOVE = True 로 바꿔 다시 실행.
import unreal

REMOVE = False
MAPS = ["/Game/Levels/L_Box", "/Game/Levels/L_Country", "/Game/Levels/L_Building", "/Game/Levels/L_Lobby"]
MAT_PATH = "/Game/Material/M_PP_SoftOutline.M_PP_SoftOutline"
mat = unreal.EditorAssetLibrary.load_asset(MAT_PATH)

for m in MAPS:
    unreal.EditorLevelLibrary.load_level(m)
    vols = [a for a in unreal.EditorLevelLibrary.get_all_level_actors() if isinstance(a, unreal.PostProcessVolume)]
    unreal.log("[CT] %s: 후처리 볼륨 %d개 %s" % (m, len(vols), [v.get_name() + (" (전체)" if v.get_editor_property("unbound") else "") for v in vols]))
    if not vols and not REMOVE:
        v = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
        v.set_editor_property("unbound", True)
        v.set_actor_label("PP_SoftOutline")
        vols = [v]
        unreal.log("[CT] %s: 전체용 후처리 볼륨을 새로 만듦" % m)
    for v in vols:
        settings = v.get_editor_property("settings")
        wb = settings.get_editor_property("weighted_blendables")
        arr = list(wb.get_editor_property("array"))
        arr = [x for x in arr if not (x.get_editor_property("object") and x.get_editor_property("object").get_name() == "M_PP_SoftOutline")]
        if not REMOVE:
            item = unreal.WeightedBlendable()
            item.set_editor_property("weight", 1.0)
            item.set_editor_property("object", mat)
            arr.append(item)
        wb.set_editor_property("array", arr)
        settings.set_editor_property("weighted_blendables", wb)
        v.set_editor_property("settings", settings)
        v.set_editor_property("enabled", True)
        unreal.log("[CT] %s: %s 블렌더블 %d개" % (m, v.get_name(), len(arr)))
    unreal.EditorLevelLibrary.save_current_level()
unreal.log("[CT] APPLY_DONE")
