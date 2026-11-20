# 카툰 느낌은 약하게, 외곽선만 살짝 주는 후처리 머티리얼 M_PP_SoftOutline 을 만듭니다. 에디터를 끄고 실행.
# 예전 M_PP_SemiToon(깊이 단계 나누기)의 계산을 그대로 쓰되 강도를 낮추고, 깊이/법선 차이로 외곽선을 더함.
import unreal

L = unreal.MaterialEditingLibrary
PATH = "/Game/Material"
NAME = "M_PP_SoftOutline"
full = PATH + "/" + NAME

# 이미 있으면 지우지 않고 안의 노드만 비움 (레벨의 연결이 끊어지지 않게)
if unreal.EditorAssetLibrary.does_asset_exist(full):
    mat = unreal.EditorAssetLibrary.load_asset(full)
    L.delete_all_material_expressions(mat)
else:
    mat = tools = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
mat.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_BEFORE_BLOOM)

CODE = """
// 각 버퍼(색, 깊이, 법선)의 UV 를 따로 구하고, 이웃 위치는 버퍼 한 칸(BufferSizeAndInvSize) 단위로 이동
float2 UVc = GetDefaultSceneTextureUV(Parameters, 14);
float2 UVd = GetDefaultSceneTextureUV(Parameters, 1);
float2 UVn = GetDefaultSceneTextureUV(Parameters, 8);
float3 Color = SceneTextureLookup(UVc, 14, false).rgb;

// 1) 약한 카툰 음영 (예전 SemiToon 계산, ToonStrength 로 섞는 양 조절)
float Lum = dot(Color, float3(0.3, 0.59, 0.11));
float x = saturate(Lum / Range) * Steps;
float S = (floor(x) + smoothstep(0.5 - Softness, 0.5 + Softness, frac(x))) / Steps * Range;
float B = lerp(Lum, S + max(Lum - Range, 0.0), ToonStrength);
float3 Toon = Color * (B / max(Lum, 0.001));

// 2) 외곽선: 주변 4방향의 깊이 차이와 법선 차이
float2 px = ResolvedView.BufferSizeAndInvSize.zw * Thickness;
float2 offs[4] = { float2(px.x, 0), float2(-px.x, 0), float2(0, px.y), float2(0, -px.y) };
float d0 = CalcSceneDepth(UVd);
float3 n0 = SceneTextureLookup(UVn, 8, false).xyz;
float dMax = 0.0;
float nMax = 0.0;
[unroll]
for (int i = 0; i < 4; ++i)
{
    dMax = max(dMax, abs(d0 - CalcSceneDepth(UVd + offs[i])));
    nMax = max(nMax, 1.0 - dot(n0, SceneTextureLookup(UVn + offs[i], 8, false).xyz));
}
float depthEdge = dMax / max(d0, 1.0);
float edge = saturate(max(smoothstep(DepthThreshold, DepthThreshold * 2.0, depthEdge), smoothstep(NormalThreshold, NormalThreshold * 2.0, nMax)));

return lerp(Toon, OutlineColor, edge * OutlineStrength);
"""

scalars = [
    ("Steps", 4.0), ("Softness", 0.08), ("Range", 0.5), ("ToonStrength", 0.2),
    ("Thickness", 1.0), ("DepthThreshold", 0.03), ("NormalThreshold", 0.85), ("OutlineStrength", 0.5),
]
custom = L.create_material_expression(mat, unreal.MaterialExpressionCustom, -200, 0)
names = [s[0] for s in scalars] + ["OutlineColor", "SceneColorIn", "SceneDepthIn", "SceneNormalIn"]
custom.set_editor_property("code", CODE)
custom.set_editor_property("description", "SoftOutline")
custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
ins = []
for n in names:
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", n)
    ins.append(ci)
custom.set_editor_property("inputs", ins)

y = -300
for n, v in scalars:
    p = L.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -600, y)
    p.set_editor_property("parameter_name", n)
    p.set_editor_property("default_value", v)
    L.connect_material_expressions(p, "", custom, n)
    y += 120
cp = L.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -600, y)
cp.set_editor_property("parameter_name", "OutlineColor")
cp.set_editor_property("default_value", unreal.LinearColor(0.035, 0.03, 0.04, 1.0))
L.connect_material_expressions(cp, "", custom, "OutlineColor")

# SceneTexture 노드를 실제로 연결해야 화면 읽기 함수가 셰이더에 포함됨
for nm, sid, yy in (("SceneColorIn", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0, 300), ("SceneDepthIn", unreal.SceneTextureId.PPI_SCENE_DEPTH, 420), ("SceneNormalIn", unreal.SceneTextureId.PPI_WORLD_NORMAL, 540)):
    st = L.create_material_expression(mat, unreal.MaterialExpressionSceneTexture, -600, yy)
    st.set_editor_property("scene_texture_id", sid)
    L.connect_material_expressions(st, "Color", custom, nm)

L.connect_material_property(custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
L.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log("[CT] SOFTOUTLINE_DONE 노드 수: %d" % L.get_num_material_expressions(mat))
