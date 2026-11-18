import unreal
out=[]
skel = unreal.load_asset("/Game/Character/Player/Mesh/Chicken_Skeleton")
sm = unreal.load_asset("/Game/Character/Player/Mesh/SK_Chicken")
try:
    names = unreal.AnimationLibrary.get_skeleton_bone_names(skel) if hasattr(unreal.AnimationLibrary,"get_skeleton_bone_names") else None
except Exception as e:
    names = str(e)
out.append("bones api: " + str(names))
for a in ["Pull_Heavy_Object","Wheelbarrow_Dump","Walk_With_Briefcase","Idle","Run"]:
    s = unreal.load_asset("/Game/Character/Animation/Blender/"+a)
    if s:
        out.append(f"{a} len={s.get_editor_property('sequence_length') if False else unreal.AnimationLibrary.get_sequence_length(s)} frames={unreal.AnimationLibrary.get_num_frames(s)}")
m = unreal.load_asset("/Game/Character/Animation/Blender/AM_Pull_Heavy_Object_Start_Montage")
out.append("grab montage slots: " + str(unreal.AnimationLibrary.get_montage_slot_names(m)))
out.append("sections: " + str([str(m.get_section_name(i)) for i in range(m.get_num_sections())]))
out.append("len: " + str(m.get_play_length()))
open("C:/Unreal/CatchTheTail/Saved/anim_info.txt","w",encoding="utf-8").write("\n".join(out))
