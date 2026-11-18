import unreal
abp = unreal.EditorAssetLibrary.load_asset("/Game/Character/Animation/ABP_CTPlayer")
open("C:/Unreal/CatchTheTail/Saved/abp_graph.txt","w",encoding="utf-8").write(unreal.CTAnimTools.describe_anim_graph(abp))
