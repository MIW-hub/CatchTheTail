import unreal
for n in ("WBP_CTMainMenuWidget", "WBP_CTKeyBindEntryWidget", "WBP_CTSettingsWidget"):
    a = unreal.EditorAssetLibrary.load_asset("/Game/UI/" + n)
    open("C:/Unreal/CatchTheTail/Saved/tree_%s.txt" % n, "w", encoding="utf-8").write(unreal.CTUIBuildTools.describe_widget_tree(a))
