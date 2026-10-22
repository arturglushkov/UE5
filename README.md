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
  NOARCore/                    # tiny C++ module: enums, structs, native gameplay tags, settings, shipping guards
Scripts/Editor/                # UE Python editor utilities (run from Editor > Tools > Execute Python Script)
  noar_validate_export.py      # pivot / scale / naming validation for interactive millwork
  noar_import_spec.py          # CSV (designer spec) -> DA_Product / DT_FinishOptions
  noar_millwork_assembler.py   # spawns BP_Interactive* actors from imported SM_* meshes by suffix
Data/Dempster/                 # CSV data tables from the ID drawings 09.30.2026 (D0.01-D0.05)
Docs/
  NOAR_UE5_Digital_Apartment_Standard.md   # the standard, v1.1 (exported from the living doc)
  ROADMAP.md                               # phases, gates, step checklist
```

## Getting started

1. Install **Unreal Engine 5.8.x**. On macOS use **Xcode 26.1.1** (minimum 26.0; Xcode 26.4 is incompatible).
2. Clone this repo into the folder that will be the project root (the `.uproject` is at the root).
3. Right-click `NOAR.uproject` → *Generate project files* (Xcode / Visual Studio), build `NOAREditor` once (the C++ module is ~300 lines).
4. Open the project. In *Content Browser → Add → Add Feature or Content Pack* add **Third Person** (Blueprint) — this brings Manny, the ABP and the input assets the standard builds on. Move them into `Content/NOAR/Characters/` and rename per the naming convention.
5. Create the folders from Standard §2 and start Phase 1 (`Docs/ROADMAP.md`).

## Rules in one screen

- One engine version for the whole project. Upgrading = separate test branch.
- `Content/NOAR/` is the framework, `Content/Apartments/<ID>/` is one apartment. Framework never references an apartment.
- Datasmith-imported assets are read-only; custom materials are duplicates in `NOAR/` or `Apartments/<ID>/Materials`.
- Every moving part is its own mesh with the pivot on the mechanism axis, X out of the cabinet, Z up, no negative/non-uniform scale. Run `noar_validate_export.py` after every import.
- No per-cabinet logic: `BP_InteractiveBase` + parameters. Communication only through `BPI_Interactable`, gameplay tags and the managers.
- Prototype room (kitchen + dining) must pass Gates 1–4 before anything else is imported.
