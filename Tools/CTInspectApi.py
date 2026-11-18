import unreal
names = [n for n in dir(unreal) if any(k in n for k in ("Montage","AnimGraph","LayeredBone","AnimBP","AnimationBlueprint","AnimBlueprint"))]
out=["\n".join(names)]
for n in ["AnimationLibrary","AnimMontage","AnimMontageFactory","AnimBlueprint","AnimGraphNode_Slot","AnimGraphNode_LayeredBoneBlend","AnimGraphNode_Base","AnimMontageLibrary"]:
    if hasattr(unreal,n):
        c=getattr(unreal,n)
        out.append(f"== {n}: "+", ".join(sorted(x for x in dir(c) if not x.startswith("_"))[:200]))
open("C:/Unreal/CatchTheTail/Saved/api.txt","w",encoding="utf-8").write("\n".join(out))
