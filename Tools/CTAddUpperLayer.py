import unreal
T = unreal.CTAnimTools
abp = unreal.EditorAssetLibrary.load_asset("/Game/Character/Animation/ABP_CTPlayer")
T.add_upper_body_layer(abp, "UpperBody", "Spine", 1)
unreal.EditorAssetLibrary.save_loaded_asset(abp)
open("C:/Unreal/CatchTheTail/Saved/abp_graph.txt","w",encoding="utf-8").write(T.describe_anim_graph(abp))
