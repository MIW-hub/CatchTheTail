import unreal
out=[]
cls = unreal.load_class(None, "/Game/Character/Player/BP_CTCharacter.BP_CTCharacter_C")
cdo = unreal.get_default_object(cls)
for p in ["GrabMontage","PushMontage","CaughtMontage","PushedMontage","RunMontage","HoldMontage","ThrowMontage"]:
    try:
        m = cdo.get_editor_property(p)
        out.append(f"{p}: {m.get_path_name() if m else None}")
        if m:
            out.append(f"  len={m.get_editor_property('sequence_length')} slots={[ (t.get_editor_property('slot_name')) for t in m.get_editor_property('slot_anim_tracks')]}")
            out.append(f"  sections={[str(s.get_editor_property('section_name')) for s in m.get_editor_property('composite_sections')]}")
            for t in m.get_editor_property('slot_anim_tracks'):
                for seg in t.get_editor_property('anim_track').get_editor_property('anim_segments'):
                    a = seg.get_editor_property('anim_reference')
                    out.append(f"  seg anim={a.get_path_name() if a else None} start={seg.get_editor_property('anim_start_time')} end={seg.get_editor_property('anim_end_time')} rate={seg.get_editor_property('anim_play_rate')}")
    except Exception as e:
        out.append(f"{p}: err {e}")
skel = unreal.load_asset("/Game/Character/Player/Mesh/Chicken_Skeleton")
out.append("skeleton: " + str(skel))
try:
    out.append("slots: " + str([str(s.get_editor_property('slot_name')) for g in skel.get_editor_property('slot_groups') for s in g.get_editor_property('slot_names')]) if False else "")
except Exception as e:
    out.append(f"slots err {e}")
open("C:/Unreal/CatchTheTail/Saved/montages.txt","w",encoding="utf-8").write("\n".join(out))
