# NOAR UE5 Digital Apartment

Production-ready Unreal Engine 5.8 project framework for premium interactive digital twins of apartments (NOAR design studio). First apartment: **Dempster Residence**, West Palm Beach, FL.

This repository holds everything that is **text** and belongs under Git: project settings, the C++ core module, editor scripts, data tables and the standard itself. Binary content (`Content/**/*.uasset`, `*.umap`) lives in Perforce (see `Docs/NOAR_UE5_Digital_Apartment_Standard.md`, section 15) — if you must keep it here, `.gitattributes` already routes it through Git LFS.

## Layout

```
NOAR.uproject                  # UE 5.8, plugins list, NOARCore module
Config/
  DefaultEngine.ini            # RHI, Lumen, VSM, Nanite, TSR, Substrate, Lumen/VSM CVars, collision channel
  DefaultScalability.ini       # NOAR_LOW / MEDIUM / HIGH / ULTRA = scalability levels 1..4
  DefaultDeviceProfiles.ini    # Mac vs Windows overrides (texture pool, screen %, HWRT)
  DefaultGame.ini              # GameInstance / GameMode, Asset Manager primary assets, chunks
  DefaultInput.ini             # Enhanced Input classes
  DefaultGameplayTags.ini      # Interact.*, Zone.*, LightGroup.*, Configurable.*
  DefaultEditor.ini
Source/
  NOAR.Target.cs, NOAREditor.Target.cs
  NOARCore/                    # the framework in C++ (~4 800 lines); Blueprints are thin subclasses
    Public/NOARTypes.h               enums, data-table row, save struct
    Public/NOARSettings.h            Project Settings > Game > NOAR: every tunable from the standard
    Public/NOARFunctionLibrary.h     shipping guard, quality tiers, photo-mode overrides, HWRT/Mac detection
    Public/Interaction/              INOARInteractable, UNOARInteractionComponent, ANOARInteractiveBase,
                                     Door / Drawer / Flap / SlidingDoor / Pocket
    Public/Lighting/                 ANOARInteractiveLight, ANOARLightSwitch, UNOARLightingSubsystem, ANOARDayNightManager
    Public/Character/                ANOARCharacter (3P/1P, lag, fade, click-to-walk), ANOARCameraZone
    Public/Core/                     ANOARPlayerController, ANOARGameMode, UNOARGameInstance (+SaveGame), UNOARApartmentSubsystem
    Public/Configurator/             UNOARConfigurableSurfaceComponent, UNOARConfiguratorSubsystem
    Public/Data/                     UNOARApartmentData, UNOARRoomData, UNOARLightPreset, UNOARProductData
    Public/UI/                       UNOARHUDWidget (toolbar, prompt, hints, toast, panel host) + panels built in code:
                                     Finishes, Rooms, Lighting, Menu, Product, Photo overlay - no WBP_ needed to run
Scripts/Editor/                # UE Python editor utilities (run from Editor > Tools > Execute Python Script)
  noar_validate_export.py      # pivot / scale / naming validation for interactive millwork
  noar_import_spec.py          # CSV (designer spec) -> DA_Product / DT_FinishOptions
  noar_import_fbx.py           # imports Export/<Apt>/*.fbx with the Standard's settings (Nanite, pivots kept)
  noar_millwork_assembler.py   # places every mesh from placement.json and wraps moving parts in ANOARInteractive*
  noar_spawn_lights.py         # spawns ANOARInteractiveLight actors from lights.json (groups, lumens, Kelvin)
  noar_setup_materials.py      # M_Master_Opaque/Glass/Mirror/Emissive + MI_<ROLE> + finish library, assigned by slot name
  noar_setup_level.py          # DA_LightPreset/Room/Apartment, BP_* thin subclasses, L_DEM_Prototype (sun, sky, PP, DayNightManager)
  noar_setup_input.py          # IA_* / IMC_Walk|UI|Photo with key + gamepad mappings, wired into the BP class defaults
  noar_bootstrap_prototype.py  # ONE script: everything above in order -> press Play
Scripts/Geometry/              # Blender-as-module (pip install bpy) procedural geometry in UE conventions
  noar_geo.py                  # box/cabinet/door/drawer builders, UE<->Blender frame, UV box projection, bevels
  build_dempster_prototype.py  # kitchen + dining prototype -> Export/Dempster/*.fbx + placement.json + lights.json
  render_preview.py            # Workbench QA renders of the .blend
Data/Dempster/                 # CSV data tables from the ID drawings 09.30.2026 (D0.01-D0.05)
  Prototype/                   # placement.json / lights.json / DIMENSIONS_ASSUMED.md of the last geometry build
Export/                        # FBX / .blend (git-ignored; regenerate with Scripts/Geometry)
Docs/
  NOAR_UE5_Digital_Apartment_Standard.md   # the standard, v1.1 (exported from the living doc)
  ROADMAP.md                               # phases, gates, step checklist
  Dempster_Prototype_Geometry.md           # what the prototype geometry contains, how to import it, assumed dimensions
```

## Getting started

1. Install **Unreal Engine 5.8.x**. On macOS use **Xcode 26.1.1** (minimum 26.0; Xcode 26.4 is incompatible).
2. Clone this repo into the folder that will be the project root (the `.uproject` is at the root).
3. Right-click `NOAR.uproject` → *Generate project files* (Xcode / Visual Studio), build `NOAREditor` once (the C++ module is ~300 lines).
4. Open the project. In *Content Browser → Add → Add Feature or Content Pack* add **Third Person** (Blueprint) — this brings Manny, the ABP and the input assets the standard builds on. Move them into `Content/NOAR/Characters/` and rename per the naming convention.
5. Create the folders from Standard §2 and start Phase 1 (`Docs/ROADMAP.md`).

## Building NOARCore (first time)

1. Right-click `NOAR.uproject` → *Generate Xcode Project* (Mac) / *Generate Visual Studio project files* (Windows).
2. Open the generated workspace, select the `NOAREditor` scheme, build. First build compiles ~45 files; later builds are seconds.
3. If the build fails, paste the first 30 lines of errors into the chat — the code was written against the UE 5.8 API but never compiled on this machine, so one or two API renames are expected.
4. Open the project. Project Settings → Game → **NOAR** holds every tunable (interaction distance, speeds, camera, timings).

### Blueprints to create on top (thin subclasses, no logic)

| Blueprint | Parent | What you set |
| --- | --- | --- |
| `BP_NOAR_Character` | `ANOARCharacter` | Skeletal mesh (Manny), ABP, `WalkContext` + IA_* assets |
| `BP_NOAR_PlayerController` | `ANOARPlayerController` | `UIContext`, `PhotoContext`, IA_*; `HUDClass` (default: the C++ HUD; a `WBP_NOAR_HUD` subclass restyles it) |
| `BP_NOAR_GameMode` | `ANOARGameMode` | `Apartment` = DA_Apartment_DEMPSTER; set as the level's GameMode override |
| `BP_InteractiveDoor` / `Drawer` / `Flap` / `SlidingDoor` / `Pocket` | `ANOARInteractive*` | Sounds; per-instance mesh and parameters are set by `noar_millwork_assembler.py` |
| `BP_InteractiveLight_Downlight` / `_LED` / `_Pendant` | `ANOARInteractiveLight` | Add Spot / Rect / Point light components, strip mesh with `M_Master_Emissive` |
| `BP_LightSwitch` | `ANOARLightSwitch` | Plate + rocker meshes |
| `BP_DayNightManager` | `ANOARDayNightManager` | Place in level, assign Sun / SkyLight / PostProcess / Backdrop, 3 presets |
| `BPV_CameraZone` | `ANOARCameraZone` | Place in corridors / WIC / bathrooms |
| `DA_Apartment_DEMPSTER`, `DA_Room_*`, `DA_LightPreset_Day/Evening/Night`, `DA_Product_*` | data assets | Values from `Data/Dempster/*.csv` (`noar_import_spec.py`) |

## Rules in one screen

- One engine version for the whole project. Upgrading = separate test branch.
- `Content/NOAR/` is the framework, `Content/Apartments/<ID>/` is one apartment. Framework never references an apartment.
- Datasmith-imported assets are read-only; custom materials are duplicates in `NOAR/` or `Apartments/<ID>/Materials`.
- Every moving part is its own mesh with the pivot on the mechanism axis, X out of the cabinet, Z up, no negative/non-uniform scale. Run `noar_validate_export.py` after every import.
- No per-cabinet logic: `BP_InteractiveBase` + parameters. Communication only through `BPI_Interactable`, gameplay tags and the managers.
- Prototype room (kitchen + dining) must pass Gates 1–4 before anything else is imported.
