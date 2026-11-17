# 라운드 사이 결과 팝업 WBP 두 개를 처음 한 번 만들고 뼈대를 채웁니다 (이미 있으면 건너뜀).
# 실행: UnrealEditor-Cmd.exe CatchTheTail.uproject -run=pythonscript -script=Tools\CTCreateRoundBreakWidgets.py  (에디터를 끄고)
import unreal

UI_DIR = "/Game/UI"


def create_widget_blueprint(name, parent_path):
    path = f"{UI_DIR}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log(f"[CT] 이미 있음, 건너뜀: {path}")
        return None
    parent = unreal.load_class(None, parent_path)
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, UI_DIR, unreal.WidgetBlueprint, factory)
    unreal.log(f"[CT] 만듦: {path} -> {asset}")
    return asset


row = create_widget_blueprint("WBP_CTRoundBreakRowWidget", "/Script/CatchTheTail.CTRoundBreakRowWidget")
if row:
    unreal.log(f"[CT] 줄 뼈대: {unreal.CTUIBuildTools.build_round_break_row_design(row)}")
    unreal.EditorAssetLibrary.save_loaded_asset(row)

result = create_widget_blueprint("WBP_CTTreasureResultWidget", "/Script/CatchTheTail.CTTreasureResultWidget")
if result:
    unreal.log(f"[CT] 보물찾기 결과 뼈대: {unreal.CTUIBuildTools.build_treasure_result_design(result)}")
    unreal.EditorAssetLibrary.save_loaded_asset(result)

popup = create_widget_blueprint("WBP_CTRoundBreakWidget", "/Script/CatchTheTail.CTRoundBreakWidget")
if popup:
    unreal.log(f"[CT] 팝업 뼈대: {unreal.CTUIBuildTools.build_round_break_design(popup)}")
    row_bp = unreal.EditorAssetLibrary.load_asset(f"{UI_DIR}/WBP_CTRoundBreakRowWidget")
    popup_class = unreal.load_class(None, f"{UI_DIR}/WBP_CTRoundBreakWidget.WBP_CTRoundBreakWidget_C")
    if popup_class and row_bp:
        cdo = unreal.get_default_object(popup_class)
        cdo.set_editor_property("RowWidgetClass", unreal.load_class(None, f"{UI_DIR}/WBP_CTRoundBreakRowWidget.WBP_CTRoundBreakRowWidget_C"))
    unreal.EditorAssetLibrary.save_loaded_asset(popup)
