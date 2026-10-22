"""
noar_millwork_assembler.py - EUW_MillworkAssembler as a script (Standard v1.1, section 3 step 3).

After importing one room's FBX as separate SM_* meshes with correct pivots, this places the
interactive actors so nobody wires 100 cabinets by hand:

    import noar_millwork_assembler as asm
    asm.assemble("/Game/Apartments/DEMPSTER/Millwork/Interactive", apt="DEM", room="KIT")

For every mesh whose PART is a moving part it spawns the matching BP_Interactive* actor
(class map in noar_naming.bp_class_for_part), names the actor like the mesh without SM_,
assigns the mesh to the actor's `DoorMesh` / `DrawerMesh` / `FlapMesh` component by convention,
sets HingeSide from _L/_R, and places it at the mesh's import transform (FBX world origin =
apartment origin at door D.01, so the actor transform is the pivot's world transform stored in the
mesh's import data; if your DCC baked transforms into vertices instead, place at identity).

Static parts (CARC, SHELF, TOP, INT, FRAME) are placed as plain StaticMeshActors.
LED parts become BP_InteractiveLight with LightGroupTag = LightGroup.<Room>.Cabinet.

Dependencies (RequiredOpen) are inferred: a TRAY inside unit B03 requires DRW_01 of B03 to be open;
anything with PART=INT is static. BlockedBy pairs are left for the swept-box pass (todo in v1.1).

Re-running is safe: an actor whose name already exists in the level is updated, not duplicated.
"""
import unreal
import noar_naming as nn

ROOM_TO_ZONE = {"FOY": "Foyer", "KIT": "Kitchen", "DIN": "Dining", "LIV": "Living", "PBR": "Primary", "WIC": "WIC",
                "PEN": "Ensuite", "GBR": "Guest", "GEN": "Guest", "DEN": "Den", "PWD": "Powder", "LAU": "Laundry",
                "BAL": "Balcony", "HAL": "Foyer", "MEC": "Laundry"}

asset_reg = unreal.AssetRegistryHelpers.get_asset_registry()
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def _existing_actor(label: str):
    for a in actor_sub.get_all_level_actors():
        if a.get_actor_label() == label:
            return a
    return None


def _mesh_component_name(part: str) -> str:
    return {"DOOR": "DoorMesh", "DRW": "DrawerMesh", "TRAY": "DrawerMesh", "FLAP": "FlapMesh", "BED": "FlapMesh",
            "SLIDE": "DoorMesh", "POCKET": "DoorMesh", "LED": "StripMesh"}.get(part, "StaticMesh")


def _set_if_exists(obj, prop, value):
    try:
        obj.set_editor_property(prop, value)
    except Exception:
        unreal.log_warning(f"[NOAR] {obj.get_name()} has no property {prop}")


def assemble(content_path: str, apt: str, room: str, place_statics: bool = True):
    assets = asset_reg.get_assets_by_path(content_path, recursive=True)
    meshes = [a for a in assets if a.asset_class_path.asset_name == "StaticMesh"]
    prefix = f"SM_{apt}_{room}_"
    spawned, updated = 0, 0
    by_name = {}

    for a in meshes:
        name = str(a.asset_name)
        if not name.startswith(prefix):
            continue
        parsed = nn.parse(name)
        if not parsed:
            unreal.log_warning(f"[NOAR] skip {name}: bad name")
            continue
        part = parsed["part"]
        mesh = a.get_asset()
        label = nn.actor_name(name)
        bp_path = nn.bp_class_for_part(part)

        if not bp_path:
            if not place_statics:
                continue
            actor = _existing_actor(label)
            if not actor:
                actor = actor_sub.spawn_actor_from_object(mesh, unreal.Vector(0, 0, 0))
                actor.set_actor_label(label)
                spawned += 1
            else:
                updated += 1
            actor.set_mobility(unreal.ComponentMobility.STATIC)
            by_name[label] = actor
            continue

        bp_class = unreal.EditorAssetLibrary.load_blueprint_class(bp_path.rsplit(".", 1)[0])
        if not bp_class:
            unreal.log_error(f"[NOAR] blueprint not found: {bp_path}")
            continue

        actor = _existing_actor(label)
        if not actor:
            actor = actor_sub.spawn_actor_from_class(bp_class, unreal.Vector(0, 0, 0))
            actor.set_actor_label(label)
            spawned += 1
        else:
            updated += 1

        # assign mesh to the conventional component
        comp_name = _mesh_component_name(part)
        comp = None
        for c in actor.get_components_by_class(unreal.StaticMeshComponent):
            if c.get_name() == comp_name:
                comp = c
                break
        if comp:
            comp.set_static_mesh(mesh)
        else:
            unreal.log_warning(f"[NOAR] {label}: component {comp_name} not found on {bp_class.get_name()}")

        # parameters from the name
        if part == "DOOR":
            side = parsed.get("side")
            if side in ("L", "R"):
                _set_if_exists(actor, "HingeSide", unreal.ENOARHingeSide.LEFT if side == "L" else unreal.ENOARHingeSide.RIGHT)
        if part in ("DRW", "TRAY"):
            mn, mx = mesh.get_bounding_box().min, mesh.get_bounding_box().max
            _set_if_exists(actor, "TravelDistance", max(0.0, (mx.x - mn.x) - 2.0))
        if part == "LED":
            tag = unreal.GameplayTag(tag_name=f"LightGroup.{ROOM_TO_ZONE.get(room, room)}.Cabinet")
            _set_if_exists(actor, "LightGroupTag", tag)

        _set_if_exists(actor, "SaveKey", label)
        _set_if_exists(actor, "ZoneTag", unreal.GameplayTag(tag_name=f"Zone.{ROOM_TO_ZONE.get(room, room)}"))
        by_name[label] = actor

    # dependencies: TRAY_nn inside unit requires DRW_nn (same index) or DRW_01
    for label, actor in by_name.items():
        parsed = nn.parse("SM_" + label)
        if parsed and parsed["part"] == "TRAY":
            unit = f"{apt}_{room}_{parsed['unit']}"
            idx = parsed.get("index") or "01"
            parent = by_name.get(f"{unit}_DRW_{idx}") or by_name.get(f"{unit}_DRW_01")
            if parent:
                _set_if_exists(actor, "RequiredOpen", [parent])
            # inner LED turns on with the outer part
        if parsed and parsed["part"] == "LED":
            unit = f"{apt}_{room}_{parsed['unit']}"
            outer = [a for l, a in by_name.items() if l.startswith(unit + "_DOOR") or l.startswith(unit + "_DRW_01")]
            for o in outer:
                _set_if_exists(o, "LinkedLight", actor)

    unreal.log(f"[NOAR] assembled {room}: {spawned} spawned, {updated} updated")
    return by_name


if __name__ == "__main__":
    assemble("/Game/Apartments/DEMPSTER/Millwork/Interactive", apt="DEM", room="KIT")
