"""
noar_setup_collision.py - makes imported room geometry walkable / blocking.

The 5.8 Interchange FBX importer ignores the legacy FbxImportUI collision option, so meshes arrive without usable
collision and the character falls through the floor. Here every static mesh under the apartment root gets
'complex as simple' collision (exact, fine for static boxy architecture and millwork), and single-box meshes
(floors, ceilings, slabs) additionally get a simple box so the character controller has a cheap, reliable floor.
"""
import unreal

BOX_PARTS = ("FLOOR", "CEIL_BASE", "BALCONY_SLAB", "HALL_S_CEIL", "HALL_W_CEIL")


def setup(content_path):
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    sms = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    n = boxes = no_body = 0
    for a in reg.get_assets_by_path(content_path, recursive=True):
        if a.asset_class_path.asset_name != "StaticMesh":
            continue
        mesh = a.get_asset()
        name = mesh.get_name()
        body = mesh.get_editor_property("body_setup")
        if body is None:
            no_body += 1
        if any(p in name for p in BOX_PARTS):
            try:
                if sms.get_simple_collision_count(mesh) == 0:
                    sms.add_simple_collisions(mesh, unreal.ScriptCollisionShapeType.BOX)
                    boxes += 1
            except Exception as e:  # noqa
                unreal.log_warning(f"[NOAR] {name}: box collision failed: {e}")
        body = mesh.get_editor_property("body_setup")
        if body is not None:
            try:
                body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
            except Exception as e:  # noqa
                unreal.log_warning(f"[NOAR] {name}: complex-as-simple failed: {e}")
        try:
            sms.set_allow_cpu_access(mesh, True)   # complex collision on Nanite meshes reads the fallback mesh
        except Exception:
            pass
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
        n += 1
    unreal.log(f"[NOAR] collision: {n} meshes complex-as-simple, {boxes} box colliders added, {no_body} had no body setup")
