"""
noar_ue.py - small helpers for the UE 5.8 Python API quirks the NOAR scripts hit:
  - C++ enums are exposed WITHOUT the 'E' prefix (ENOARHingeSide -> unreal.NOARHingeSide)
  - FGameplayTag / FKey cannot be built with keyword args; they are built from text
  - unreal.Rotator positional order is (roll, pitch, yaw), so we always pass keywords
"""
import unreal


def enum(name):
    """enum("ENOARHingeSide") -> unreal.NOARHingeSide (falls back to the E-prefixed name)."""
    short = name[1:] if name.startswith("E") else name
    e = getattr(unreal, short, None) or getattr(unreal, name, None)
    if e is None:
        raise AttributeError(f"enum {name} not exposed to Python (is NOARCore compiled and loaded?)")
    return e


def tag(name):
    """FGameplayTag from its string. Requests the tag through the engine so an unknown tag is reported, not silent."""
    t = unreal.GameplayTag()
    try:
        t.import_text(f'(TagName="{name}")')
    except Exception:
        try:
            t.set_editor_property("tag_name", name)
        except Exception as e:  # noqa
            unreal.log_warning(f"[NOAR] cannot build gameplay tag {name}: {e}")
    return t


def key(name):
    """FKey from its name ('W', 'LeftMouseButton', 'Gamepad_Left2D')."""
    k = unreal.Key()
    try:
        k.import_text(name)
    except Exception:
        try:
            k.import_text(f'(KeyName="{name}")')
        except Exception:
            k.set_editor_property("key_name", name)
    return k


def rot(pitch=0.0, yaw=0.0, roll=0.0):
    return unreal.Rotator(roll=float(roll), pitch=float(pitch), yaw=float(yaw))


def set_mobility(actor, mobility):
    """Actors don't expose set_mobility in Python (only AStaticMeshActor does); set it on the root component."""
    try:
        actor.set_mobility(mobility)
        return
    except Exception:
        pass
    root = actor.get_editor_property("root_component")
    if root:
        root.set_mobility(mobility)


def add_instance_component(actor, comp_class, name):
    """Adds a component to a placed actor through the editor Subobject subsystem (AActor.add_component_by_class is not
    exposed to Python). Returns the new component or None."""
    try:
        sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
        handles = sds.k2_gather_subobject_data_for_instance(actor)
        if not handles:
            return None
        params = unreal.AddNewSubobjectParams(parent_handle=handles[0], new_class=comp_class, blueprint_context=None)
        handle, fail = sds.add_new_subobject(params)
        if fail and str(fail):
            unreal.log_warning(f"[NOAR] {actor.get_actor_label()}: add {name}: {fail}")
        try:
            sds.rename_subobject(handle, unreal.Text(name))
        except Exception:
            pass
        comps = actor.get_components_by_class(comp_class)
        return comps[-1] if comps else None
    except Exception as e:  # noqa
        unreal.log_warning(f"[NOAR] {actor.get_actor_label()}: cannot add {name}: {e}")
        return None
