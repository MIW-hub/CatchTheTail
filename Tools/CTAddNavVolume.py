# 지정한 맵에 NavMeshBoundsVolume 을 맵 전체 크기로 넣습니다 (이미 있으면 건너뜀). 에디터를 끄고 실행.
# 사용: UnrealEditor-Cmd CatchTheTail.uproject -run=pythonscript -script="Tools/CTAddNavVolume.py" (맵 이름은 아래 MAP)
import unreal

MAP = "/Game/Levels/L_Box"

unreal.EditorLevelLibrary.load_level(MAP)
actors = unreal.EditorLevelLibrary.get_all_level_actors()

for a in actors:
    if isinstance(a, unreal.NavMeshBoundsVolume):
        unreal.log("[CT] 이미 NavMeshBoundsVolume 있음: " + a.get_name())
        raise SystemExit

lo = None
hi = None
for a in actors:
    if not isinstance(a, (unreal.StaticMeshActor, unreal.InstancedFoliageActor)):
        continue
    origin, extent = a.get_actor_bounds(False)
    mn = origin - extent
    mx = origin + extent
    if lo is None:
        lo, hi = mn, mx
    else:
        lo = unreal.Vector(min(lo.x, mn.x), min(lo.y, mn.y), min(lo.z, mn.z))
        hi = unreal.Vector(max(hi.x, mx.x), max(hi.y, mx.y), max(hi.z, mx.z))

if lo is None:
    unreal.log_error("[CT] 오류: 맵에서 메시를 찾지 못함")
else:
    center = (lo + hi) * 0.5
    size = hi - lo
    unreal.log("[CT] 맵 범위: 가운데 " + str(center) + ", 크기 " + str(size))
    vol = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.NavMeshBoundsVolume, center)
    # 볼륨 기본 크기는 한 변 200 (브러시 기준 100 단위 x2). 스케일로 맞춤
    vol.set_actor_scale3d(unreal.Vector(size.x / 200.0, size.y / 200.0, max(size.z, 800.0) / 200.0))
    unreal.EditorLevelLibrary.save_current_level()
    unreal.log("[CT] NavMeshBoundsVolume 추가 완료: " + MAP)
