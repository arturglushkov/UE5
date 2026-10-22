"""
noar_millwork_assembler.py - EUW_MillworkAssembler as a script (Standard v1.1, section 3 step 3).

Places every mesh of one room export at its real world transform and wraps the moving parts in the right
BP_Interactive* / ANOARInteractive* actor, so nobody wires 100 cabinets by hand.

Source of truth for transforms is the `placement.json` written next to the FBX by
Scripts/Geometry/build_*.py (UE's static-mesh import discards the FBX node transforms, so every mesh
is imported at its pivot = local origin and the actor is placed here):

    import noar_millwork_assembler as asm
    asm.assemble("/Game/Apartments/DEMPSTER/Millwork", "/Users/<you>/.../Export/Dempster/placement.json", apt="DEM")

placement.json per object:
    "SM_DEM_KIT_T01_DOOR_01_L": {"mechanism": "HingedDoor", "open_angle": 105, "hinge": "L", "opens_inward": false,
                                 "location": [63.5, 150.0, 10.0], "yaw": 0.0, "collection": "Millwork", "materials": ["FRONT", ...]}
mechanisms: Static | HingedDoor | Drawer | FlapUp | FlapDown | SlidingDoor | Pocket | Light | Switch

Class per mechanism: a Blueprint if it exists (BP_Interactive<Mechanism> in CLASS_ROOT), otherwise the C++ class.
Re-running is safe: an actor whose label already exists is updated in place, not duplicated.
"""
import json
import os
import unreal
import noar_ue as ue

CLASS_ROOT = "/Game/NOAR/Blueprints/Interactive"
BP_BY_MECH = {
    "HingedDoor": ("BP_InteractiveDoor", "NOARInteractiveDoor"),
    "Drawer": ("BP_InteractiveDrawer", "NOARInteractiveDrawer"),
    "FlapUp": ("BP_InteractiveFlap", "NOARInteractiveFlap"),
    "FlapDown": ("BP_InteractiveFlap", "NOARInteractiveFlap"),
    "SlidingDoor": ("BP_InteractiveSlidingDoor", "NOARInteractiveSlidingDoor"),
    "Pocket": ("BP_InteractivePocket", "NOARInteractivePocket"),
    "Light": ("BP_InteractiveLight_LED", "NOARInteractiveLight"),
    "Switch": ("BP_LightSwitch", "NOARLightSwitch"),
}
MESH_COMPONENT = {"HingedDoor": "MovingMesh", "Drawer": "MovingMesh", "FlapUp": "MovingMesh", "FlapDown": "MovingMesh",
                  "SlidingDoor": "MovingMesh", "Pocket": "MovingMesh", "Light": "StripMesh", "Switch": "MovingMesh"}
ROOM_TO_ZONE = {"FOY": "Foyer", "KIT": "Kitchen", "DIN": "Dining", "LIV": "Living", "PBR": "Primary", "WIC": "WIC",
                "PEN": "Ensuite", "GBR": "Guest", "GEN": "Guest", "DEN": "Den", "PWD": "Powder", "LAU": "Laundry",
                "BAL": "Balcony", "HAL": "Foyer", "MEC": "Laundry", "ARCH": "Kitchen"}

asset_reg = unreal.AssetRegistryHelpers.get_asset_registry()
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
_actor_cache = None


def _existing_actor(label):
    global _actor_cache
    if _actor_cache is None:
        _actor_cache = {a.get_actor_label(): a for a in actor_sub.get_all_level_actors()}
    return _actor_cache.get(label)


def _set(obj, prop, value):
    try:
        obj.set_editor_property(prop, value)
        return True
    except Exception as e:  # noqa
        unreal.log_warning(f"[NOAR] {obj.get_name()}: cannot set {prop}: {e}")
        return False


def _class_for(mech):
    bp, cpp = BP_BY_MECH[mech]
    bp_path = f"{CLASS_ROOT}/{bp}.{bp}_C"
    if unreal.EditorAssetLibrary.does_asset_exist(f"{CLASS_ROOT}/{bp}"):
        c = unreal.load_class(None, bp_path)
        if c:
            return c
    return unreal.load_class(None, f"/Script/NOARCore.{cpp}")


def _find_meshes(content_path):
    """Asset name -> AssetData. The 5.8 Interchange FBX importer names a mesh after the exported mesh data, which
    Blender suffixes when modifiers are applied on export (SM_X -> SM_X_001); both spellings map to the same asset."""
    import re
    out = {}
    for a in asset_reg.get_assets_by_path(content_path, recursive=True):
        if a.asset_class_path.asset_name == "StaticMesh":
            n = str(a.asset_name)
            # legacy importer prefixes the FBX file name: SM_DEM_PROTO_Millwork_SM_DEM_KIT_... -> SM_DEM_KIT_...
            n = re.sub(r"^SM_[A-Z]{3}_PROTO_[A-Za-z]+_", "", n)
            out.setdefault(n, a)
            out.setdefault(re.sub(r"_\d{3}$", "", n), a)
    return out


def _transform(entry):
    loc = entry.get("location", [0, 0, 0])
    return unreal.Transform(unreal.Vector(*loc), ue.rot(yaw=entry.get("yaw", 0.0)), unreal.Vector(1, 1, 1))


def _room_of(name):
    parts = name.split("_")
    return parts[2] if len(parts) > 2 else "KIT"


def _unit_of(name):
    """SM_DEM_KIT_T01_DOOR_01_L -> DEM_KIT_T01"""
    parts = name.split("_")
    return "_".join(parts[1:4]) if len(parts) > 4 else None


def assemble(content_path, placement_json, apt="DEM", rooms=None, place_statics=True):
    global _actor_cache
    _actor_cache = None
    with open(placement_json, "r", encoding="utf-8") as f:
        data = json.load(f)
    objects = data["objects"]
    meshes = _find_meshes(content_path)
    spawned = updated = missing = 0
    actors = {}

    for name, entry in objects.items():
        if not name.startswith(f"SM_{apt}_"):
            continue
        room = _room_of(name)
        if rooms and room not in rooms:
            continue
        mesh_asset = meshes.get(name)
        if not mesh_asset:
            missing += 1
            unreal.log_warning(f"[NOAR] mesh not imported: {name}")
            continue
        mesh = mesh_asset.get_asset()
        label = name[3:]
        mech = entry.get("mechanism", "Static")
        xf = _transform(entry)

        if mech == "Static":
            if not place_statics:
                continue
            actor = _existing_actor(label)
            if not actor:
                actor = actor_sub.spawn_actor_from_object(mesh, xf.translation, xf.rotation.rotator())
                actor.set_actor_label(label)
                spawned += 1
            else:
                actor.set_actor_transform(xf, False, False)
                updated += 1
            for c in actor.get_components_by_class(unreal.StaticMeshComponent):
                c.set_static_mesh(mesh)
                break
            ue.set_mobility(actor, unreal.ComponentMobility.STATIC)
            actors[label] = (actor, entry)
            continue

        cls = _class_for(mech)
        if not cls:
            unreal.log_error(f"[NOAR] no class for {mech} ({name})")
            continue
        actor = _existing_actor(label)
        if not actor:
            actor = actor_sub.spawn_actor_from_class(cls, xf.translation, xf.rotation.rotator())
            actor.set_actor_label(label)
            spawned += 1
        else:
            actor.set_actor_transform(xf, False, False)
            updated += 1

        comp_name = MESH_COMPONENT[mech]
        comp = next((c for c in actor.get_components_by_class(unreal.StaticMeshComponent) if c.get_name() == comp_name), None)
        if comp:
            comp.set_static_mesh(mesh)
        else:
            unreal.log_warning(f"[NOAR] {label}: component {comp_name} not found on {cls.get_name()}")

        if mech == "HingedDoor":
            _set(actor, "open_angle", float(entry.get("open_angle", 105)))
            _set(actor, "hinge_side", ue.enum("ENOARHingeSide").LEFT if entry.get("hinge", "L") == "L" else ue.enum("ENOARHingeSide").RIGHT)
            _set(actor, "opens_inward", bool(entry.get("opens_inward", False)))
        elif mech == "Drawer":
            _set(actor, "travel_distance", float(entry.get("travel", 0.0)))
        elif mech in ("FlapUp", "FlapDown"):
            _set(actor, "mode", ue.enum("ENOARFlapMode").LIFT_UP if mech == "FlapUp" else ue.enum("ENOARFlapMode").DROP_DOWN)
            _set(actor, "open_angle", float(entry.get("open_angle", 90)))
        elif mech == "SlidingDoor":
            _set(actor, "travel_distance", float(entry.get("travel", 0.0)))
        elif mech == "Light":
            _set(actor, "light_group_tag", ue.tag(entry.get("group", f"LightGroup.{ROOM_TO_ZONE.get(room, room)}.Cabinet")))
            _set(actor, "lumens", float(entry.get("lumens", 300)))
            _set(actor, "kelvin", float(entry.get("kelvin", 3000)))
        elif mech == "Switch":
            _set(actor, "light_group_tags", [ue.tag(t) for t in entry.get("groups", [])])
            plate = entry.get("plate")
            if plate and plate in meshes:
                pc = next((c for c in actor.get_components_by_class(unreal.StaticMeshComponent) if c.get_name() == "PlateMesh"), None)
                if pc:
                    pc.set_static_mesh(meshes[plate].get_asset())

        if mech not in ("Light", "Switch"):
            _set(actor, "save_key", label)
            _set(actor, "zone_tag", ue.tag(f"Zone.{ROOM_TO_ZONE.get(room, room)}"))
            _set(actor, "blocks_pawn_when_closed", bool(entry.get("blocks_pawn", False)))
        actors[label] = (actor, entry)

    # dependencies declared in placement.json ("requires": ["SM_..."]) + LED <-> outer part linking by unit
    for label, (actor, entry) in actors.items():
        req = [actors[r[3:]][0] for r in entry.get("requires", []) if r[3:] in actors]
        if req:
            _set(actor, "required_open", req)
        if entry.get("mechanism") == "Light" and entry.get("link_to_doors", False):
            unit = _unit_of("SM_" + label)
            if unit:
                for l2, (a2, e2) in actors.items():
                    if l2.startswith(unit + "_DOOR") and e2.get("mechanism") == "HingedDoor":
                        _set(a2, "linked_light", actor)

    # configurable surfaces: add UNOARConfigurableSurfaceComponent to static actors flagged in placement.json
    for label, (actor, entry) in actors.items():
        slot_tag = entry.get("configurable")
        if not slot_tag:
            continue
        existing = actor.get_components_by_class(unreal.NOARConfigurableSurfaceComponent)
        comp = existing[0] if existing else ue.add_instance_component(actor, unreal.NOARConfigurableSurfaceComponent, "ConfigurableSurface")
        if comp:
            _set(comp, "slot_tag", ue.tag(slot_tag))
            _set(comp, "material_slot_name", entry.get("slot", "FRONT"))

    # remove actors from earlier geometry versions that placement.json no longer lists (e.g. prototype walls replaced by the plan shell)
    wanted = {n[3:] for n in objects}
    removed = 0
    for a in actor_sub.get_all_level_actors():
        lab = a.get_actor_label()
        if lab.startswith(f"{apt}_") and lab not in wanted:
            actor_sub.destroy_actor(a)
            removed += 1
    unreal.log(f"[NOAR] assembled {apt}: {spawned} spawned, {updated} updated, {missing} meshes missing, {removed} stale removed")
    return actors


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    assemble("/Game/Apartments/DEMPSTER", os.path.join(here, "..", "..", "Export", "Dempster", "placement.json"), apt="DEM")
