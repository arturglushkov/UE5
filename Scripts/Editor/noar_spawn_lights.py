"""
noar_spawn_lights.py - spawns the light fixtures of a room export from lights.json (written by Scripts/Geometry/build_*.py).

    import noar_spawn_lights as sl
    sl.spawn("/Users/<you>/.../Export/Dempster/lights.json")

Each entry becomes a BP_InteractiveLight_<Type> actor (falls back to ANOARInteractiveLight + a light component added
here) with LightGroupTag, Lumens, Kelvin set from the JSON, so UNOARLightingSubsystem groups and the switches work
immediately. Point = pendant/chandelier bulb, Spot = downlight (IES in entry 'ies' if present under
/Game/NOAR/Lighting/IES/), Rect = LED strip / cove.

Re-running updates actors by label instead of duplicating them.
"""
import json
import unreal

CLASS_ROOT = "/Game/NOAR/Blueprints/Lighting"
BP_BY_TYPE = {"Spot": "BP_InteractiveLight_Downlight", "Rect": "BP_InteractiveLight_LED", "Point": "BP_InteractiveLight_Pendant"}
IES_ROOT = "/Game/NOAR/Lighting/IES"

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def _set(obj, prop, value):
    try:
        obj.set_editor_property(prop, value)
    except Exception as e:  # noqa
        unreal.log_warning(f"[NOAR] {obj.get_name()}: cannot set {prop}: {e}")


def _class_for(light_type):
    bp = BP_BY_TYPE.get(light_type, "BP_InteractiveLight_LED")
    if unreal.EditorAssetLibrary.does_asset_exist(f"{CLASS_ROOT}/{bp}"):
        c = unreal.load_class(None, f"{CLASS_ROOT}/{bp}.{bp}_C")
        if c:
            return c, True
    return unreal.load_class(None, "/Script/NOARCore.NOARInteractiveLight"), False


def _ensure_light_component(actor, light_type):
    """When the actor is the bare C++ class, add one light component of the right type."""
    existing = actor.get_components_by_class(unreal.LocalLightComponent)
    if existing:
        return existing[0]
    cls = {"Spot": unreal.SpotLightComponent, "Rect": unreal.RectLightComponent, "Point": unreal.PointLightComponent}[light_type]
    comp = actor.add_component_by_class(cls, False, unreal.Transform(), False)
    return comp


def spawn(lights_json, prefix="L_DEM_"):
    with open(lights_json, "r", encoding="utf-8") as f:
        data = json.load(f)
    existing = {a.get_actor_label(): a for a in actor_sub.get_all_level_actors()}
    n_new = n_upd = 0
    for e in data["lights"]:
        label = prefix + e["name"]
        t = e.get("type", "Spot")
        loc = unreal.Vector(*e["pos"])
        d = e.get("dir", [0, 0, -1])
        rot = unreal.Vector(d[0], d[1], d[2]).rotator() if hasattr(unreal.Vector, "rotator") else unreal.MathLibrary.conv_vector_to_rotator(unreal.Vector(d[0], d[1], d[2]))
        cls, is_bp = _class_for(t)
        actor = existing.get(label)
        if actor:
            actor.set_actor_location_and_rotation(loc, rot, False, False)
            n_upd += 1
        else:
            actor = actor_sub.spawn_actor_from_class(cls, loc, rot)
            actor.set_actor_label(label)
            n_new += 1
        _set(actor, "light_group_tag", unreal.GameplayTag(tag_name=e["group"]))
        _set(actor, "lumens", float(e.get("lumens", 700)))
        _set(actor, "kelvin", float(e.get("kelvin", 3000)))
        comp = _ensure_light_component(actor, t)
        if comp:
            _set(comp, "intensity_units", unreal.LightUnits.LUMENS)
            _set(comp, "intensity", float(e.get("lumens", 700)))
            _set(comp, "use_temperature", True)
            _set(comp, "temperature", float(e.get("kelvin", 3000)))
            _set(comp, "cast_shadows", t != "Rect")
            _set(comp, "attenuation_radius", 400.0 if t == "Rect" else 800.0)
            if t == "Spot":
                _set(comp, "outer_cone_angle", 40.0)
                _set(comp, "inner_cone_angle", 20.0)
                _set(comp, "source_radius", 3.0)
                ies = e.get("ies")
                if ies and unreal.EditorAssetLibrary.does_asset_exist(f"{IES_ROOT}/{ies}"):
                    _set(comp, "ies_texture", unreal.load_asset(f"{IES_ROOT}/{ies}"))
                    _set(comp, "use_ies_brightness", False)
            elif t == "Rect":
                w, h = e.get("size", [100, 1])
                _set(comp, "source_width", float(w))
                _set(comp, "source_height", float(h))
                _set(comp, "barn_door_angle", 88.0)
            elif t == "Point":
                _set(comp, "source_radius", float(e.get("radius", 5)))
    unreal.log(f"[NOAR] lights: {n_new} spawned, {n_upd} updated")


if __name__ == "__main__":
    import os
    here = os.path.dirname(os.path.abspath(__file__))
    spawn(os.path.join(here, "..", "..", "Export", "Dempster", "lights.json"))
