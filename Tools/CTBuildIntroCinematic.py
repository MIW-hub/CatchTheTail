# 인트로/홍보용 시네마틱 준비: 큐브 시티(L_Box) 복사본 L_Cinematic 에 연출 감독(ACTCinematicDirector)을 놓습니다. 에디터를 끄고 실행.
# 컷: 1) 아무도 없는 큐브 시티 전경  2) 흰 닭이 달려감(옆)  3) 초록 닭이 쫓아감(옆)  4) 정면에서 잡히는 순간까지
# 영상 만들기는 Tools/CTRenderIntroCinematic.ps1
import unreal

EAL = unreal.EditorAssetLibrary
ELL = unreal.EditorLevelLibrary
SRC = "/Game/Levels/L_Box"
DST = "/Game/Levels/L_Cinematic"
SEQ_DIR = "/Game/Cinematic"
SEQ_NAME = "LS_IntroCinematic"
FPS = 30
TOTAL_SEC = 16.0  # 컷 길이 합 (2.5 + 2.7 + 2.8 + 8.0)

if not EAL.does_asset_exist(DST):
    EAL.duplicate_asset(SRC, DST)
ELL.load_level(DST)
world = ELL.get_editor_world()

for a in ELL.get_all_level_actors():
    if a.get_actor_label().startswith("CINE_"):
        a.destroy_actor()

# 시네마틱용 빈 게임 모드 (경기 로직 없이 관전자만)
GM_PATH = SEQ_DIR + "/BP_CineGameMode"
if not EAL.does_asset_exist(GM_PATH):
    fac = unreal.BlueprintFactory()
    fac.set_editor_property("parent_class", unreal.GameModeBase)
    gm_bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset("BP_CineGameMode", SEQ_DIR, unreal.Blueprint, fac)
    cdo = unreal.get_default_object(gm_bp.generated_class())
    cdo.set_editor_property("default_pawn_class", unreal.SpectatorPawn)
    cdo.set_editor_property("hud_class", unreal.HUD)
    EAL.save_loaded_asset(gm_bp)
world.get_world_settings().set_editor_property("default_game_mode", EAL.load_asset(GM_PATH).generated_class())

# 박스에 안 걸리는 일직선 길 찾기
def hit(a, b, radius):
    r = unreal.SystemLibrary.sphere_trace_single(world, a, b, radius, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
    if isinstance(r, tuple):
        return bool(r[0]) if isinstance(r[0], bool) else bool(r[-1])
    return bool(r)

LANE_LEN = 3800.0
best = None
for y in range(-3500, 3501, 250):
    for x0 in range(-5200, 1500, 250):
        if hit(unreal.Vector(x0, y, 160), unreal.Vector(x0 + LANE_LEN, y, 160), 110.0):
            continue
        score = abs(x0 + LANE_LEN / 2 + 1170) + abs(y + 140)
        if best is None or score < best[0]:
            best = (score, x0, y)
if best is None:
    raise Exception("[CT] 오류: 비어 있는 길을 찾지 못함")
_, XA, LY = best

SIDE_SIGN = -1.0
for sign in (-1.0, 1.0):
    if not hit(unreal.Vector(XA + 200, LY + sign * 800, 140), unreal.Vector(XA + 2900, LY + sign * 800, 140), 110.0):
        SIDE_SIGN = sign
        break

GZ = 45.0
BP = EAL.load_blueprint_class("/Game/Character/Player/BP_CTCharacter")
tmp = ELL.spawn_actor_from_class(BP, unreal.Vector(0, 0, 5000))
HH = tmp.get_editor_property("capsule_component").get_scaled_capsule_half_height()
tmp.destroy_actor()

director = ELL.spawn_actor_from_class(unreal.CTCinematicDirector, unreal.Vector(XA, LY, 0))
director.set_actor_label("CINE_Director")
director.set_editor_property("lane_start", unreal.Vector(XA, LY, GZ + HH))
director.set_editor_property("ground_z", GZ)
director.set_editor_property("side_sign", SIDE_SIGN)
director.set_editor_property("character_class", BP)
director.set_editor_property("pull_montage", EAL.load_asset("/Game/Character/Animation/Blender/AM_Grapple_Pull_Montage"))
director.set_editor_property("flee_montage", EAL.load_asset("/Game/Character/Animation/Blender/AM_Grapple_Flee_Montage"))
director.set_editor_property("fall_montage", EAL.load_asset("/Game/Character/Animation/Blender/AM_FallBack_Montage"))
director.set_editor_property("get_up_montage", EAL.load_asset("/Game/Character/Animation/Blender/AM_GettingUp_Montage"))
unreal.log("[CT] 길 x=%d~%d y=%d 옆카메라=%s 캡슐반높이=%.0f" % (XA, XA + LANE_LEN, LY, "남" if SIDE_SIGN < 0 else "북", HH))
ELL.save_current_level()

# 영상 캡처용 빈 시퀀스 (길이만 정함. 연출은 감독이 진행)
if not EAL.does_directory_exist(SEQ_DIR):
    EAL.make_directory(SEQ_DIR)
full_seq = SEQ_DIR + "/" + SEQ_NAME
if EAL.does_asset_exist(full_seq):
    EAL.delete_asset(full_seq)
seq = unreal.AssetToolsHelpers.get_asset_tools().create_asset(SEQ_NAME, SEQ_DIR, unreal.LevelSequence, unreal.LevelSequenceFactoryNew())
seq.set_display_rate(unreal.FrameRate(FPS, 1))
seq.set_playback_start(0)
seq.set_playback_end(int(round(TOTAL_SEC * FPS)))
EAL.save_loaded_asset(seq)
unreal.log("[CT] CINE_BUILD_DONE")
