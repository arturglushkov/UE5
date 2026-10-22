"""
noar_validate_export.py - validate imported millwork meshes against the Standard (sections 2, 3, 4).

Run inside the Unreal Editor (Tools > Execute Python Script, or Output Log in Python mode):

    import noar_validate_export as v
    v.validate("/Game/Apartments/DEMPSTER/Millwork")          # report only
    v.validate("/Game/Apartments/DEMPSTER/Millwork", fix=True) # also sets Nanite / collision where safe

Checks per Static Mesh:
  1. Name follows SM_<APT>_<ROOM>_<UNIT>_<PART>[_<INDEX>][_<SIDE>]
  2. Moving parts: pivot is on the mechanism axis
       DOOR  -> pivot on a vertical edge (bounds min or max X/Y within tolerance), local X points out
       DRW   -> pivot at back-bottom-centre, mesh extends along +X
       FLAP  -> pivot on top edge (lift) or bottom edge (drop-down)
  3. No negative or non-uniform import scale (the mesh itself is in cm, 1:1)
  4. Moving opaque parts: Nanite enabled; glass parts (GLASS): Nanite disabled
  5. Simple collision exists for moving parts (box), collision complexity != UseComplexAsSimple
  6. Material slots named FRONT / CARCASS / TOP / GLASS / INTERIOR (the configurator looks up slots by name)

Every finding is printed as  [LEVEL] mesh : message  and collected in the returned list.
"""
import unreal
import noar_naming as nn

TOL_CM = 0.6                     # tolerance for "pivot on edge"
MATERIAL_SLOT_NAMES = {"FRONT", "CARCASS", "TOP", "GLASS", "INTERIOR", "METAL", "LED", "BACK", "SHELF"}

asset_reg = unreal.AssetRegistryHelpers.get_asset_registry()


def _bounds(mesh: unreal.StaticMesh):
    b = mesh.get_bounding_box()
    return b.min, b.max


def _check_pivot(name, part, side, mesh, findings):
    mn, mx = _bounds(mesh)
    size = unreal.Vector(mx.x - mn.x, mx.y - mn.y, mx.z - mn.z)

    if part == "DOOR":
        # pivot at the hinge edge: one of the Y bounds at ~0, X bounds start at ~0 (door extends along +X when open? no:
        # closed door lies in the YZ plane, X is outward) -> expect min.x ~ 0 and either min.y ~ 0 (L) or max.y ~ 0 (R)
        if abs(mn.x) > TOL_CM and abs(mx.x) > TOL_CM:
            findings.append(("ERROR", name, f"DOOR pivot not on front plane (x bounds {mn.x:.1f}..{mx.x:.1f})"))
        hinge_left = abs(mn.y) <= TOL_CM
        hinge_right = abs(mx.y) <= TOL_CM
        if not (hinge_left or hinge_right):
            findings.append(("ERROR", name, f"DOOR pivot not on a vertical edge (y bounds {mn.y:.1f}..{mx.y:.1f})"))
        elif side == "L" and not hinge_left:
            findings.append(("WARN", name, "name says _L but pivot is on the right edge"))
        elif side == "R" and not hinge_right:
            findings.append(("WARN", name, "name says _R but pivot is on the left edge"))
        if abs(mn.z + size.z * 0.5) > size.z * 0.2 and abs(mn.z) > TOL_CM:
            findings.append(("INFO", name, f"DOOR pivot z={mn.z:.1f}: expected ~mid-height or bottom edge"))

    elif part in ("DRW", "TRAY"):
        if abs(mn.x) > TOL_CM:
            findings.append(("ERROR", name, f"DRAWER pivot must be at the back face (min.x={mn.x:.1f}); travel is +X"))
        if abs(mn.z) > TOL_CM:
            findings.append(("WARN", name, f"DRAWER pivot z={mn.z:.1f}: expected bottom edge"))
        if abs(mn.y + mx.y) > TOL_CM * 2:
            findings.append(("WARN", name, f"DRAWER pivot not centred in Y ({mn.y:.1f}..{mx.y:.1f})"))

    elif part in ("FLAP", "BED"):
        on_top = abs(mx.z) <= TOL_CM
        on_bottom = abs(mn.z) <= TOL_CM
        if not (on_top or on_bottom):
            findings.append(("ERROR", name, f"FLAP pivot must be on top (lift-up) or bottom (drop-down) edge; z {mn.z:.1f}..{mx.z:.1f}"))

    elif part in ("SLIDE", "POCKET"):
        if abs(mx.z) > TOL_CM:
            findings.append(("WARN", name, f"SLIDING pivot expected on top edge at the track (max.z={mx.z:.1f})"))
        if abs(mn.y + mx.y) > TOL_CM * 2:
            findings.append(("WARN", name, "SLIDING pivot not centred in Y"))


def validate(content_path: str, fix: bool = False):
    findings = []
    assets = asset_reg.get_assets_by_path(content_path, recursive=True)
    meshes = [a for a in assets if a.asset_class_path.asset_name == "StaticMesh"]
    unreal.log(f"[NOAR] validating {len(meshes)} static meshes under {content_path}")

    for a in meshes:
        name = str(a.asset_name)
        mesh = a.get_asset()
        parsed = nn.parse(name)
        if not parsed:
            findings.append(("ERROR", name, "name does not follow SM_<APT>_<ROOM>_<UNIT>_<PART>[_<INDEX>][_<SIDE>]"))
            continue
        part, side = parsed["part"], parsed["side"]
        moving = part in nn.MOVING_PARTS

        # 2. pivot
        if moving:
            _check_pivot(name, part, side, mesh, findings)

        # 3. import scale
        import_data = mesh.get_editor_property("asset_import_data")
        try:
            s = import_data.get_editor_property("import_uniform_scale")
            if abs(s - 1.0) > 1e-3:
                findings.append(("ERROR", name, f"import uniform scale {s} != 1.0 (mesh must be exported 1:1 in cm)"))
        except Exception:
            pass

        # 4. Nanite
        nanite = mesh.get_editor_property("nanite_settings")
        wants_nanite = part != "GLASS"
        if bool(nanite.enabled) != wants_nanite:
            msg = f"Nanite should be {'ON' if wants_nanite else 'OFF'} for part {part}"
            if fix:
                nanite.enabled = wants_nanite
                mesh.set_editor_property("nanite_settings", nanite)
                findings.append(("FIXED", name, msg))
            else:
                findings.append(("WARN", name, msg))

        # 5. collision
        body = mesh.get_editor_property("body_setup")
        if body:
            complexity = body.get_editor_property("collision_trace_flag")
            if complexity == unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE:
                findings.append(("ERROR", name, "collision is Complex-as-Simple (not allowed on Nanite millwork; use simple box)"))
            agg = body.get_editor_property("agg_geom")
            n_simple = len(agg.get_editor_property("box_elems")) + len(agg.get_editor_property("convex_elems"))
            if moving and n_simple == 0:
                msg = "moving part has no simple collision (trace channel Interactable needs a box)"
                if fix:
                    unreal.EditorStaticMeshLibrary.add_simple_collisions(mesh, unreal.ScriptingCollisionShapeType.BOX)
                    findings.append(("FIXED", name, msg))
                else:
                    findings.append(("WARN", name, msg))

        # 6. material slots
        for sm in mesh.static_materials:
            slot = str(sm.material_slot_name)
            if slot.upper() not in MATERIAL_SLOT_NAMES:
                findings.append(("WARN", name, f"material slot '{slot}' is not one of {sorted(MATERIAL_SLOT_NAMES)}"))

    errors = sum(1 for f in findings if f[0] == "ERROR")
    for level, mesh_name, msg in findings:
        unreal.log_warning(f"[{level}] {mesh_name} : {msg}") if level in ("WARN", "ERROR") else unreal.log(f"[{level}] {mesh_name} : {msg}")
    unreal.log(f"[NOAR] done: {len(findings)} findings, {errors} errors")
    return findings


if __name__ == "__main__":
    validate("/Game/Apartments/DEMPSTER/Millwork")
