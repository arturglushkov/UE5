"""
noar_setup_level.py - creates the data assets, the thin Blueprint subclasses and the prototype level for one apartment.

    import noar_setup_level as lvl
    lvl.setup_all("/Game/Apartments/DEMPSTER", "<repo>/Data/Dempster")

Creates (idempotent, re-run safe):
  Data assets  <apt>/Data/DA_LightPreset_Day|Evening|Night, DA_Room_KIT|DIN, DA_Apartment_DEMPSTER, DT_FinishOptions_DEMPSTER (from CSV)
  Blueprints   /Game/NOAR/Blueprints/Core/BP_NOAR_GameMode (Apartment set), BP_NOAR_PlayerController, BP_NOAR_Character,
               /Game/NOAR/Blueprints/Interactive/BP_InteractiveDoor|Drawer|Flap|SlidingDoor|Pocket,
               /Game/NOAR/Blueprints/Lighting/BP_InteractiveLight_Downlight|LED|Pendant (+ light component), BP_LightSwitch, BP_DayNightManager
  Level        <apt>/Maps/L_DEM_Prototype: DirectionalLight (sun), SkyLight (real-time capture), SkyAtmosphere, ExponentialHeightFog,
               PostProcessVolume (unbound, Lumen, exposure), PlayerStart, NOARDayNightManager wired to all of them,
               World Settings GameMode override = BP_NOAR_GameMode.
Then: noar_millwork_assembler.assemble(...) and noar_spawn_lights.spawn(...) into this level, save, Play.

What this script cannot do from Python and is left for the editor (5 minutes, listed in Docs/Dempster_Prototype_Geometry.md):
  - assign Manny's skeletal mesh + ABP to BP_NOAR_Character (needs the Third Person content pack)
  - create the IA_* / IMC_* Enhanced Input assets and plug them into the character / controller
  - WBP_* widgets
"""
import os
import unreal

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

BP_ROOT = "/Game/NOAR/Blueprints"


def _set(obj, prop, value):
    try:
        obj.set_editor_property(prop, value)
        return True
    except Exception as e:  # noqa
        unreal.log_warning(f"[NOAR] {obj.get_name()}: cannot set {prop}: {e}")
        return False


def _cpp(name):
    return unreal.load_class(None, f"/Script/NOARCore.{name}")


# ---------------------------------------------------------------- blueprints
def _blueprint(folder, name, parent_class):
    path = f"{folder}/{name}"
    if eal.does_asset_exist(path):
        return eal.load_asset(path)
    f = unreal.BlueprintFactory()
    f.set_editor_property("parent_class", parent_class)
    bp = asset_tools.create_asset(name, folder, unreal.Blueprint, f)
    eal.save_loaded_asset(bp)
    return bp


def _add_component(bp, comp_class, comp_name):
    """Adds a component to a Blueprint's SCS (UE 5.x SubobjectDataSubsystem)."""
    try:
        sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
        root = sds.k2_gather_subobject_data_for_blueprint(bp)
        for h in root:
            d = sds.k2_find_subobject_data_from_handle(h)
            if str(unreal.SubobjectDataBlueprintFunctionLibrary.get_variable_name(d)) == comp_name:
                return True
        params = unreal.AddNewSubobjectParams(parent_handle=root[0], new_class=comp_class, blueprint_context=bp)
        handle, fail = sds.add_new_subobject(params)
        if fail and str(fail):
            unreal.log_warning(f"[NOAR] {bp.get_name()}: add {comp_name}: {fail}")
            return False
        sds.rename_subobject(handle, unreal.Text(comp_name))
        return True
    except Exception as e:  # noqa
        unreal.log_warning(f"[NOAR] {bp.get_name()}: cannot add component {comp_name} ({e}); add it by hand")
        return False


def build_blueprints(apartment_da):
    out = {}
    core = f"{BP_ROOT}/Core"
    out["GameMode"] = _blueprint(core, "BP_NOAR_GameMode", _cpp("NOARGameMode"))
    out["PlayerController"] = _blueprint(core, "BP_NOAR_PlayerController", _cpp("NOARPlayerController"))
    out["Character"] = _blueprint(core, "BP_NOAR_Character", _cpp("NOARCharacter"))
    inter = f"{BP_ROOT}/Interactive"
    for n, c in [("BP_InteractiveDoor", "NOARInteractiveDoor"), ("BP_InteractiveDrawer", "NOARInteractiveDrawer"),
                 ("BP_InteractiveFlap", "NOARInteractiveFlap"), ("BP_InteractiveSlidingDoor", "NOARInteractiveSlidingDoor"),
                 ("BP_InteractivePocket", "NOARInteractivePocket")]:
        out[n] = _blueprint(inter, n, _cpp(c))
    light = f"{BP_ROOT}/Lighting"
    for n, comp in [("BP_InteractiveLight_Downlight", unreal.SpotLightComponent), ("BP_InteractiveLight_LED", unreal.RectLightComponent),
                    ("BP_InteractiveLight_Pendant", unreal.PointLightComponent)]:
        bp = _blueprint(light, n, _cpp("NOARInteractiveLight"))
        _add_component(bp, comp, "Light")
        out[n] = bp
    out["BP_LightSwitch"] = _blueprint(light, "BP_LightSwitch", _cpp("NOARLightSwitch"))
    out["BP_DayNightManager"] = _blueprint(light, "BP_DayNightManager", _cpp("NOARDayNightManager"))
    out["BPV_CameraZone"] = _blueprint(f"{BP_ROOT}/Camera", "BPV_CameraZone", _cpp("NOARCameraZone"))

    # class defaults: GameMode.Apartment, default pawn / controller classes
    try:
        gm_cls = unreal.load_object(None, out["GameMode"].get_path_name() + "_C")
        cdo = unreal.get_default_object(gm_cls)
        _set(cdo, "apartment", apartment_da)
        _set(cdo, "default_pawn_class", unreal.load_object(None, out["Character"].get_path_name() + "_C"))
        _set(cdo, "player_controller_class", unreal.load_object(None, out["PlayerController"].get_path_name() + "_C"))
        unreal.BlueprintEditorLibrary.compile_blueprint(out["GameMode"])
        eal.save_loaded_asset(out["GameMode"])
    except Exception as e:  # noqa
        unreal.log_warning(f"[NOAR] set BP_NOAR_GameMode defaults by hand (Apartment, DefaultPawn, PlayerController): {e}")
    for bp in out.values():
        try:
            unreal.BlueprintEditorLibrary.compile_blueprint(bp)
            eal.save_loaded_asset(bp)
        except Exception:
            pass
    return out


# ---------------------------------------------------------------- data assets
def _data_asset(folder, name, cls):
    path = f"{folder}/{name}"
    if eal.does_asset_exist(path):
        return eal.load_asset(path)
    f = unreal.DataAssetFactory()
    f.set_editor_property("data_asset_class", cls)
    return asset_tools.create_asset(name, folder, unreal.DataAsset, f)


def _group_state(tag, on, dimmer=1.0):
    s = unreal.NOARLightGroupState()
    _set(s, "group", unreal.GameplayTag(tag_name=tag))
    _set(s, "on", on)
    _set(s, "dimmer", dimmer)
    return s


def build_data(apt_root, apt_id, data_dir):
    d = f"{apt_root}/Data"
    presets = []
    spec = [
        ("Day", unreal.ENOARLightingPreset.DAY, dict(sun_lux=75000.0, sun_elevation=38.0, sun_azimuth=120.0, sun_kelvin=5500.0, sky_light_intensity=1.0,
                                                     exposure_compensation=0.3, white_balance_kelvin=6000.0, backdrop_index=0),
         [("LightGroup.Kitchen.Cabinet", True), ("LightGroup.Dining.Cove", True, 0.6), ("LightGroup.Kitchen.Ambient", False), ("LightGroup.Dining.Ambient", False),
          ("LightGroup.Dining.Pendant", False), ("LightGroup.Dining.Accent", False), ("LightGroup.Kitchen.Cove", True, 0.6)]),
        ("Evening", unreal.ENOARLightingPreset.EVENING, dict(sun_lux=8000.0, sun_elevation=6.0, sun_azimuth=265.0, sun_kelvin=3200.0, sky_light_intensity=0.6,
                                                             exposure_compensation=0.6, white_balance_kelvin=5200.0, backdrop_index=1),
         [("LightGroup.Kitchen.Cabinet", True), ("LightGroup.Dining.Cove", True), ("LightGroup.Kitchen.Ambient", True, 0.7), ("LightGroup.Dining.Ambient", True, 0.6),
          ("LightGroup.Dining.Pendant", True, 0.8), ("LightGroup.Dining.Accent", True), ("LightGroup.Kitchen.Cove", True)]),
        ("Night", unreal.ENOARLightingPreset.NIGHT, dict(sun_lux=0.0, sun_elevation=-20.0, sun_azimuth=0.0, sun_kelvin=4000.0, sky_light_intensity=0.15,
                                                         exposure_compensation=0.9, white_balance_kelvin=4200.0, backdrop_index=2),
         [("LightGroup.Kitchen.Cabinet", True, 0.8), ("LightGroup.Dining.Cove", True, 0.7), ("LightGroup.Kitchen.Ambient", True, 0.5), ("LightGroup.Dining.Ambient", True, 0.4),
          ("LightGroup.Dining.Pendant", True), ("LightGroup.Dining.Accent", True, 0.8), ("LightGroup.Kitchen.Cove", True, 0.8)]),
    ]
    for name, enum, props, groups in spec:
        da = _data_asset(d, f"DA_LightPreset_{name}", _cpp("NOARLightPreset"))
        _set(da, "preset", enum)
        _set(da, "display_name", unreal.Text(name))
        for k, v in props.items():
            _set(da, k, v)
        _set(da, "light_groups", [_group_state(*g) for g in groups])
        eal.save_loaded_asset(da)
        presets.append(da)

    rooms = []
    for rid, disp, zone, loc, yaw, slots, order in [
        ("KIT", "Kitchen", "Zone.Kitchen", (330, 330, 0), 180.0, ["Configurable.CabinetFinish.Kitchen", "Configurable.Countertop.Kitchen"], 0),
        ("DIN", "Dining", "Zone.Dining", (450, 300, 0), -90.0, ["Configurable.WallPanel.Dining", "Configurable.FeatureStone.Dining", "Configurable.Paint.Walls"], 1),
    ]:
        da = _data_asset(d, f"DA_Room_{rid}", _cpp("NOARRoomData"))
        _set(da, "room_id", rid)
        _set(da, "display_name", unreal.Text(disp))
        _set(da, "zone_tag", unreal.GameplayTag(tag_name=zone))
        _set(da, "teleport_transform", unreal.Transform(unreal.Vector(*loc), unreal.Rotator(0, yaw, 0), unreal.Vector(1, 1, 1)))
        _set(da, "configurable_slots", [unreal.GameplayTag(tag_name=s) for s in slots])
        _set(da, "sort_order", order)
        eal.save_loaded_asset(da)
        rooms.append(da)

    table = None
    try:
        import noar_import_spec as spec
        table = spec.import_finish_options(d, os.path.join(data_dir, "finish_options.csv"), f"DT_FinishOptions_{apt_id}")
    except Exception as e:  # noqa
        unreal.log_warning(f"[NOAR] finish options table not imported: {e}")

    apt = _data_asset(d, f"DA_Apartment_{apt_id}", _cpp("NOARApartmentData"))
    _set(apt, "apartment_id", "DEM")
    _set(apt, "display_name", unreal.Text("Dempster Residence"))
    _set(apt, "address", unreal.Text("4714 N Flagler Dr, West Palm Beach, FL"))
    _set(apt, "design_version", "ID drawings 09.30.2026 / prototype")
    _set(apt, "rooms", rooms)
    _set(apt, "light_presets", presets)
    if table:
        _set(apt, "finish_options", table)
    _set(apt, "start_transform", unreal.Transform(unreal.Vector(450, 300, 95), unreal.Rotator(0, 180, 0), unreal.Vector(1, 1, 1)))
    _set(apt, "level", unreal.SoftObjectPath(f"{apt_root}/Maps/L_DEM_Prototype"))
    eal.save_loaded_asset(apt)
    return apt, presets


# ---------------------------------------------------------------- level
def _spawn(cls, label, loc=(0, 0, 0), rot=(0, 0, 0)):
    for a in actor_sub.get_all_level_actors():
        if a.get_actor_label() == label:
            return a
    a = actor_sub.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(*rot))
    a.set_actor_label(label)
    return a


def build_level(apt_root, apartment_da, presets, bps):
    map_path = f"{apt_root}/Maps/L_DEM_Prototype"
    if eal.does_asset_exist(map_path):
        level_sub.load_level(map_path)
    else:
        level_sub.new_level(map_path)

    sun = _spawn(unreal.DirectionalLight, "Sun", (0, 0, 800), (-38, 120, 0))
    sc = sun.get_component_by_class(unreal.DirectionalLightComponent)
    _set(sc, "intensity", 75000.0)   # lux; NOARDayNightManager overrides from the active preset
    _set(sc, "use_temperature", True); _set(sc, "temperature", 5500.0)
    _set(sc, "atmosphere_sun_light", True)
    _set(sc, "cast_shadows", True)
    _set(sc, "dynamic_shadow_distance_movable_light", 3000.0)
    _set(sc, "light_source_angle", 0.6)
    sun.set_mobility(unreal.ComponentMobility.MOVABLE)

    sky = _spawn(unreal.SkyLight, "SkyLight", (0, 0, 600))
    skc = sky.get_component_by_class(unreal.SkyLightComponent)
    _set(skc, "real_time_capture", True)
    _set(skc, "intensity", 1.0)
    sky.set_mobility(unreal.ComponentMobility.MOVABLE)
    _spawn(unreal.SkyAtmosphere, "SkyAtmosphere")
    fog = _spawn(unreal.ExponentialHeightFog, "HeightFog")
    fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    _set(fc, "fog_density", 0.005); _set(fc, "fog_height_falloff", 0.2)

    pp = _spawn(unreal.PostProcessVolume, "PP_Apartment", (345, 500, 150))
    _set(pp, "unbound", True)
    s = pp.get_editor_property("settings")
    for k, v in [("override_auto_exposure_method", True), ("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM),
                 ("override_auto_exposure_min_brightness", True), ("auto_exposure_min_brightness", 6.0),
                 ("override_auto_exposure_max_brightness", True), ("auto_exposure_max_brightness", 12.0),
                 ("override_auto_exposure_bias", True), ("auto_exposure_bias", 0.3),
                 ("override_dynamic_global_illumination_method", True), ("dynamic_global_illumination_method", unreal.DynamicGlobalIlluminationMethod.LUMEN),
                 ("override_reflection_method", True), ("reflection_method", unreal.ReflectionMethod.LUMEN),
                 ("override_lumen_scene_lighting_quality", True), ("lumen_scene_lighting_quality", 1.0),
                 ("override_lumen_final_gather_quality", True), ("lumen_final_gather_quality", 1.0),
                 ("override_lumen_scene_detail", True), ("lumen_scene_detail", 2.0),
                 ("override_vignette_intensity", True), ("vignette_intensity", 0.25),
                 ("override_bloom_intensity", True), ("bloom_intensity", 0.15),
                 ("override_white_temp", True), ("white_temp", 6000.0)]:
        _set(s, k, v)
    _set(pp, "settings", s)

    _spawn(unreal.PlayerStart, "PlayerStart_Dining", (450, 300, 95), (0, 180, 0))

    dn_cls = unreal.load_object(None, bps["BP_DayNightManager"].get_path_name() + "_C") or _cpp("NOARDayNightManager")
    dn = _spawn(dn_cls, "DayNightManager", (0, 0, 0))
    _set(dn, "sun", sun); _set(dn, "sky_light", sky); _set(dn, "post_process", pp); _set(dn, "presets", presets)

    ws = unreal.EditorLevelLibrary.get_editor_world().get_world_settings() if hasattr(unreal, "EditorLevelLibrary") else None
    if ws:
        gm = unreal.load_object(None, bps["GameMode"].get_path_name() + "_C") or _cpp("NOARGameMode")
        _set(ws, "default_game_mode", gm)
    level_sub.save_current_level()
    unreal.log(f"[NOAR] level ready: {map_path}")
    return map_path


def setup_all(apt_root="/Game/Apartments/DEMPSTER", data_dir=None, apt_id="DEMPSTER"):
    if data_dir is None:
        here = os.path.dirname(os.path.abspath(__file__))
        data_dir = os.path.abspath(os.path.join(here, "..", "..", "Data", "Dempster"))
    apt, presets = build_data(apt_root, apt_id, data_dir)
    bps = build_blueprints(apt)
    return build_level(apt_root, apt, presets, bps)


if __name__ == "__main__":
    setup_all()
