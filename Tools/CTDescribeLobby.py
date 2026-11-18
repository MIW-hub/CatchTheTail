import unreal
asset = unreal.EditorAssetLibrary.load_asset("/Game/UI/WBP_CTLobbyWidget")
text = unreal.CTUIBuildTools.describe_widget_tree(asset)
open("C:/Unreal/CatchTheTail/Saved/lobby_tree.txt", "w", encoding="utf-8").write(text)
