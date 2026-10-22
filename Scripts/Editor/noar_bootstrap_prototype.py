"""
noar_bootstrap_prototype.py - ONE call that turns the repo + Export/Dempster into a playable prototype level.

    Tools > Execute Python Script > Scripts/Editor/noar_bootstrap_prototype.py

Order: import FBX (skipped when already imported) -> materials -> data assets + Blueprints + level -> input assets
       -> place millwork/architecture from placement.json -> spawn lights from lights.json -> save.
Every step is idempotent and runs even if an earlier one failed. Everything the steps log (and any Python traceback)
is written to Saved/BuildLogs/bootstrap.log so the result can be reviewed without the Output Log.
"""
import os
import sys
import time
import traceback
import importlib

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
EXPORT = os.path.join(REPO, "Export", "Dempster")
APT_ROOT = "/Game/Apartments/DEMPSTER"
LOG = os.path.join(REPO, "Saved", "BuildLogs", "bootstrap.log")
if HERE not in sys.path:
    sys.path.append(HERE)

import unreal  # noqa: E402

os.makedirs(os.path.dirname(LOG), exist_ok=True)
_logf = open(LOG, "w", encoding="utf-8")


def _write(level, msg):
    _logf.write(f"{time.strftime('%H:%M:%S')} {level} {msg}\n")
    _logf.flush()


# mirror every unreal.log* call made by the NOAR scripts into the file
_orig = (unreal.log, unreal.log_warning, unreal.log_error)
unreal.log = lambda m: (_write("INFO ", m), _orig[0](m))[-1]
unreal.log_warning = lambda m: (_write("WARN ", m), _orig[1](m))[-1]
unreal.log_error = lambda m: (_write("ERROR", m), _orig[2](m))[-1]

mods = {}
for name in ("noar_import_fbx", "noar_setup_materials", "noar_setup_level", "noar_setup_input", "noar_millwork_assembler", "noar_spawn_lights", "noar_setup_collision"):
    try:
        mods[name] = importlib.reload(sys.modules[name]) if name in sys.modules else importlib.import_module(name)
    except Exception:
        _write("ERROR", f"import {name}:\n{traceback.format_exc()}")


def _geo():
    return mods["noar_import_fbx"].geometry_root(EXPORT, APT_ROOT)


def _already_imported():
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    found = any(a.asset_class_path.asset_name == "StaticMesh" for a in reg.get_assets_by_path(_geo() + "/Millwork", recursive=True))
    unreal.log(f"[NOAR] geometry {_geo()}: {'already imported' if found else 'importing'}")
    return found


def _report():
    sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = sub.get_all_level_actors()
    unreal.log(f"[NOAR] level actors: {len(actors)}")
    for a in actors:
        lab = a.get_actor_label()
        if lab in ("PlayerStart_Dining", "Sun", "DEM_ARCH_FLOOR_KITDIN", "DEM_KIT_I01_STONE") or a.get_class().get_name() == "PlayerStart":
            o, e = a.get_actor_bounds(False)
            unreal.log(f"[NOAR] {lab} [{a.get_class().get_name()}] loc={a.get_actor_location()} rot={a.get_actor_rotation()} bounds_origin={o} extent={e}")
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    for a in reg.get_assets_by_path(_geo(), recursive=True):
        n = str(a.asset_name)
        if a.asset_class_path.asset_name == "StaticMesh" and ("FLOOR_PLAN" in n or "T01_DOOR_01_L" in n):
            b = a.get_asset().get_bounding_box()
            unreal.log(f"[NOAR] mesh {n}: min={b.min} max={b.max}")


STEPS = [
    ("Import FBX", lambda: None if _already_imported() else mods["noar_import_fbx"].import_room(EXPORT, APT_ROOT)),
    ("Materials", lambda: mods["noar_setup_materials"].setup_all(APT_ROOT)),
    ("Collision", lambda: mods["noar_setup_collision"].setup(_geo())),
    ("Data assets, Blueprints, level", lambda: mods["noar_setup_level"].setup_all(APT_ROOT, os.path.join(REPO, "Data", "Dempster"))),
    ("Enhanced Input", lambda: mods["noar_setup_input"].setup_all()),
    ("Place millwork", lambda: mods["noar_millwork_assembler"].assemble(_geo(), os.path.join(EXPORT, "placement.json"), apt="DEM")),
    ("Lights", lambda: mods["noar_spawn_lights"].spawn(os.path.join(EXPORT, "lights.json"))),
    ("Report", _report),
    ("Save", lambda: (unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level(),
                      unreal.EditorAssetLibrary.save_directory(APT_ROOT, recursive=True),
                      unreal.EditorAssetLibrary.save_directory("/Game/NOAR", recursive=True))),
]

ok = 0
with unreal.ScopedSlowTask(len(STEPS), "NOAR prototype bootstrap") as task:
    task.make_dialog(True)
    for title, fn in STEPS:
        task.enter_progress_frame(1, title)
        _write("STEP ", f"---- {title}")
        try:
            fn()
            ok += 1
            _write("STEP ", f"---- {title}: OK")
        except Exception:
            _write("ERROR", f"{title} failed:\n{traceback.format_exc()}")

_write("DONE ", f"{ok}/{len(STEPS)} steps OK")
_logf.close()
unreal.log, unreal.log_warning, unreal.log_error = _orig
unreal.log(f"[NOAR] bootstrap: {ok}/{len(STEPS)} steps OK - see Saved/BuildLogs/bootstrap.log")
