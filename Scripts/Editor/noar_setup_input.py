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
import noar_ue as ue

ACTIONS_DIR = "/Game/NOAR/Input/Actions"
CONTEXTS_DIR = "/Game/NOAR/Input/Contexts"
BP_CORE = "/Game/NOAR/Blueprints/Core"
eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

ACTIONS = {
    "IA_Move": unreal.InputActionValueType.AXIS2D, "IA_Look": unreal.InputActionValueType.AXIS2D, "IA_LookHold": unreal.InputActionValueType.BOOLEAN,
    "IA_FastWalk": unreal.InputActionValueType.BOOLEAN, "IA_Interact": unreal.InputActionValueType.BOOLEAN, "IA_ToggleCamera": unreal.InputActionValueType.BOOLEAN,
    "IA_Zoom": unreal.InputActionValueType.AXIS1D, "IA_KeyLook": unreal.InputActionValueType.AXIS2D, "IA_Menu": unreal.InputActionValueType.BOOLEAN, "IA_Finishes": unreal.InputActionValueType.BOOLEAN,
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


def build_actions():
    out = {}
    for name, vt in ACTIONS.items():
        ia = _data_asset(ACTIONS_DIR, name, unreal.InputAction)
        _set(ia, "value_type", vt)
        eal.save_loaded_asset(ia)
        out[name] = ia
    return out


def build_contexts(ia):
    """Mappings go through UNOARFunctionLibrary.add_key_mapping (C++): Python only gets a copy of MapKey's result,
    so modifiers set from Python would be lost."""
    lib = unreal.NOARFunctionLibrary

    def ctx(name):
        c = _data_asset(CONTEXTS_DIR, name, unreal.InputMappingContext)
        lib.clear_key_mappings(c)
        return c

    def m(c, action, key, swizzle=False, negate=False, dead_zone=0.0, scale=1.0):
        lib.add_key_mapping(c, ia[action], key, swizzle, negate, dead_zone, scale)

    def wasd(c):
        # IA_Move is Axis2D, X = right, Y = forward
        m(c, "IA_Move", "W", swizzle=True); m(c, "IA_Move", "S", swizzle=True, negate=True)
        m(c, "IA_Move", "D"); m(c, "IA_Move", "A", negate=True)
        m(c, "IA_Move", "Gamepad_Left2D", dead_zone=0.2)

    walk = ctx("IMC_Walk")
    wasd(walk)
    # arrows turn / tilt the view (trackpad users have no comfortable RMB-drag)
    m(walk, "IA_KeyLook", "Right"); m(walk, "IA_KeyLook", "Left", negate=True)
    m(walk, "IA_KeyLook", "Up", swizzle=True); m(walk, "IA_KeyLook", "Down", swizzle=True, negate=True)
    m(walk, "IA_Look", "Mouse2D")
    m(walk, "IA_Look", "Gamepad_Right2D", dead_zone=0.2, scale=2.5)
    m(walk, "IA_LookHold", "RightMouseButton")
    m(walk, "IA_FastWalk", "LeftShift"); m(walk, "IA_FastWalk", "Gamepad_LeftTrigger")
    m(walk, "IA_Interact", "LeftMouseButton"); m(walk, "IA_Interact", "E"); m(walk, "IA_Interact", "Gamepad_FaceButton_Bottom")
    m(walk, "IA_ToggleCamera", "V"); m(walk, "IA_ToggleCamera", "Gamepad_FaceButton_Top")
    m(walk, "IA_Zoom", "MouseWheelAxis"); m(walk, "IA_Zoom", "Gamepad_RightTrigger", negate=True); m(walk, "IA_Zoom", "Gamepad_LeftShoulder")
    for a, k, g in [("IA_Menu", "Escape", "Gamepad_Special_Right"), ("IA_Finishes", "F", "Gamepad_FaceButton_Left"), ("IA_Rooms", "R", "Gamepad_DPad_Up"),
                    ("IA_DayNight", "L", "Gamepad_DPad_Right"), ("IA_PhotoMode", "P", "Gamepad_DPad_Down"), ("IA_ProductInfo", "I", "Gamepad_FaceButton_Right")]:
        m(walk, a, k); m(walk, a, g)
    eal.save_loaded_asset(walk)

    ui = ctx("IMC_UI")
    m(ui, "IA_Menu", "Escape"); m(ui, "IA_Menu", "Gamepad_FaceButton_Right")
    eal.save_loaded_asset(ui)

    photo = ctx("IMC_Photo")
    m(photo, "IA_PhotoMode", "P"); m(photo, "IA_PhotoMode", "Escape"); m(photo, "IA_PhotoMode", "Gamepad_FaceButton_Right")
    m(photo, "IA_Look", "Mouse2D"); m(photo, "IA_Look", "Gamepad_Right2D", dead_zone=0.2)
    wasd(photo)
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
        for n in ("IA_Move", "IA_Look", "IA_LookHold", "IA_FastWalk", "IA_Interact", "IA_ToggleCamera", "IA_Zoom", "IA_KeyLook"):
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
