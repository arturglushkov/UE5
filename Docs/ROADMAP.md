# NOAR Digital Apartment — Roadmap

Prototype room = **Dempster kitchen + dining**. Nothing else is imported until Gates 1–4 pass there.
Gate criteria are fixed before the phase starts and accepted by the studio designer, not the developer.

## Gates

| Gate | Criterion |
| --- | --- |
| G1 Visual quality | Screenshot next to the reference video frame and the designer's render; Ground Truth: Path Tracer of the same hero shot vs Lumen real-time |
| G2 Camera feel | 5 minutes of walking without irritation, camera never inside a wall; avatar decided (Manny / MetaHuman / custom) with GPU and shadow cost measured |
| G3 Interaction | A person outside the team opens a door within 10 s without a hint |
| G4 Performance | `LS_PerfRoute` on DEV MAC / CLIENT STANDARD PC / HIGH-END RTX, plus Lumen Lite on Mac; avg and 1% low recorded |
| G5 Client experience | Test with two people outside the team |

## Phases (one technical artist; a parallel 3D artist halves Phase 9)

- [ ] **1. Empty room prototype** (3 d) — project from this repo, folders, Datasmith shell of kitchen+dining, windows, HDRI backdrop
- [ ] **2. Lighting** (4 d) — sun/sky, Lumen, VSM, exposure, `PP_NOAR_MASTER`, 10 fixtures → **G1** (temp materials)
- [ ] **3. Materials** (6 d) — 7 masters, hero slabs and oak, glass, mirror, metal → **G1** again
- [ ] **4. Character + camera** (4 d) — `BP_NOAR_Character`, spring arm, 1P, ABP, camera zones; avatar decision → **G2**
- [ ] **5. Door interaction** (5 d) — `BPI_Interactable`, `BPC_Interaction`, `BP_InteractiveBase`, Door, highlight, prompt, sound
- [ ] **6. Drawers + mechanisms** (5 d) — Drawer, Flap (HK), dependencies, occlusion, appliances, `noar_millwork_assembler.py` → **G3**
- [ ] **7. Lighting interaction** (3 d) — `BP_InteractiveLight`, `BP_LightSwitch`, `BP_LightingManager`, `BP_DayNightManager`
- [ ] **8. Material configurator** (5 d) — DA/DT, `BP_MaterialConfigurator`, `WBP_ConfigPanel`, SaveGame, Reset → **G4 (prototype)**
- [ ] **9. Full apartment** (15–20 d) — all rooms, ~100 fronts/drawers, furniture, decor, all fixtures, teleport → G1–G3 per room
- [ ] **10. Optimization** (5 d) — budgets, scalability, PSO cache, Mac profile → **G4 final**
- [ ] **11. Client UI** (6 d) — splash, HUD, rooms + floor plan, settings, photo mode, EN/RU, audio, final avatar → **G5**
- [ ] **12. Final build** (4 d) — Shipping Win/Mac, signing, installer, MRQ stills + hero video, Pixel Streaming test

## Step list

1. [ ] Create project from this repo (UE 5.8.x, Xcode 26.1.1 on Mac), build NOARCore, add Third Person content pack, connect Perforce, first submit
2. [ ] Rendering configured (this repo's Config), device profiles verified on Mac and Windows
3. [ ] Import prototype room (Datasmith shell, collision shell, tinted window glass, HDRI backdrop, static millwork FBX)
4. [ ] Lumen set (sun/sky, PP master, exposure, 10 fixtures from `Data/Dempster/light_fixtures.csv`) → G1
5. [ ] Master materials + MI for M01, LAM01, LAM02, SS01, SS02, GL01, GL02, PT01, HW01; request slab photos from Ciot/Marbella → G1
6. [ ] Character (`UNOARSettings` values), `IMC_NOAR_Walk`, ABP with BS_1D + turn-in-place
7. [ ] Camera (lag, collision recovery, `BPV_CameraZone`, 1P blend, character fade); avatar decision → G2
8. [ ] Interaction interface + component + `PP_Highlight` + prompt + `Interactable` channel
9. [ ] `BP_InteractiveBase` + soft-close curve + `DA_SoundSet`; `BP_InteractiveDoor` L/R; first 3 fronts validated with `noar_validate_export.py`
10. [ ] `BP_InteractiveDrawer` + TRAY dependency; `BP_InteractiveFlap` (HK); appliances; assembler → G3
11. [ ] Lighting system (LED, switches, manager, Day/Evening)
12. [ ] Configurator (`DT_FinishOptions_DEMPSTER` from `Data/Dempster/finish_options.csv`, `BPC_ConfigurableSurface`, panel, SaveGame, Reset)
13. [ ] Optimize prototype: `LS_PerfRoute`, stat unit / ProfileGPU, budgets, PSO cache → G4
14. [ ] Approve quality: demo to designer + one client; fixes; tag `v0.5-prototype`; archive
15. [ ] Scale to full apartment: Datasmith all rooms; millwork room by room (WIC → primary → ensuite → guest → den → powder → laundry → foyer → living); furniture, decor; fixtures L01–L15; `DA_Room` × 13; mirrors (3-level scheme)
16. [ ] Full-apartment gates G1–G3 per room; defect list in tracker
17. [ ] Optimization pass on three profiles; texture pool per machine → G4 final
18. [ ] Client UI (splash, HUD, rooms + floor plan, settings, photo mode, product cards, EN/RU, audio, final avatar) → G5
19. [ ] Cinematics: `LS_DEM_Hero_30s`, 2 reels, 12 stills 4K/6K via MRQ
20. [ ] Final client build: Shipping Windows (installer) + macOS (notarized .dmg), PSO cache, clean-machine test, Pixel Streaming from office PC, archive `v1.0`, backup

## v1.1 candidates

- `NOAR_ULTRA_RTX` profile with MegaLights (Production Ready in 5.8) — benchmark vs VSM scheme on `LS_PerfRoute`
- `NOAR_MAC_EXPERIMENTAL` (HWRT Lumen + MegaLights on Apple Silicon M2+, experimental) — tests only, never shipped
- Adaptive Substrate GBuffer for hero fabrics after profiling
- Interactive floor plan overlay (D1.00 texture + player marker)
- Primary Asset Labels / per-apartment chunks when the second apartment arrives
- MANUFACTURER kinematics for hero mechanisms (Blum/Hawa trajectories)
