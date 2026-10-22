"""
noar_setup_input.py - creates the Enhanced Input assets of Standard v1.1 section 7 and plugs them into
BP_NOAR_Character / BP_NOAR_PlayerController class defaults.

    import noar_setup_input as inp
    inp.setup_all()

/Game/NOAR/Input/Actions:  IA_Move (Axis2D), IA_Look (Axis2D), IA_LookHold (bool), IA_FastWalk, IA_Interact, IA_ToggleCamera,
                           IA_Zoom (Axis1D), IA_Menu, IA_Finishes, IA_Rooms, IA_DayNight, IA_PhotoMode, IA_ProductInfo
/Game/NOAR/Input/Contexts: IMC_Walk (priority 0), IMC_UI (10), IMC_Photo (5)
Mappings: WASD + arrows + gamepad left stick -> Move; mouse XY + right stick -> Look; RMB -> LookHold (3P orbit);
          Shift / LT -> FastWalk; LMB / E / gamepad A -> Interact; V / Y -> ToggleCamera; wheel / RT-LT -> Zoom;
          Esc / Start -> Menu; F -> Finishes; R -> Rooms; L -> DayNight; P -> PhotoMode; I -> ProductInfo.
If the SkeletalMesh for BP_NOAR_Character is missing (Third Person pack not added yet) the character still moves as a capsule.
"""
import unreal

ACTIONS_DIR = "/Game/NOAR/Input/Actions"
CONTEXTS_DIR = "/Game/NOAR/Input/Contexts"
BP_CORE = "/Game/NOAR/Blueprints/Core"
eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

ACTIONS = {
    "IA_Move": unreal.InputActionValueType.AXIS2D, "IA_Look": unreal.InputActionValueType.AXIS2D, "IA_LookHold": unreal.InputActionValueType.BOOLEAN,
    "IA_FastWalk": unreal.InputActionValueType.BOOLEAN, "IA_Interact": unreal.InputActionValueType.BOOLEAN, "IA_ToggleCamera": unreal.InputActionValueType.BOOLEAN,
    "IA_Zoom": unreal.InputActionValueType.AXIS1D, "IA_Menu": unreal.InputActionValueType.BOOLEAN, "IA_Finishes": unreal.InputActionValueType.BOOLEAN,
    "IA_Rooms": unreal.InputActionValueType.BOOLEAN, "IA_DayNight": unreal.InputActionValueType.BOOLEAN, "IA_PhotoMode": unreal.InputActionValueType.BOOLEAN,
    "IA_ProductInfo": unreal.InputActionValueType.BOOLEAN,
}


def _set(obj, prop, value):
    try:
        obj.set_editor_property(prop, value)
    except Exception as e:  # noqa
        unreal.log_warning(f"[NOAR] {obj.get_name()}: cannot set {prop}: {e}")


def _data_asset(folder, name, cls):
    path = f"{folder}/{name}"
    if eal.does_asset_exist(path):
        return eal.load_asset(path)
    f = unreal.DataAssetFactory()
    f.set_editor_property("data_asset_class", cls)
    return asset_tools.create_asset(name, folder, unreal.DataAsset, f)


def _mod(cls, **props):
    m = unreal.new_object(cls)
    for k, v in props.items():
        _set(m, k, v)
    return m


def _map(imc, action, key, modifiers=(), triggers=()):
    m = imc.map_key(action, unreal.Key(key_name=key)) if hasattr(imc, "map_key") else None
    if m is None:
        unreal.log_warning(f"[NOAR] {imc.get_name()}: map_key not available - add {key} -> {action.get_name()} by hand")
        return
    if modifiers:
        _set(m, "modifiers", list(modifiers))
    if triggers:
        _set(m, "triggers", list(triggers))


def build_actions():
    out = {}
    for name, vt in ACTIONS.items():
        ia = _data_asset(ACTIONS_DIR, name, unreal.InputAction)
        _set(ia, "value_type", vt)
        eal.save_loaded_asset(ia)
        out[name] = ia
    return out


def build_contexts(ia):
    neg = lambda: _mod(unreal.InputModifierNegate)  # noqa
    swz = lambda: _mod(unreal.InputModifierSwizzleAxis, order=unreal.InputAxisSwizzle.YXZ)  # noqa
    dz = lambda: _mod(unreal.InputModifierDeadZone, lower_threshold=0.2)  # noqa

    walk = _data_asset(CONTEXTS_DIR, "IMC_Walk", unreal.InputMappingContext)
    _set(walk, "mappings", [])
    # Move: W/S -> Y (forward), A/D -> X
    _map(walk, ia["IA_Move"], "W", [swz()]); _map(walk, ia["IA_Move"], "S", [swz(), neg()])
    _map(walk, ia["IA_Move"], "D"); _map(walk, ia["IA_Move"], "A", [neg()])
    _map(walk, ia["IA_Move"], "Up", [swz()]); _map(walk, ia["IA_Move"], "Down", [swz(), neg()])
    _map(walk, ia["IA_Move"], "Right"); _map(walk, ia["IA_Move"], "Left", [neg()])
    _map(walk, ia["IA_Move"], "Gamepad_Left2D", [dz()])
    _map(walk, ia["IA_Look"], "Mouse2D")
    _map(walk, ia["IA_Look"], "Gamepad_Right2D", [dz(), _mod(unreal.InputModifierScalar, scalar=unreal.Vector(2.5, 2.5, 1.0))])
    _map(walk, ia["IA_LookHold"], "RightMouseButton")
    _map(walk, ia["IA_FastWalk"], "LeftShift"); _map(walk, ia["IA_FastWalk"], "Gamepad_LeftTrigger")
    _map(walk, ia["IA_Interact"], "LeftMouseButton"); _map(walk, ia["IA_Interact"], "E"); _map(walk, ia["IA_Interact"], "Gamepad_FaceButton_Bottom")
    _map(walk, ia["IA_ToggleCamera"], "V"); _map(walk, ia["IA_ToggleCamera"], "Gamepad_FaceButton_Top")
    _map(walk, ia["IA_Zoom"], "MouseWheelAxis"); _map(walk, ia["IA_Zoom"], "Gamepad_RightTrigger", [neg()])
    _map(walk, ia["IA_Zoom"], "Gamepad_LeftShoulder")
    for a, k, g in [("IA_Menu", "Escape", "Gamepad_Special_Right"), ("IA_Finishes", "F", "Gamepad_FaceButton_Left"), ("IA_Rooms", "R", "Gamepad_DPad_Up"),
                    ("IA_DayNight", "L", "Gamepad_DPad_Right"), ("IA_PhotoMode", "P", "Gamepad_DPad_Down"), ("IA_ProductInfo", "I", "Gamepad_FaceButton_Right")]:
        _map(walk, ia[a], k); _map(walk, ia[a], g)
    eal.save_loaded_asset(walk)

    ui = _data_asset(CONTEXTS_DIR, "IMC_UI", unreal.InputMappingContext)
    _set(ui, "mappings", [])
    _map(ui, ia["IA_Menu"], "Escape"); _map(ui, ia["IA_Menu"], "Gamepad_FaceButton_Right")
    eal.save_loaded_asset(ui)

    photo = _data_asset(CONTEXTS_DIR, "IMC_Photo", unreal.InputMappingContext)
    _set(photo, "mappings", [])
    _map(photo, ia["IA_PhotoMode"], "P"); _map(photo, ia["IA_PhotoMode"], "Escape"); _map(photo, ia["IA_PhotoMode"], "Gamepad_FaceButton_Right")
    _map(photo, ia["IA_Look"], "Mouse2D"); _map(photo, ia["IA_Look"], "Gamepad_Right2D", [dz()])
    _map(photo, ia["IA_Move"], "W", [swz()]); _map(photo, ia["IA_Move"], "S", [swz(), neg()]); _map(photo, ia["IA_Move"], "D"); _map(photo, ia["IA_Move"], "A", [neg()])
    _map(photo, ia["IA_Move"], "Gamepad_Left2D", [dz()])
    eal.save_loaded_asset(photo)
    return {"IMC_Walk": walk, "IMC_UI": ui, "IMC_Photo": photo}


def wire_blueprints(ia, imc):
    def cdo(bp_name):
        p = f"{BP_CORE}/{bp_name}"
        if not eal.does_asset_exist(p):
            unreal.log_warning(f"[NOAR] {p} missing - run noar_setup_level first")
            return None, None
        bp = eal.load_asset(p)
        cls = unreal.load_object(None, bp.get_path_name() + "_C")
        return bp, unreal.get_default_object(cls)

    bp, c = cdo("BP_NOAR_Character")
    if c:
        _set(c, "walk_context", imc["IMC_Walk"])
        for n in ("IA_Move", "IA_Look", "IA_LookHold", "IA_FastWalk", "IA_Interact", "IA_ToggleCamera", "IA_Zoom"):
            _set(c, n.lower(), ia[n])
        unreal.BlueprintEditorLibrary.compile_blueprint(bp); eal.save_loaded_asset(bp)
    bp, c = cdo("BP_NOAR_PlayerController")
    if c:
        _set(c, "ui_context", imc["IMC_UI"]); _set(c, "photo_context", imc["IMC_Photo"])
        for n in ("IA_Menu", "IA_Finishes", "IA_Rooms", "IA_DayNight", "IA_PhotoMode", "IA_ProductInfo"):
            _set(c, n.lower(), ia[n])
        unreal.BlueprintEditorLibrary.compile_blueprint(bp); eal.save_loaded_asset(bp)


def setup_all():
    ia = build_actions()
    imc = build_contexts(ia)
    wire_blueprints(ia, imc)
    unreal.log("[NOAR] input assets ready")


if __name__ == "__main__":
    setup_all()
