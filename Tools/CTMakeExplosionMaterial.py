# 폭발 효과용 머티리얼 M_CT_ExplosionFx 를 만듭니다 (조명 없음, 반투명, 파라미터: Color, Brightness, Opacity). 에디터를 끄고 실행:
#   UnrealEditor-Cmd.exe CatchTheTail.uproject -run=pythonscript -script=Tools/CTMakeExplosionMaterial.py
import unreal

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
PATH = "/Game/VFX/M_CT_ExplosionFx"
if EAL.does_asset_exist(PATH):
    EAL.delete_asset(PATH)

mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset("M_CT_ExplosionFx", "/Game/VFX", unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property("two_sided", True)
mat.set_editor_property("used_with_instanced_static_meshes", True)

color = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -600, 0)
color.set_editor_property("parameter_name", "Color")
color.set_editor_property("default_value", unreal.LinearColor(1.0, 0.45, 0.08, 1.0))
bright = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -600, 200)
bright.set_editor_property("parameter_name", "Brightness")
bright.set_editor_property("default_value", 3.0)
opacity = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -600, 400)
opacity.set_editor_property("parameter_name", "Opacity")
opacity.set_editor_property("default_value", 1.0)
mul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 80)

MEL.connect_material_expressions(color, "", mul, "A")
MEL.connect_material_expressions(bright, "", mul, "B")
MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)
MEL.recompile_material(mat)
EAL.save_loaded_asset(mat)
unreal.log("[CT] EXPLOSION_MAT_DONE")
