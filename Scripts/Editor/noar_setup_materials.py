"""
noar_setup_materials.py - creates the NOAR master materials, the material instances for every slot role used by the
geometry builders, the finish-library instances referenced by Data/<Apt>/finish_options.csv, and assigns the slot
instances to every imported static mesh by slot name.

    import noar_setup_materials as mats
    mats.setup_all("/Game/Apartments/DEMPSTER")

Masters (Content/NOAR/Materials/Master):
    M_Master_Opaque    Tint, Roughness, Metallic, Specular, UseTextures (0/1 lerp), Tiling (1 UV = 1 m), T_BaseColor, T_Normal, T_ORM, NormalStrength
    M_Master_Glass     Tint, Opacity, Roughness (thin translucent, forward shaded)
    M_Master_Mirror    Tint (metallic 1, roughness 0.02)
    M_Master_Emissive  Tint, Emissive (driven by ANOARInteractiveLight: 1.0 == 200 nit), EmissiveScale
Instances are MI_<ROLE> under Content/NOAR/Materials/Roles and MI_* under Content/NOAR/Materials/Library/<Group>.
Everything is a placeholder tint until real slab scans / veneer textures are dropped into the instances (UseTextures=1).
Re-running keeps existing assets and only updates parameter values.
"""
import csv
import os
import unreal

MASTER_DIR = "/Game/NOAR/Materials/Master"
ROLE_DIR = "/Game/NOAR/Materials/Roles"
LIB_DIR = "/Game/NOAR/Materials/Library"

mel = unreal.MaterialEditingLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary

# role -> (master, {param: value}) ; colours are linear sRGB-ish approximations of the spec finishes
ROLES = {
    "FRONT":         ("Opaque",   dict(Tint=(0.86, 0.86, 0.85), Roughness=0.45, Metallic=0.0)),   # existing white cabinetry
    "CARCASS":       ("Opaque",   dict(Tint=(0.78, 0.78, 0.76), Roughness=0.55)),
    "INTERIOR":      ("Opaque",   dict(Tint=(0.72, 0.70, 0.66), Roughness=0.5)),                 # LAM01 light grey soft touch
    "TOP":           ("Opaque",   dict(Tint=(0.90, 0.89, 0.86), Roughness=0.35)),
    "STONE":         ("Opaque",   dict(Tint=(0.88, 0.87, 0.84), Roughness=0.25, Specular=0.6)),  # SS01 Estatuario
    "STONE_BLUE":    ("Opaque",   dict(Tint=(0.42, 0.50, 0.58), Roughness=0.12, Specular=0.7)),  # SS02 Cipollino polished
    "PANEL":         ("Opaque",   dict(Tint=(0.40, 0.28, 0.17), Roughness=0.5)),                 # M01 rift oak stained
    "WOOD":          ("Opaque",   dict(Tint=(0.38, 0.27, 0.16), Roughness=0.5)),
    "MIRROR_BRONZE": ("Mirror",   dict(Tint=(0.62, 0.48, 0.34))),                                # GL01 bronze mirror
    "GLASS_BRONZE":  ("Glass",    dict(Tint=(0.55, 0.40, 0.28), Opacity=0.35, Roughness=0.03)),  # GL02 bronze glass
    "GLASS":         ("Glass",    dict(Tint=(0.92, 0.96, 1.0), Opacity=0.12, Roughness=0.02)),
    "METAL_BRONZE":  ("Opaque",   dict(Tint=(0.45, 0.31, 0.19), Roughness=0.35, Metallic=1.0)),  # MT01 aged bronze
    "METAL_BLACK":   ("Opaque",   dict(Tint=(0.03, 0.03, 0.03), Roughness=0.4, Metallic=0.8)),
    "STAINLESS":     ("Opaque",   dict(Tint=(0.70, 0.70, 0.72), Roughness=0.3, Metallic=1.0)),
    "PAINT":         ("Opaque",   dict(Tint=(0.93, 0.92, 0.88), Roughness=0.85, Specular=0.3)),  # PT01 White Dove
    "CEILING":       ("Opaque",   dict(Tint=(0.96, 0.96, 0.95), Roughness=0.9, Specular=0.25)),
    "FLOOR":         ("Opaque",   dict(Tint=(0.74, 0.72, 0.68), Roughness=0.3, Specular=0.5, Tiling=1.0 / 1.2)),  # TL01 existing tile ~120 cm
    "LED":           ("Emissive", dict(Tint=(1.0, 0.92, 0.80), Emissive=1.0)),
    "FABRIC":        ("Opaque",   dict(Tint=(0.80, 0.77, 0.70), Roughness=0.95, Specular=0.2)),  # FB01 Midori ivory
    "APPLIANCE":     ("Opaque",   dict(Tint=(0.05, 0.05, 0.05), Roughness=0.15, Specular=0.6)),  # black glass cooktop
    "CONCRETE":      ("Opaque",   dict(Tint=(0.55, 0.55, 0.53), Roughness=0.9)),
}

# finish library instances referenced by finish_options.csv (path relative to LIB_DIR) -> master + params
LIBRARY = {
    "Laminate/MI_LAM01_LightGrey":   ("Opaque", dict(Tint=(0.72, 0.70, 0.66), Roughness=0.6)),
    "Laminate/MI_LAM02_CashmereGold": ("Opaque", dict(Tint=(0.55, 0.43, 0.30), Roughness=0.4, Metallic=0.35)),
    "Wood/MI_Oak_RiftSawn_M01":      ("Opaque", dict(Tint=(0.40, 0.28, 0.17), Roughness=0.5)),
    "Wood/MI_Oak_Natural":           ("Opaque", dict(Tint=(0.70, 0.56, 0.40), Roughness=0.55)),
    "Wood/MI_Walnut":                ("Opaque", dict(Tint=(0.30, 0.19, 0.12), Roughness=0.5)),
    "Wood/MI_Floor_OakPlank":        ("Opaque", dict(Tint=(0.66, 0.52, 0.37), Roughness=0.45, Tiling=1.0 / 1.8)),
    "Stone/MI_SS01_Estatuario":      ("Opaque", dict(Tint=(0.88, 0.87, 0.84), Roughness=0.25, Specular=0.6, Tiling=1.0 / 3.2)),
    "Stone/MI_SS02_Cipollino":       ("Opaque", dict(Tint=(0.42, 0.50, 0.58), Roughness=0.12, Specular=0.7, Tiling=1.0 / 3.2)),
    "Stone/MI_Quartz_White":         ("Opaque", dict(Tint=(0.92, 0.92, 0.90), Roughness=0.2)),
    "Stone/MI_Porcelain_Grey":       ("Opaque", dict(Tint=(0.55, 0.55, 0.54), Roughness=0.3)),
    "Stone/MI_Floor_Limestone":      ("Opaque", dict(Tint=(0.78, 0.74, 0.66), Roughness=0.6, Tiling=1.0 / 0.9)),
    "Fabric/MI_FB01_MidoriIvory":    ("Opaque", dict(Tint=(0.80, 0.77, 0.70), Roughness=0.95, Specular=0.2)),
    "Fabric/MI_Boucle_Taupe":        ("Opaque", dict(Tint=(0.55, 0.50, 0.44), Roughness=0.98, Specular=0.15)),
    "Fabric/MI_Velvet_Blue":         ("Opaque", dict(Tint=(0.10, 0.22, 0.40), Roughness=0.8, Specular=0.3)),
    "Paint/MI_PT01_WhiteDove":       ("Opaque", dict(Tint=(0.93, 0.92, 0.88), Roughness=0.85, Specular=0.3)),
    "Paint/MI_PT05_LimewashMoth":    ("Opaque", dict(Tint=(0.70, 0.67, 0.62), Roughness=0.95, Specular=0.2)),
    "Paint/MI_Paint_WarmGrey":       ("Opaque", dict(Tint=(0.62, 0.60, 0.57), Roughness=0.85, Specular=0.3)),
}
APT_LIBRARY = {  # under /Game/Apartments/<APT>/Materials
    "MI_Kitchen_ExistingWhite": ("Opaque", dict(Tint=(0.86, 0.86, 0.85), Roughness=0.45)),
    "MI_Floor_ExistingTile":    ("Opaque", dict(Tint=(0.74, 0.72, 0.68), Roughness=0.3, Specular=0.5, Tiling=1.0 / 1.2)),
}


# ---------------------------------------------------------------- masters
def _new_material(name):
    path = f"{MASTER_DIR}/{name}"
    if eal.does_asset_exist(path):
        return eal.load_asset(path)
    return asset_tools.create_asset(name, MASTER_DIR, unreal.Material, unreal.MaterialFactoryNew())


def _scalar(m, name, default, x, y, group="NOAR"):
    n = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, x, y)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("default_value", default)
    n.set_editor_property("group", group)
    return n


def _vector(m, name, default, x, y, group="NOAR"):
    n = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, x, y)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("default_value", unreal.LinearColor(*default, 1.0))
    n.set_editor_property("group", group)
    return n


_DEFAULT_TEX = {
    # a texture parameter without a texture is a compile error (UE then shows the checker WorldGridMaterial)
    "SAMPLERTYPE_COLOR": "/Engine/EngineResources/DefaultTexture.DefaultTexture",
    "SAMPLERTYPE_NORMAL": "/Engine/EngineMaterials/FlatNormal.FlatNormal",
}


def _texture(m, name, x, y, sampler="SAMPLERTYPE_COLOR"):
    if sampler not in _DEFAULT_TEX:
        sampler = "SAMPLERTYPE_COLOR"   # ORM uses the colour sampler with the engine default texture until real maps exist
    n = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    n.set_editor_property("parameter_name", name)
    tex = unreal.load_asset(_DEFAULT_TEX[sampler])
    if tex:
        n.set_editor_property("texture", tex)
    n.set_editor_property("sampler_type", getattr(unreal.MaterialSamplerType, sampler))
    n.set_editor_property("group", "NOAR Textures")
    return n


def _node(m, cls, x, y):
    return mel.create_material_expression(m, cls, x, y)


def _connect(a, a_out, b, b_in):
    mel.connect_material_expressions(a, a_out, b, b_in)


def _build_opaque():
    m = _new_material("M_Master_Opaque_v2")   # v1 failed to compile (texture params without textures)
    if mel.get_num_material_expressions(m) > 0:
        return m
    # UVs: TexCoord * Tiling
    tc = _node(m, unreal.MaterialExpressionTextureCoordinate, -1400, 0)
    tiling = _scalar(m, "Tiling", 1.0, -1400, 120)
    uv = _node(m, unreal.MaterialExpressionMultiply, -1200, 40)
    _connect(tc, "", uv, "A"); _connect(tiling, "", uv, "B")
    t_bc = _texture(m, "T_BaseColor", -1000, -200)
    t_n = _texture(m, "T_Normal", -1000, 200, "SAMPLERTYPE_NORMAL")
    t_orm = _texture(m, "T_ORM", -1000, 500, "SAMPLERTYPE_LINEAR_COLOR")
    for t in (t_bc, t_n, t_orm):
        _connect(uv, "", t, "UVs")
    tint = _vector(m, "Tint", (0.8, 0.8, 0.8), -1000, -450)
    use = _scalar(m, "UseTextures", 0.0, -800, -600)
    # BaseColor = lerp(Tint, T_BaseColor * Tint, UseTextures)
    mul_bc = _node(m, unreal.MaterialExpressionMultiply, -700, -300)
    _connect(t_bc, "RGB", mul_bc, "A"); _connect(tint, "", mul_bc, "B")
    lerp_bc = _node(m, unreal.MaterialExpressionLinearInterpolate, -500, -350)
    _connect(tint, "", lerp_bc, "A"); _connect(mul_bc, "", lerp_bc, "B"); _connect(use, "", lerp_bc, "Alpha")
    mel.connect_material_property(lerp_bc, "", unreal.MaterialProperty.MP_BASE_COLOR)
    # Roughness = lerp(Roughness, T_ORM.G * Roughness*2, UseTextures)  (texture authored so mid-grey == the scalar)
    rough = _scalar(m, "Roughness", 0.5, -800, 400)
    mul_r = _node(m, unreal.MaterialExpressionMultiply, -600, 450)
    _connect(t_orm, "G", mul_r, "A"); _connect(rough, "", mul_r, "B")
    two = _node(m, unreal.MaterialExpressionConstant, -600, 520); two.set_editor_property("r", 2.0)
    mul_r2 = _node(m, unreal.MaterialExpressionMultiply, -480, 470)
    _connect(mul_r, "", mul_r2, "A"); _connect(two, "", mul_r2, "B")
    lerp_r = _node(m, unreal.MaterialExpressionLinearInterpolate, -300, 420)
    _connect(rough, "", lerp_r, "A"); _connect(mul_r2, "", lerp_r, "B"); _connect(use, "", lerp_r, "Alpha")
    mel.connect_material_property(lerp_r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    # Metallic = lerp(Metallic, T_ORM.B, UseTextures)
    met = _scalar(m, "Metallic", 0.0, -800, 650)
    lerp_m = _node(m, unreal.MaterialExpressionLinearInterpolate, -300, 650)
    _connect(met, "", lerp_m, "A"); _connect(t_orm, "B", lerp_m, "B"); _connect(use, "", lerp_m, "Alpha")
    mel.connect_material_property(lerp_m, "", unreal.MaterialProperty.MP_METALLIC)
    spec = _scalar(m, "Specular", 0.5, -300, 800)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    # Normal = lerp(flat, T_Normal, UseTextures * NormalStrength)
    flat = _node(m, unreal.MaterialExpressionConstant3Vector, -800, 150); flat.set_editor_property("constant", unreal.LinearColor(0, 0, 1, 1))
    nstr = _scalar(m, "NormalStrength", 1.0, -800, 250)
    mul_n = _node(m, unreal.MaterialExpressionMultiply, -600, 220)
    _connect(use, "", mul_n, "A"); _connect(nstr, "", mul_n, "B")
    lerp_n = _node(m, unreal.MaterialExpressionLinearInterpolate, -300, 200)
    _connect(flat, "", lerp_n, "A"); _connect(t_n, "RGB", lerp_n, "B"); _connect(mul_n, "", lerp_n, "Alpha")
    mel.connect_material_property(lerp_n, "", unreal.MaterialProperty.MP_NORMAL)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    return m


def _build_glass():
    m = _new_material("M_Master_Glass")
    if mel.get_num_material_expressions(m) > 0:
        return m
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    m.set_editor_property("two_sided", True)
    try:
        m.set_editor_property("screen_space_reflections", True)
    except Exception:
        pass
    tint = _vector(m, "Tint", (0.92, 0.96, 1.0), -600, -200)
    mel.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
    op = _scalar(m, "Opacity", 0.15, -600, 0)
    mel.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)
    rough = _scalar(m, "Roughness", 0.03, -600, 150)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = _scalar(m, "Specular", 1.0, -600, 300)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    return m


def _build_mirror():
    m = _new_material("M_Master_Mirror")
    if mel.get_num_material_expressions(m) > 0:
        return m
    tint = _vector(m, "Tint", (0.9, 0.9, 0.9), -600, -200)
    mel.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
    one = _node(m, unreal.MaterialExpressionConstant, -600, 0); one.set_editor_property("r", 1.0)
    mel.connect_material_property(one, "", unreal.MaterialProperty.MP_METALLIC)
    rough = _scalar(m, "Roughness", 0.02, -600, 150)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    return m


def _build_emissive():
    m = _new_material("M_Master_Emissive")
    if mel.get_num_material_expressions(m) > 0:
        return m
    tint = _vector(m, "Tint", (1.0, 0.92, 0.8), -800, -200)
    emis = _scalar(m, "Emissive", 1.0, -800, 0)               # driven at runtime: 1.0 == 200 nit
    scale = _scalar(m, "EmissiveScale", 5.0, -800, 120)        # tune once with a nit meter / EV100 10 reference
    mul1 = _node(m, unreal.MaterialExpressionMultiply, -600, -50)
    _connect(emis, "", mul1, "A"); _connect(scale, "", mul1, "B")
    mul2 = _node(m, unreal.MaterialExpressionMultiply, -400, -100)
    _connect(tint, "", mul2, "A"); _connect(mul1, "", mul2, "B")
    mel.connect_material_property(mul2, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    dark = _node(m, unreal.MaterialExpressionConstant3Vector, -400, 150); dark.set_editor_property("constant", unreal.LinearColor(0.02, 0.02, 0.02, 1))
    mel.connect_material_property(dark, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = _node(m, unreal.MaterialExpressionConstant, -400, 250); rough.set_editor_property("r", 0.6)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    return m


def build_masters():
    return {"Opaque": _build_opaque(), "Glass": _build_glass(), "Mirror": _build_mirror(), "Emissive": _build_emissive()}


# ---------------------------------------------------------------- instances
def _instance(folder, name, master, params):
    path = f"{folder}/{name}"
    if eal.does_asset_exist(path):
        mi = eal.load_asset(path)
    else:
        mi = asset_tools.create_asset(name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    if mi.get_editor_property("parent") != master:
        mel.set_material_instance_parent(mi, master)
    for k, v in params.items():
        if isinstance(v, tuple):
            mel.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(*v, 1.0))
        else:
            mel.set_material_instance_scalar_parameter_value(mi, k, float(v))
    mel.update_material_instance(mi)
    eal.save_loaded_asset(mi)
    return mi


def build_instances(masters, apt_root):
    roles = {r: _instance(ROLE_DIR, f"MI_{r}", masters[mk], p) for r, (mk, p) in ROLES.items()}
    for rel, (mk, p) in LIBRARY.items():
        folder, name = rel.rsplit("/", 1)
        _instance(f"{LIB_DIR}/{folder}", name, masters[mk], p)
    for name, (mk, p) in APT_LIBRARY.items():
        _instance(f"{apt_root}/Materials", name, masters[mk], p)
    return roles


# ---------------------------------------------------------------- assignment
def assign_to_meshes(content_path, roles):
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    n_mesh = n_slot = 0
    for a in reg.get_assets_by_path(content_path, recursive=True):
        if a.asset_class_path.asset_name != "StaticMesh":
            continue
        mesh = a.get_asset()
        mats = list(mesh.get_editor_property("static_materials"))
        changed = False
        for sm in mats:
            slot = str(sm.get_editor_property("material_slot_name"))
            role = slot.split("__")[0]
            mi = roles.get(role)
            if mi:
                sm.set_editor_property("material_interface", mi)
                changed = True
                n_slot += 1
            else:
                unreal.log_warning(f"[NOAR] {mesh.get_name()}: no role instance for slot '{slot}'")
        if changed:
            mesh.set_editor_property("static_materials", mats)
            eal.save_loaded_asset(mesh)
            n_mesh += 1
    unreal.log(f"[NOAR] materials assigned: {n_slot} slots on {n_mesh} meshes")


def setup_all(apt_root="/Game/Apartments/DEMPSTER"):
    masters = build_masters()
    roles = build_instances(masters, apt_root)
    assign_to_meshes(apt_root, roles)
    return roles


if __name__ == "__main__":
    setup_all()
