"""
noar_import_fbx.py - imports a room export (Export/<Apt>/SM_*.fbx) with the settings of Standard v1.1 section 4:
one static mesh per FBX node, pivot = node origin (no transform baked), Nanite on, no auto-collision
(simple box for walls/floors only), no auto-generated LODs, materials by slot name (no texture import).

    import noar_import_fbx as imp
    imp.import_room("/Users/<you>/.../Export/Dempster", "/Game/Apartments/DEMPSTER")

The three FBX go to:
    SM_DEM_PROTO_Architecture.fbx -> <dest>/Architecture
    SM_DEM_PROTO_Millwork.fbx     -> <dest>/Millwork
    SM_DEM_PROTO_Furniture.fbx    -> <dest>/Furniture
Then run noar_millwork_assembler.assemble(...) and noar_spawn_lights.spawn(...).
"""
import os
import unreal

FILES = {"SM_DEM_PROTO_Architecture.fbx": "Architecture", "SM_DEM_PROTO_Millwork.fbx": "Millwork", "SM_DEM_PROTO_Furniture.fbx": "Furniture"}


def _options(collision):
    o = unreal.FbxImportUI()
    o.import_mesh = True
    o.import_as_skeletal = False
    o.import_materials = True        # creates one simple material per slot name so slots are visible; replace with M_* later
    o.import_textures = False
    o.import_animations = False
    o.create_physics_asset = False
    o.automated_import_should_detect_type = False
    o.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    sm = o.static_mesh_import_data
    sm.combine_meshes = False
    sm.auto_generate_collision = collision
    sm.generate_lightmap_u_vs = False
    sm.remove_degenerates = True
    sm.build_nanite = True
    sm.import_uniform_scale = 1.0
    sm.convert_scene = True
    sm.force_front_x_axis = False
    sm.convert_scene_unit = True
    sm.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
    sm.import_mesh_lo_ds = False
    sm.transform_vertex_to_absolute = False   # KEEP pivots: vertices stay relative to the node, the node transform is NOT baked
    sm.bake_pivot_in_vertex = False
    sm.reorder_material_to_fbx_order = True
    return o


def export_version(export_dir):
    try:
        with open(os.path.join(export_dir, "export_version.txt")) as f:
            return f.read().strip()
    except Exception:
        return "v0"


def geometry_root(export_dir, dest_root):
    """Each geometry export is imported into its own folder, so a re-export never fights old assets / names."""
    return f"{dest_root}/Geometry/G{export_version(export_dir).replace('-', '_')}"


def use_legacy_fbx_importer():
    """UE 5.8 routes .fbx through Interchange, which ignores FbxImportUI (pivots, collision, scale options).
    The legacy importer keeps each node's pivot as the mesh origin - exactly what the NOAR pipeline needs."""
    for cmd in ("Interchange.FeatureFlags.Import.FBX False", "Interchange.FeatureFlags.Import.FBX 0"):
        unreal.SystemLibrary.execute_console_command(None, cmd)


def import_room(export_dir, dest_root):
    use_legacy_fbx_importer()
    root = geometry_root(export_dir, dest_root)
    tasks = []
    for fn, sub in FILES.items():
        path = os.path.join(export_dir, fn)
        if not os.path.exists(path):
            unreal.log_warning(f"[NOAR] missing {path}")
            continue
        t = unreal.AssetImportTask()
        t.filename = path
        t.destination_path = f"{root}/{sub}"
        t.automated = True
        t.replace_existing = True
        t.save = True
        t.options = _options(collision=(sub == "Architecture"))
        tasks.append(t)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    n = sum(len(t.imported_object_paths) for t in tasks)
    unreal.log(f"[NOAR] imported {n} assets into {root}")
    return n


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    import_room(os.path.abspath(os.path.join(here, "..", "..", "Export", "Dempster")), "/Game/Apartments/DEMPSTER")
