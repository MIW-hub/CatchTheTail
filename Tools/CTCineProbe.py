# L_Box 의 PlayerStart 위치와 주요 액터 범위를 로그로 출력 (시네마틱 경로 정하는 용도). 에디터를 끄고 실행.
import unreal
unreal.EditorLevelLibrary.load_level("/Game/Levels/L_Box")
acts = unreal.EditorLevelLibrary.get_all_level_actors()
starts = [a for a in acts if isinstance(a, unreal.PlayerStart)]
unreal.log("[CT] PlayerStart %d개" % len(starts))
for a in starts:
    l = a.get_actor_location(); r = a.get_actor_rotation()
    unreal.log("[CT] START %s %.0f %.0f %.0f yaw %.0f" % (a.get_name(), l.x, l.y, l.z, r.yaw))
n = 0
for a in acts:
    if isinstance(a, unreal.StaticMeshActor):
        o, e = a.get_actor_bounds(False)
        if e.x > 600 or e.y > 600:
            unreal.log("[CT] BIG %s %.0f %.0f %.0f ext %.0f %.0f %.0f" % (a.get_name(), o.x, o.y, o.z, e.x, e.y, e.z)); n += 1
            if n > 25: break
unreal.log("[CT] PROBE_DONE")
