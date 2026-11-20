# 후처리 카툰 머티리얼 내용을 로그로 출력합니다 (출력 핀에서 거꾸로 따라감). 에디터를 끄고 실행.
import unreal

L = unreal.MaterialEditingLibrary

def props(ex):
    out = []
    for p in ("parameter_name", "r", "g", "b", "a", "constant", "code", "description", "default_value", "value", "texture", "function", "material_function", "input_name", "output_type", "coordinate", "const_a", "const_b", "default_value_x"):
        try:
            v = ex.get_editor_property(p)
            if v is not None and str(v) != "":
                out.append("%s=%s" % (p, str(v)[:900].replace("\n", " | ")))
        except Exception:
            pass
    return " ".join(out)

def walk(mat, ex, seen, depth=0):
    key = ex.get_name()
    if key in seen:
        unreal.log("[CT] %s(재참조) %s" % ("  " * depth, key))
        return
    seen.add(key)
    unreal.log("[CT] %s%s <%s> %s" % ("  " * depth, key, ex.get_class().get_name(), props(ex)))
    try:
        ins = L.get_inputs_for_material_expression(mat, ex)
        names = L.get_material_expression_input_names(ex)
    except Exception as e:
        unreal.log("[CT] %s입력 읽기 실패: %s" % ("  " * depth, e))
        return
    for i, sub in enumerate(ins):
        label = names[i] if i < len(names) else str(i)
        if sub is None:
            continue
        try:
            outname = L.get_input_node_output_name_for_material_expression(ex, sub)
        except Exception:
            outname = ""
        unreal.log("[CT] %s  └ 입력 %s (출력 %s):" % ("  " * depth, label, outname))
        walk(mat, sub, seen, depth + 2)

for name in ("M_PP_SemiToon1", "M_PP_SemiToon"):
    mat = unreal.EditorAssetLibrary.load_asset("/Game/Material/" + name)
    unreal.log("[CT] ==== %s" % name)
    for prop in (unreal.MaterialProperty.MP_EMISSIVE_COLOR,):
        node = L.get_material_property_input_node(mat, prop)
        unreal.log("[CT] 출력 %s 연결: %s" % (prop, node))
        if node:
            walk(mat, node, set())
    unreal.log("[CT] 스칼라 파라미터: %s / 벡터: %s / 텍스처: %s" % (L.get_scalar_parameter_names(mat), L.get_vector_parameter_names(mat), L.get_texture_parameter_names(mat)))
unreal.log("[CT] DESCRIBE_DONE")
