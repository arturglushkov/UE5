"""
noar_bootstrap_prototype.py - ONE call that turns the repo + Export/Dempster into a playable prototype level.

    Tools > Execute Python Script > Scripts/Editor/noar_bootstrap_prototype.py
or in the Output Log:
    py "<repo>/Scripts/Editor/noar_bootstrap_prototype.py"

Order: import FBX -> master materials + role instances assigned by slot name -> data assets + Blueprints + level -> input assets
       -> place millwork/architecture from placement.json -> spawn lights from lights.json -> save.
Every step is idempotent; re-run after regenerating the geometry. Requires NOARCore to be compiled first.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
EXPORT = os.path.join(REPO, "Export", "Dempster")
APT_ROOT = "/Game/Apartments/DEMPSTER"
if HERE not in sys.path:
    sys.path.append(HERE)

import unreal
import noar_import_fbx, noar_setup_materials, noar_setup_level, noar_setup_input, noar_millwork_assembler, noar_spawn_lights  # noqa: E402

with unreal.ScopedSlowTask(7, "NOAR prototype bootstrap") as task:
    task.make_dialog(True)
    task.enter_progress_frame(1, "Import FBX")
    noar_import_fbx.import_room(EXPORT, APT_ROOT)
    task.enter_progress_frame(1, "Materials")
    noar_setup_materials.setup_all(APT_ROOT)
    task.enter_progress_frame(1, "Data assets, Blueprints, level")
    noar_setup_level.setup_all(APT_ROOT, os.path.join(REPO, "Data", "Dempster"))
    task.enter_progress_frame(1, "Enhanced Input")
    noar_setup_input.setup_all()
    task.enter_progress_frame(1, "Place millwork")
    noar_millwork_assembler.assemble(APT_ROOT, os.path.join(EXPORT, "placement.json"), apt="DEM")
    task.enter_progress_frame(1, "Lights")
    noar_spawn_lights.spawn(os.path.join(EXPORT, "lights.json"))
    task.enter_progress_frame(1, "Save")
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    unreal.EditorAssetLibrary.save_directory(APT_ROOT, recursive=True)
    unreal.EditorAssetLibrary.save_directory("/Game/NOAR", recursive=True)
unreal.log("[NOAR] bootstrap done - press Play in L_DEM_Prototype")
