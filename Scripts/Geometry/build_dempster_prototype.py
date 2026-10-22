"""
build_dempster_prototype.py - PROTOTYPE ROOM (kitchen + dining + living stub + balcony) of the Dempster Residence,
modelled from the ID drawings 09.30.2026 (D1.00, D1.01, D2.01, D2.02) with the Blender Python module.

    python3 -m pip install bpy        # Blender 4.2+/5.x as a module
    python3 Scripts/Geometry/build_dempster_prototype.py  [out_dir]

Outputs (out_dir, default Export/Dempster):
    SM_DEM_PROTO_Architecture.fbx   shell: floor, walls, ceilings, soffits, baseboards, balcony, glazing frames
    SM_DEM_PROTO_Millwork.fbx       carcasses, fronts, drawers, flaps, island, display, feature wall (one object per part)
    SM_DEM_PROTO_Furniture.fbx      table, chairs, chandelier, appliances, fixtures
    placement.json                  UE world transform per object (the FBX also carries it; UE 'import meshes' drops it)
    lights.json                     fixture positions / types / groups for noar_spawn_lights.py
    DIMENSIONS_ASSUMED.md           every dimension that is NOT on the drawings, for review

UE frame: X east, Y north, Z up, cm; origin = SW interior corner of the kitchen/dining floor (hall-side wall x
south wall, west wall). Door D.01 / vestibule are north-west of this and not in the prototype.
"""
import sys, os, json, math
import bpy

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import noar_geo as g
import noar_plan_shell

PLAN = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "Data", "Dempster", "plan_dempster.json")

OUT = sys.argv[-1] if len(sys.argv) > 1 and not sys.argv[-1].endswith(".py") else os.path.join(os.getcwd(), "Export", "Dempster")
os.makedirs(OUT, exist_ok=True)

ASSUMED = []   # (what, value, why)
PLACEMENT = {}
LIGHTS = []


def note(what, value, why):
    ASSUMED.append((what, value, why))


def inch(ft, inches=0.0):
    return (ft * 12 + inches) * 2.54


# ======================================================================= dimensions from the drawings (cm)
ROOM_W = inch(22, 8)          # 691  south wall, D2.01 / D2.02-01
ROOM_D = inch(17, 0.5)        # 519  west wall, D2.01
KIT_N_W = inch(10, 9.75)      # 330  kitchen north wall run, D2.01 / D2.02-03
CEIL = inch(9, 6)             # 290  finished ceiling, elevations
CEIL_KIT = inch(8, 6)         # 259  kitchen dropped ceiling, D1.01
CEIL_DIN = inch(9, 3)         # 282  dining dropped ceiling, D1.01
WALL_T = 15.0
note("Wall thickness", "15 cm", "not on ID drawings (architectural set); typical condo partition")

# south feature wall (D2.02-01): 2'-11 1/4" | 10'-4 1/4" feature | 3'-7 1/2" opening | 5'-9" tall M01 cabinets
S_A = inch(2, 11.25)          # 89
S_FEAT = inch(10, 4.25)       # 316
S_OPEN = inch(3, 7.5)         # 110
S_CAB = inch(5, 9)            # 175
assert abs(S_A + S_FEAT + S_OPEN + S_CAB - ROOM_W) < 1.0

# west wall (D2.02-02): 4'-11" (hall door zone) + 12'-1 1/2" existing tall cabinetry
W_DOORZONE = inch(4, 11)      # 150
W_TALL = inch(12, 1.5)        # 369
TALL_D = inch(2, 1)           # 63.5 tall cabinet depth (D2.01)
TALL_H = inch(7, 2.25)        # 219 existing tall units (D2.02-02)
HALL_DOOR_W = 89.8            # D.01 36" (gap measured on D1.00)
HALL_DOOR_H = inch(8, 0)      # D.01 96" (door schedule D0.01)
note("Hall door height", "7'-6\"", "door schedule gives 90\" for most interior doors")

# north wall (D2.02-03): 2'-1" end panel | 3'-11 1/2" existing tall | 4'-2" display | 7"
N_EXIST = inch(3, 11.5)       # 120
N_DISP = inch(4, 2)           # 127
N_END = inch(0, 7)            # 18

# island (D2.02-02): 11'-9" long, 3'-0" high, stone clad; aisle to tall run 3'-11 1/2" (D2.01)
ISL_L = inch(11, 9)           # 358
ISL_H = inch(3, 0)            # 91
ISL_W = inch(3, 6)            # 107
note("Island width", "3'-6\" incl. 12\" seating overhang on the dining side", "plan gives length only")
AISLE = inch(3, 11.5)         # 120
ISL_X0 = TALL_D + AISLE       # 183.5 west face
ISL_Y0 = W_DOORZONE + 10      # 160 start, aligned ~with cabinetry run
note("Island position", "west face 120 cm from tall run; south end 160 cm from south wall", "D2.01 aisle dim; N-S position read off plan")

# dining table (furniture plan): 8 seats
TBL_L, TBL_W, TBL_H = inch(10, 0), inch(3, 6), 76.0
note("Dining table", "10'-0\" x 3'-6\" x 30\"", "furniture plan shows 8 chairs; size estimated")
TBL_CX = ISL_X0 + ISL_W + 150 + TBL_W / 2 + 60   # ~ 450
TBL_CY = ROOM_D / 2

# balcony (D1.00): 13'-9" deep along the east side
BAL_D = inch(13, 9)           # 419
RAIL_H = 107.0

# living stub north of dining (real room is 16'-7" x 16'-11"; we open the dining north side into a 3.6 x 5 m stub)
LIV_D = 500.0
note("Living room", "stub 361 x 500 cm north of dining (real 505 x 516)", "prototype scope; full room in Phase 9")


# ======================================================================= shell
def build_shell():
    col = "Architecture"
    mat_wall, mat_ceil, mat_floor = "PAINT", "CEILING", "FLOOR"
    X1, Y1 = ROOM_W, ROOM_D

    if not os.path.exists(PLAN):
        # floor (kitchen + dining) + living stub, base ceiling - the plan shell builds these for the whole apartment
        g.make_box("SM_DEM_ARCH_FLOOR_KITDIN", (0, 0, -2), (X1, Y1, 0), mat_floor, col=col, bevel_mm=0)
        g.make_box("SM_DEM_ARCH_FLOOR_LIV", (KIT_N_W, Y1, -2), (X1, Y1 + LIV_D, 0), mat_floor, col=col, bevel_mm=0)
        g.make_box("SM_DEM_ARCH_CEIL_BASE", (0, 0, CEIL), (X1, Y1 + LIV_D, CEIL + 2), mat_ceil, col=col, bevel_mm=0)
    # kitchen dropped ceiling 8'-6" over the kitchen zone (x 0..330)
    g.make_box("SM_DEM_ARCH_SOFFIT_KIT", (0, 0, CEIL_KIT), (KIT_N_W, Y1, CEIL), mat_ceil, col=col, bevel_mm=2)
    # kitchen ceiling feature MD01: three layered painted MDF strips (PT03 Iron Mountain accent) - D1.01 / precedent
    for i, (xa, xb) in enumerate([(40, 120), (140, 220), (240, 310)]):
        g.make_box(f"SM_DEM_ARCH_CEIL_STRIP_{i+1:02d}", (xa, 30, CEIL_KIT - 4), (xb, Y1 - 30, CEIL_KIT), "METAL_BLACK", col=col, bevel_mm=1)
    note("Kitchen ceiling feature MD01", "3 strips 80 cm wide, 4 cm drop", "detail D3.00 not in the set")
    # dining dropped ceiling 9'-3" with perimeter cove (LED L02 in the recess)
    g.make_box("SM_DEM_ARCH_SOFFIT_DIN", (KIT_N_W + 30, 30, CEIL_DIN), (X1 - 30, Y1 - 30, CEIL), mat_ceil, col=col, bevel_mm=2)
    g.make_box("SM_DEM_ARCH_SOFFIT_DIN_RIM", (KIT_N_W, 0, CEIL - 12), (X1, Y1, CEIL), mat_ceil, col=col, bevel_mm=2)
    note("Dining cove", "30 cm perimeter recess, 8 cm deep, LED on the inner lip", "ceiling detail D3.00 not in the set")

    DOOR_Y0 = 0.0     # D.01 sits in the SW corner of the kitchen (D1.00)
    if os.path.exists(PLAN):
        noar_plan_shell.build(PLAN, CEIL, note)
        g.make_box("SM_DEM_ARCH_DRAPERY_POCKET", (X1 - 30, 0, CEIL - 1), (X1, Y1, CEIL + 0.5), mat_ceil, col=col, bevel_mm=0)
        for i, (ya, yb) in enumerate([(0, 70), (450, Y1)]):
            g.make_box(f"SM_DEM_ARCH_DRAPE_{i+1:02d}", (X1 - 22, ya, 2), (X1 - 10, yb, CEIL - 2), "FABRIC", col=col, bevel_mm=0)
        _hall_door(col)
        return

    # ---- fallback (no plan json): prototype shell with assumed walls
    # walls (15 cm, height CEIL)
    # south wall: opening to hall at x S_A+S_FEAT .. +S_OPEN, full height to 230
    OPEN_X0 = S_A + S_FEAT
    OPEN_H = 230.0
    note("Hall opening height", "230 cm", "D2.02-01 'OPEN TO HALL' height 8'-5 1/4\" scaled")
    g.make_box("SM_DEM_ARCH_WALL_S_A", (-WALL_T, -WALL_T, 0), (OPEN_X0, 0, CEIL + 2), mat_wall, col=col, bevel_mm=3)
    g.make_box("SM_DEM_ARCH_WALL_S_B", (OPEN_X0 + S_OPEN, -WALL_T, 0), (X1 + WALL_T, 0, CEIL + 2), mat_wall, col=col, bevel_mm=3)
    g.make_box("SM_DEM_ARCH_WALL_S_LINTEL", (OPEN_X0, -WALL_T, OPEN_H), (OPEN_X0 + S_OPEN, 0, CEIL + 2), mat_wall, col=col, bevel_mm=3)
    # west wall with hall door opening
    g.make_box("SM_DEM_ARCH_WALL_W_A", (-WALL_T, -WALL_T, 0), (0, DOOR_Y0, CEIL + 2), mat_wall, col=col, bevel_mm=3)
    g.make_box("SM_DEM_ARCH_WALL_W_B", (-WALL_T, DOOR_Y0 + HALL_DOOR_W, 0), (0, Y1 + WALL_T, CEIL + 2), mat_wall, col=col, bevel_mm=3)
    g.make_box("SM_DEM_ARCH_WALL_W_LINTEL", (-WALL_T, DOOR_Y0, HALL_DOOR_H), (0, DOOR_Y0 + HALL_DOOR_W, CEIL + 2), mat_wall, col=col, bevel_mm=3)
    # north wall of kitchen (x 0..330) and the stub's west wall (x=330, y 519..1019)
    g.make_box("SM_DEM_ARCH_WALL_N_KIT", (-WALL_T, Y1, 0), (KIT_N_W, Y1 + WALL_T, CEIL + 2), mat_wall, col=col, bevel_mm=3)
    g.make_box("SM_DEM_ARCH_WALL_LIV_W", (KIT_N_W - WALL_T, Y1 + WALL_T, 0), (KIT_N_W, Y1 + LIV_D + WALL_T, CEIL + 2), mat_wall, col=col, bevel_mm=3)
    g.make_box("SM_DEM_ARCH_WALL_LIV_N", (KIT_N_W - WALL_T, Y1 + LIV_D, 0), (X1 + WALL_T, Y1 + LIV_D + WALL_T, CEIL + 2), mat_wall, col=col, bevel_mm=3)
    # east wall: floor-to-ceiling glazing to the balcony: dining y 60..460 (sliding), living y 560..980 (fixed + slider)
    GL_H = 260.0
    note("Balcony glazing", "floor-to-ceiling 260 cm, sliders 100 cm panels", "existing condo glazing, no dims in the set")
    segs = [(-WALL_T, 60), (460, Y1 + 40), (Y1 + 440, Y1 + LIV_D + WALL_T)]
    for i, (ya, yb) in enumerate(segs):
        g.make_box(f"SM_DEM_ARCH_WALL_E_{i+1:02d}", (X1, ya, 0), (X1 + WALL_T, yb, CEIL + 2), mat_wall, col=col, bevel_mm=3)
    for i, (ya, yb) in enumerate([(60, 460), (Y1 + 40, Y1 + 440)]):
        g.make_box(f"SM_DEM_ARCH_WALL_E_HEAD_{i+1:02d}", (X1, ya, GL_H), (X1 + WALL_T, yb, CEIL + 2), mat_wall, col=col, bevel_mm=3)
        # glazing: black aluminium frame + glass panels
        n = int(round((yb - ya) / 100))
        pw = (yb - ya) / n
        for k in range(n):
            y0 = ya + k * pw
            g.make_parts(f"SM_DEM_ARCH_GLAZ_{i+1:02d}_{k+1:02d}", [
                ((X1 + 4, y0, 0), (X1 + 9, y0 + 4, GL_H), "METAL_BLACK"),
                ((X1 + 4, y0 + pw - 4, 0), (X1 + 9, y0 + pw, GL_H), "METAL_BLACK"),
                ((X1 + 4, y0, 0), (X1 + 9, y0 + pw, 4), "METAL_BLACK"),
                ((X1 + 4, y0, GL_H - 4), (X1 + 9, y0 + pw, GL_H), "METAL_BLACK"),
                ((X1 + 6, y0 + 4, 4), (X1 + 7, y0 + pw - 4, GL_H - 4), "GLASS"),
            ], col=col, bevel_mm=0.5)

    # hall stubs (2 m deep) behind the south opening and the west door, so no view opens onto the void
    HALL_D = 200.0
    g.make_box("SM_DEM_ARCH_HALL_S_FLOOR", (OPEN_X0 - 60, -WALL_T - HALL_D, -2), (OPEN_X0 + S_OPEN + 60, -WALL_T, 0), mat_floor, col=col, bevel_mm=0)
    g.make_box("SM_DEM_ARCH_HALL_S_CEIL", (OPEN_X0 - 60, -WALL_T - HALL_D, 244), (OPEN_X0 + S_OPEN + 60, -WALL_T, 246), mat_ceil, col=col, bevel_mm=0)
    g.make_parts("SM_DEM_ARCH_HALL_S_WALLS", [
        ((OPEN_X0 - 60 - WALL_T, -WALL_T - HALL_D, 0), (OPEN_X0 - 60, -WALL_T, 246), mat_wall),
        ((OPEN_X0 + S_OPEN + 60, -WALL_T - HALL_D, 0), (OPEN_X0 + S_OPEN + 60 + WALL_T, -WALL_T, 246), mat_wall),
        ((OPEN_X0 - 60 - WALL_T, -WALL_T - HALL_D - WALL_T, 0), (OPEN_X0 + S_OPEN + 60 + WALL_T, -WALL_T - HALL_D, 246), mat_wall)], col=col, bevel_mm=3)
    g.make_box("SM_DEM_ARCH_HALL_W_FLOOR", (-WALL_T - HALL_D, DOOR_Y0 - 40, -2), (-WALL_T, DOOR_Y0 + HALL_DOOR_W + 40, 0), mat_floor, col=col, bevel_mm=0)
    g.make_box("SM_DEM_ARCH_HALL_W_CEIL", (-WALL_T - HALL_D, DOOR_Y0 - 40, 244), (-WALL_T, DOOR_Y0 + HALL_DOOR_W + 40, 246), mat_ceil, col=col, bevel_mm=0)
    g.make_parts("SM_DEM_ARCH_HALL_W_WALLS", [
        ((-WALL_T - HALL_D, DOOR_Y0 - 40 - WALL_T, 0), (-WALL_T, DOOR_Y0 - 40, 246), mat_wall),
        ((-WALL_T - HALL_D, DOOR_Y0 + HALL_DOOR_W + 40, 0), (-WALL_T, DOOR_Y0 + HALL_DOOR_W + 40 + WALL_T, 246), mat_wall),
        ((-WALL_T - HALL_D - WALL_T, DOOR_Y0 - 40 - WALL_T, 0), (-WALL_T - HALL_D, DOOR_Y0 + HALL_DOOR_W + 40 + WALL_T, 246), mat_wall)], col=col, bevel_mm=3)
    note("Hall stubs", "2 m deep corridors behind the south opening and the west door, 244 cm ceiling", "hall is outside prototype scope (Phase 9)")

    # baseboards 10 x 1.5 (existing, PT10 repainted)
    bb = [((0, 0, 0), (OPEN_X0, 1.5, 10)), ((OPEN_X0 + S_OPEN, 0, 0), (X1, 1.5, 10)),
          ((0, DOOR_Y0 + HALL_DOOR_W, 0), (1.5, W_DOORZONE, 10)),
          ((X1 - 1.5, 0, 0), (X1, 60, 10)), ((X1 - 1.5, 460, 0), (X1, Y1 + 40, 10)),
          ((KIT_N_W, Y1, 0), (KIT_N_W + 1.5, Y1 + LIV_D, 10)), ((KIT_N_W, Y1 + LIV_D - 1.5, 0), (X1, Y1 + LIV_D, 10))]
    g.make_parts("SM_DEM_ARCH_BASEBOARD", [(a, b, "PAINT") for a, b in bb], col=col, bevel_mm=1)

    # balcony slab, railing, drapery pocket (12" recessed double drapery pocket, D1.01)
    g.make_box("SM_DEM_ARCH_BALCONY_SLAB", (X1 + WALL_T, -WALL_T, -20), (X1 + WALL_T + BAL_D, Y1 + LIV_D + WALL_T, -1), "CONCRETE", col=col, bevel_mm=0)
    g.make_parts("SM_DEM_ARCH_BALCONY_RAIL", [
        ((X1 + WALL_T + BAL_D - 6, -WALL_T, -1), (X1 + WALL_T + BAL_D, Y1 + LIV_D + WALL_T, RAIL_H), "GLASS"),
        ((X1 + WALL_T + BAL_D - 8, -WALL_T, RAIL_H), (X1 + WALL_T + BAL_D + 2, Y1 + LIV_D + WALL_T, RAIL_H + 4), "METAL_BLACK"),
    ], col=col, bevel_mm=1)
    g.make_box("SM_DEM_ARCH_DRAPERY_POCKET", (X1 - 30, 0, CEIL - 1), (X1, Y1 + LIV_D, CEIL + 2), mat_ceil, col=col, bevel_mm=0)
    # drapery FB01 (static, closed at the ends)
    for i, (ya, yb) in enumerate([(0, 70), (450, Y1), (Y1, Y1 + 50), (Y1 + 450, Y1 + LIV_D)]):
        g.make_box(f"SM_DEM_ARCH_DRAPE_{i+1:02d}", (X1 - 22, ya, 2), (X1 - 10, yb, CEIL - 2), "FABRIC", col=col, bevel_mm=0)

    _hall_door(col)


def _hall_door(col, DOOR_Y0=0.0):
    # entry door D.01 (swing 36" x 96"): interactive, pivot on the hinge jamb at y=DOOR_Y0, opens out into the shared hall (-X)
    frame = [((-WALL_T - 1, DOOR_Y0 - 5, 0), (1, DOOR_Y0, HALL_DOOR_H + 5), "WOOD"),
             ((-WALL_T - 1, DOOR_Y0 + HALL_DOOR_W, 0), (1, DOOR_Y0 + HALL_DOOR_W + 5, HALL_DOOR_H + 5), "WOOD"),
             ((-WALL_T - 1, DOOR_Y0 - 5, HALL_DOOR_H), (1, DOOR_Y0 + HALL_DOOR_W + 5, HALL_DOOR_H + 5), "WOOD")]
    g.make_parts("SM_DEM_ARCH_HALLDOOR_FRAME", frame, col=col, bevel_mm=2)
    # leaf: faces +X (into the kitchen), hinge on the south jamb: viewer in kitchen looks -X, his left is... local frame yaw 0:
    # local +X = east (into room). Hinge at y = DOOR_Y0 -> door extends +Y -> side "L". Opens inward toward the hall (-X) -> bOpensInward.
    g.make_parts("SM_DEM_KIT_D01_DOOR_01_L", g.door_parts(HALL_DOOR_W, HALL_DOOR_H, "L", "WOOD", handle=False) + [
        ((-4.5, HALL_DOOR_W - 10, 100), (6.5, HALL_DOOR_W - 8.5, 101.5), "STAINLESS")],  # lever HW01 placeholder
        pivot=(-WALL_T / 2 - 2.2, DOOR_Y0 + 1, 0), yaw=0, col="Millwork", bevel_mm=1)
    PLACEMENT["SM_DEM_KIT_D01_DOOR_01_L"] = dict(mechanism="HingedDoor", open_angle=90, hinge="L", opens_inward=True, blocks_pawn=True)


# ======================================================================= kitchen millwork
def unit(name_base, front_center, yaw, width, height, depth, doors, shelves=2, mat_front="FRONT", mat_carc="CARCASS", toe=True, zone="KIT"):
    """
    One cabinet. front_center = UE world (x, y, z) of the front plane, left end (local y=0) at floor.
    doors: list of dicts {w, side, h?, z0?, kind: 'door'|'drawer'|'flap'|'flap_down'|'static', name}
    """
    g.make_parts(f"{name_base}_CARC", g.carcass_parts(width, height, depth, mat_carc, "INTERIOR", shelves, toe), pivot=front_center, yaw=yaw, col="Millwork", bevel_mm=0.5)
    PLACEMENT[f"{name_base}_CARC"] = dict(mechanism="Static")
    y = 0.0
    for d in doors:
        w = d.get("w", width)
        z0 = d.get("z0", g.TOE if toe else 0.0)
        h = d.get("h", height - z0)
        kind = d.get("kind", "door")
        nm = f"{name_base}_{d['name']}"
        if kind == "door":
            side = d.get("side", "L")
            py = y if side == "L" else y + w
            g.make_parts(nm, g.door_parts(w, h, side, mat_front), pivot=_world(front_center, yaw, (0, py, z0)), yaw=yaw, col="Millwork", bevel_mm=1)
            PLACEMENT[nm] = dict(mechanism="HingedDoor", open_angle=d.get("angle", 105), hinge=side)
        elif kind == "drawer":
            body_d = depth - 4.0
            g.make_parts(nm, g.drawer_parts(w, h, body_d, mat=mat_front), pivot=_world(front_center, yaw, (-body_d, y + w / 2, z0)), yaw=yaw, col="Millwork", bevel_mm=1)
            PLACEMENT[nm] = dict(mechanism="Drawer", travel=body_d - 2.0)
        elif kind in ("flap", "flap_down"):
            # lift-up: pivot on top edge; drop-down: pivot on bottom edge
            if kind == "flap":
                parts = [((0, 0, -h + g.GAP), (g.T_FRONT, w - g.GAP, 0), mat_front)]
                pz = z0 + h
            else:
                parts = [((0, 0, 0), (g.T_FRONT, w - g.GAP, h - g.GAP), mat_front)]
                pz = z0
            g.make_parts(nm, parts, pivot=_world(front_center, yaw, (0, y, pz)), yaw=yaw, col="Millwork", bevel_mm=1)
            PLACEMENT[nm] = dict(mechanism="FlapUp" if kind == "flap" else "FlapDown", open_angle=d.get("angle", 90))
        elif kind == "static":
            g.make_parts(nm, [((0, 0, 0), (g.T_FRONT, w - g.GAP, h - g.GAP), d.get("mat", mat_front))], pivot=_world(front_center, yaw, (0, y, z0)), yaw=yaw, col="Millwork", bevel_mm=1)
            PLACEMENT[nm] = dict(mechanism="Static")
        y += w if d.get("advance", True) else 0.0


def _world(origin, yaw, local):
    """UE local (x out, y lateral, z) -> UE world given origin and yaw."""
    r = math.radians(yaw)
    lx, ly, lz = local
    return (origin[0] + lx * math.cos(r) - ly * math.sin(r), origin[1] + lx * math.sin(r) + ly * math.cos(r), origin[2] + lz)


def build_kitchen():
    # ---- west wall tall run (existing white cabinetry): front plane at x = TALL_D, facing east (yaw 0), from y=150 to 519
    # units along +Y: pantry 29" | appliance tower 30" | fridge 36" | freezer 36" | pantry 14"  = 145" = 12'-1"
    note("Tall run split", "pantry 29\" + appliance tower 30\" + fridge 36\" + freezer 36\" + pantry 14\"", "plan labels only; widths estimated to fill 12'-1 1/2\"")
    y = W_DOORZONE
    specs = [
        ("T01", inch(0, 29), [dict(name="DOOR_01_L", side="L")], 4),
        ("T02", inch(0, 30), [dict(name="FLAP_01", kind="flap", h=inch(0, 24), z0=TALL_H - inch(0, 24), angle=85, advance=False),
                              dict(name="DOOR_01_L", side="L", h=TALL_H - inch(0, 24) - g.TOE)], 2),
        ("A01", inch(0, 36), [dict(name="DOOR_01_R", side="R", angle=115)], 0),
        ("A02", inch(0, 36), [dict(name="DOOR_01_L", side="L", angle=115)], 0),
        ("T03", inch(0, 14), [dict(name="DOOR_01_R", side="R")], 4),
    ]
    for code, w, doors, shelves in specs:
        unit(f"SM_DEM_KIT_{code}", (TALL_D, y, 0), 0, w, TALL_H, TALL_D, doors, shelves=shelves, mat_front="FRONT", toe=True)
        y += w
    # soffit above the tall run to the dropped ceiling
    g.make_box("SM_DEM_KIT_TALL_SOFFIT", (0, W_DOORZONE, TALL_H), (TALL_D, ROOM_D, CEIL_KIT), "PAINT", col="Millwork", bevel_mm=1)
    # end panel at the south end of the run (2'-1" deep, D2.02-03 "existing kitchen 2'-1"")
    g.make_box("SM_DEM_KIT_TALL_ENDPANEL", (0, W_DOORZONE - 2, g.TOE), (TALL_D, W_DOORZONE, TALL_H), "FRONT", col="Millwork", bevel_mm=1)

    # ---- north wall: existing tall doors 3'-11 1/2" (x 63.5..183.5) facing south (yaw -90), then display 4'-2"
    unit("SM_DEM_KIT_T04", (TALL_D, ROOM_D - TALL_D, 0), -90, N_EXIST, TALL_H, TALL_D,
         [dict(name="DOOR_01_L", side="L", w=N_EXIST / 2), dict(name="DOOR_02_R", side="R", w=N_EXIST / 2)], shelves=4)
    # orientation: yaw -90 -> local +X = world -Y (faces south), local +Y = world +X, so the origin is the WEST end of the unit
    g.make_box("SM_DEM_KIT_T04_SOFFIT", (TALL_D, ROOM_D - TALL_D, TALL_H), (TALL_D + N_EXIST, ROOM_D, CEIL_KIT), "PAINT", col="Millwork", bevel_mm=1)

    # ---- display shelving (D2.02-03): 4'-2" wide: SS01 stone base cabinet 3'-0" high, stone back panel, bronze MT01 frame,
    #      GL02 bronze glass shelves 3 rows x 2 columns with LED L02 in each shelf, open back so stone shows; bulkhead above 7'-6"
    x0 = TALL_D + N_EXIST
    DISP_H = inch(7, 6)
    base_d = 45.0
    note("Display base depth", "45 cm (18\")", "D2.02-03 shows a shallow stone base; depth not dimensioned")
    unit("SM_DEM_KIT_S01", (x0, ROOM_D - base_d, 0), -90, N_DISP, ISL_H, base_d,
         [dict(name="DOOR_01_L", side="L", w=N_DISP / 2), dict(name="DOOR_02_R", side="R", w=N_DISP / 2)], shelves=1, mat_front="STONE", mat_carc="CARCASS", toe=True)
    g.make_box("SM_DEM_KIT_S01_TOP", (x0, ROOM_D - base_d - 2, ISL_H), (x0 + N_DISP, ROOM_D, ISL_H + 2), "STONE", col="Millwork", bevel_mm=2)
    g.make_box("SM_DEM_KIT_S01_BACKWALL", (x0, ROOM_D - 2, ISL_H + 2), (x0 + N_DISP, ROOM_D, DISP_H), "STONE", col="Millwork", bevel_mm=2)
    # bronze frame: 2 uprights + 2 mullions, shelves 3 rows; glass shelf 30 cm deep with LED under the front edge
    fr = []
    shelf_d = 30.0
    cols_x = [x0 + 6, x0 + N_DISP / 2, x0 + N_DISP - 6]
    for cx in cols_x:
        fr.append(((cx - 1.5, ROOM_D - 2 - shelf_d, ISL_H + 2), (cx + 1.5, ROOM_D - 2, DISP_H), "METAL_BRONZE"))
    fr.append(((x0 + 4.5, ROOM_D - 2 - shelf_d, DISP_H - 3), (x0 + N_DISP - 4.5, ROOM_D - 2, DISP_H), "METAL_BRONZE"))
    g.make_parts("SM_DEM_KIT_S01_FRAME", fr, col="Millwork", bevel_mm=0.5)
    rows_z = [ISL_H + 2 + 36 * i + 30 for i in range(3)]
    for r, z in enumerate(rows_z):
        for c in range(2):
            xa, xb = cols_x[c] + 1.5, cols_x[c + 1] - 1.5
            g.make_parts(f"SM_DEM_KIT_S01_SHELF_{r+1:02d}{c+1:02d}", [
                ((xa, ROOM_D - 2 - shelf_d, z), (xb, ROOM_D - 2, z + 1.0), "GLASS_BRONZE"),
                ((xa, ROOM_D - 2 - shelf_d, z - 2.5), (xb, ROOM_D - 2 - shelf_d + 2.5, z), "METAL_BRONZE"),  # front profile housing the LED
            ], col="Millwork", bevel_mm=0.5)
            g.make_box(f"SM_DEM_KIT_S01_LED_{r+1:02d}{c+1:02d}", (xa + 1, ROOM_D - 2 - shelf_d + 0.6, z - 2.2), (xb - 1, ROOM_D - 2 - shelf_d + 1.6, z - 0.3), "LED", col="Millwork", bevel_mm=0)
            PLACEMENT[f"SM_DEM_KIT_S01_LED_{r+1:02d}{c+1:02d}"] = dict(mechanism="Light", group="LightGroup.Kitchen.Cabinet", lumens=120, kelvin=3000, type="Rect")
            LIGHTS.append(dict(name=f"L02_DISPLAY_{r+1}{c+1}", type="Rect", group="LightGroup.Kitchen.Cabinet", lumens=120, kelvin=3000,
                               pos=((xa + xb) / 2, ROOM_D - 2 - shelf_d + 1.0, z - 1.5), dir=(0.3, 0.3, -1), size=(xb - xa - 2, 1.0)))
    g.make_box("SM_DEM_KIT_S01_BULKHEAD", (x0, ROOM_D - base_d, DISP_H), (x0 + N_DISP + N_END, ROOM_D, CEIL_KIT), "PAINT", col="Millwork", bevel_mm=1)
    g.make_box("SM_DEM_KIT_N_FILLER", (x0 + N_DISP, ROOM_D - 2, 0), (x0 + N_DISP + N_END, ROOM_D, DISP_H), "PAINT", col="Millwork", bevel_mm=1)

    # ---- island: stone clad all sides (SS01), working side faces west (yaw 180) towards the tall run
    X0, Y0 = ISL_X0, ISL_Y0
    X1i, Y1i = X0 + ISL_W, Y0 + ISL_L
    seat = 30.0
    body_w = ISL_W - seat          # cabinet depth 77 incl. 2 cm cladding
    # stone shell: east waterfall panel (seating side), two ends, top
    g.make_parts("SM_DEM_KIT_I01_STONE", [
        ((X0, Y0, 0), (X1i, Y1i, ISL_H), "STONE") if False else ((X0 + body_w - 2, Y0, 0), (X0 + body_w, Y1i, ISL_H - 2), "STONE"),  # back (east) of cabinets
        ((X0, Y0, 0), (X1i - seat, Y0 + 2, ISL_H - 2), "STONE"),                   # south end
        ((X0, Y1i - 2, 0), (X1i - seat, Y1i, ISL_H - 2), "STONE"),                 # north end
        # top 2 cm incl. 30 cm seating overhang east, with the undermount sink cutout (B02: y Y0+60..Y0+150, x X0+8..X0+53)
        ((X0 - 2, Y0 - 2, ISL_H - 2), (X1i + 2, Y0 + 75, ISL_H), "STONE"),
        ((X0 - 2, Y0 + 135, ISL_H - 2), (X1i + 2, Y1i + 2, ISL_H), "STONE"),
        ((X0 - 2, Y0 + 75, ISL_H - 2), (X0 + 8, Y0 + 135, ISL_H), "STONE"),
        ((X0 + 53, Y0 + 75, ISL_H - 2), (X1i + 2, Y0 + 135, ISL_H), "STONE"),
        ((X1i - seat - 2, Y0, 0), (X1i - seat, Y1i, ISL_H - 2), "STONE"),          # overhang support panel
    ], col="Millwork", bevel_mm=2)
    PLACEMENT["SM_DEM_KIT_I01_STONE"] = dict(mechanism="Static", configurable="Configurable.Countertop.Kitchen", slot="STONE")
    # working side units from south to north, fronts at x = X0 facing west (yaw 180): local +Y = world -Y, so origin at the NORTH end of each unit
    cab_d = body_w - 2.0
    runs = [("B01", 60, "dishwasher"), ("B02", 90, "sink"), ("B03", 60, "drawers"), ("B04", 60, "drawers"), ("B05", ISL_L - 270, "doors")]
    y = Y0
    for code, w, kind in runs:
        origin = (X0, y + w, 0)
        if kind == "dishwasher":
            # integrated panel dishwasher: carcass + drop-down door (pivot bottom edge) + 2 racks (drawers) inside
            unit(f"SM_DEM_KIT_{code}", origin, 180, w, ISL_H - 2, cab_d, [dict(name="FLAP_01", kind="flap_down", h=ISL_H - 2 - g.TOE, angle=88)], shelves=0)
            for r, (z0, h) in enumerate([(g.TOE + 4, 34), (g.TOE + 42, 32)]):
                nm = f"SM_DEM_KIT_{code}_TRAY_{r+1:02d}"
                bd = cab_d - 6
                g.make_parts(nm, g.drawer_parts(w - 6, h, bd, body_h=h - 2, mat="STAINLESS", mat_box="STAINLESS"), pivot=_world(origin, 180, (-bd - 2, w / 2, z0)), yaw=180, col="Millwork", bevel_mm=0.5)
                PLACEMENT[nm] = dict(mechanism="Drawer", travel=bd - 4, requires=[f"SM_DEM_KIT_{code}_FLAP_01"])
        elif kind == "sink":
            unit(f"SM_DEM_KIT_{code}", origin, 180, w, ISL_H - 2, cab_d, [dict(name="DOOR_01_L", side="L", w=w / 2), dict(name="DOOR_02_R", side="R", w=w / 2)], shelves=0)
            # undermount sink + faucet PF01 (Gessi, aged bronze)
            g.make_parts("SM_DEM_KIT_SINK", [((X0 + 8, y + 15, ISL_H - 22), (X0 + 8 + 45, y + w - 15, ISL_H - 2.5), "STAINLESS")], col="Furniture", bevel_mm=1)
            g.make_cylinder("SM_DEM_KIT_FAUCET", 1.6, 30, "METAL_BRONZE", (X0 + 60, y + w / 2, ISL_H), col="Furniture")
        elif kind == "drawers":
            unit(f"SM_DEM_KIT_{code}", origin, 180, w, ISL_H - 2, cab_d, [
                dict(name="DRW_01", kind="drawer", h=18, z0=ISL_H - 2 - 18, advance=False),
                dict(name="DRW_02", kind="drawer", h=30, z0=ISL_H - 2 - 48, advance=False),
                dict(name="DRW_03", kind="drawer", h=ISL_H - 2 - 48 - g.TOE, z0=g.TOE)], shelves=0)
        else:
            unit(f"SM_DEM_KIT_{code}", origin, 180, w, ISL_H - 2, cab_d, [dict(name="DOOR_01_L", side="L", w=w / 2), dict(name="DOOR_02_R", side="R", w=w / 2)], shelves=1)
        y += w
    # cooktop over B03+B04 (induction, black glass)
    g.make_box("SM_DEM_KIT_COOKTOP", (X0 + 8, Y0 + 150 + 5, ISL_H), (X0 + 8 + 52, Y0 + 270 - 5, ISL_H + 0.6), "APPLIANCE", col="Furniture", bevel_mm=0.5)
    note("Island layout (working side, S->N)", "dishwasher 24\" | sink base 36\" | drawers 24\" | drawers 24\" | doors 34\"; cooktop over the drawer stacks", "D2.01 shows sink and cooktop on the island; cabinet split estimated")

    # ---- downlights L01 (existing square pot lights): kitchen 6, grid
    for i, x in enumerate([90, 230]):
        for j, yy in enumerate([200, 330, 460]):
            LIGHTS.append(dict(name=f"L01_KIT_{i+1}{j+1}", type="Spot", group="LightGroup.Kitchen.Ambient", lumens=700, kelvin=3000, ies="IES_Downlight_36deg",
                               pos=(x, yy, CEIL_KIT - 1), dir=(0, 0, -1)))
            g.make_box(f"SM_DEM_KIT_LIGHT_L01_{i+1}{j+1}", (x - 5, yy - 5, CEIL_KIT - 0.5), (x + 5, yy + 5, CEIL_KIT), "PAINT", col="Furniture", bevel_mm=0.5)
    # undercabinet LED on the display shelves (done), cove LED under the kitchen soffit edge facing dining
    LIGHTS.append(dict(name="L02_KIT_SOFFIT_EDGE", type="Rect", group="LightGroup.Kitchen.Cove", lumens=1500, kelvin=3000,
                       pos=(KIT_N_W - 3, ROOM_D / 2, CEIL_KIT + 6), dir=(1, 0, 0.2), size=(ROOM_D - 60, 1.0)))
    g.make_box("SM_DEM_KIT_LED_SOFFIT_EDGE", (KIT_N_W - 2, 30, CEIL_KIT + 4), (KIT_N_W - 1, ROOM_D - 30, CEIL_KIT + 5), "LED", col="Millwork", bevel_mm=0)
    PLACEMENT["SM_DEM_KIT_LED_SOFFIT_EDGE"] = dict(mechanism="Light", group="LightGroup.Kitchen.Cove", lumens=1500, kelvin=3000, type="Rect")


# ======================================================================= dining
def build_dining():
    col = "Millwork"
    # ---- south feature wall (D2.02-01): M01 millwork + GL01 bronze mirror panels in 3 rows, SS02 stone band on top, 3/4" reveals
    # D2.02-01 is drawn looking SOUTH, so the viewer's left is EAST: return | feature | opening | cabinets runs east -> west
    RET_X0 = ROOM_W - S_A                 # 602..691 return panel
    x0 = RET_X0 - S_FEAT                  # 286..602 feature wall (sits on the W.I.C. north wall, D1.00)
    OPEN_X0 = x0 - S_OPEN                 # 176..286 opening to the hall
    row_h = inch(2, 2.25)          # 66.7
    reveal = 1.9
    plinth = inch(1, 0)            # 30
    band_top = 232.0
    # panel widths per row from the elevation (left to right), W = wood M01, M = mirror GL01
    rows = [
        [("W", inch(3, 1.75)), ("M", inch(1, 0)), ("W", inch(1, 8)), ("M", inch(1, 0)), ("W", inch(3, 6)), ("M", inch(0, 9.25)), ("W", None)],
        [("M", inch(1, 0)), ("W", inch(2, 1)), ("M", inch(1, 0)), ("W", inch(3, 6)), ("M", inch(0, 9.25)), ("W", inch(2, 5)), ("M", None)],
        [("W", inch(2, 6.5)), ("M", inch(0, 9.25)), ("W", inch(1, 8)), ("M", inch(1, 0)), ("W", inch(3, 6)), ("M", inch(0, 9.25)), ("W", None)],
    ]
    note("Feature wall panel rhythm", "widths read off D2.02-01 per row, scaled ~0.93 to fit 10'-4 1/4\"; last panel 12\"", "listed widths over-sum the wall by 2-7%; reveal 3/4\"")
    g.make_box("SM_DEM_DIN_P01_PLINTH", (x0, 0, 0), (x0 + S_FEAT, 6, plinth), "WOOD", col=col, bevel_mm=1)
    g.make_box("SM_DEM_DIN_P01_BACKER", (x0, 0, plinth), (x0 + S_FEAT, 2.0, band_top), "METAL_BLACK", col=col, bevel_mm=0)
    parts_w, parts_m = [], []
    for r, row in enumerate(rows):
        z0 = plinth + r * (row_h + reveal)
        z1 = z0 + row_h
        x = x0 + S_FEAT                   # start at the east end (viewer's left) and go west
        listed = sum(w for _, w in row if w is not None)
        scale = (S_FEAT - inch(1, 0)) / listed          # last panel = 12", the rest scaled to fit the 10'-4 1/4" wall
        for k, (kind, w) in enumerate(row):
            if w is None:
                w = x - x0
            else:
                w *= scale
            if kind == "W":
                parts_w.append(((x - w + reveal, 2.0, z0), (x, 2.0 + 2.0, z1), "PANEL"))
            else:
                parts_m.append(((x - w + reveal, 2.0, z0), (x, 2.0 + 0.6, z1), "MIRROR_BRONZE"))
            x -= w
    g.make_parts("SM_DEM_DIN_P01_PANELS_WOOD", parts_w, col=col, bevel_mm=1)
    PLACEMENT["SM_DEM_DIN_P01_PANELS_WOOD"] = dict(mechanism="Static", configurable="Configurable.WallPanel.Dining", slot="PANEL")
    g.make_parts("SM_DEM_DIN_P01_PANELS_MIRROR", parts_m, col=col, bevel_mm=0.5)
    g.make_box("SM_DEM_DIN_P01_STONE_BAND", (x0, 0, band_top + 1), (x0 + S_FEAT, 2.5, CEIL), "STONE_BLUE", col=col, bevel_mm=2)
    PLACEMENT["SM_DEM_DIN_P01_STONE_BAND"] = dict(mechanism="Static", configurable="Configurable.FeatureStone.Dining", slot="STONE_BLUE")
    # left return 2'-11 1/4": wood panel
    g.make_box("SM_DEM_DIN_P01_RETURN", (RET_X0, 0, 0), (ROOM_W, 2.0, CEIL), "PANEL", col=col, bevel_mm=1)
    # opening trim (M01 casing 2" x 3/4")
    ox = OPEN_X0
    g.make_parts("SM_DEM_DIN_OPENING_CASING", [((ox - 5, 0, 0), (ox, 2.0, 232), "WOOD"), ((ox + S_OPEN, 0, 0), (ox + S_OPEN + 5, 2.0, 232), "WOOD"),
                                                ((ox - 5, 0, 230), (ox + S_OPEN + 5, 2.0, 235), "WOOD")], col=col, bevel_mm=1)
    # ---- tall M01 cabinets right of the opening (5'-9" = 2 doors), depth 45, height to the stone band, top band continues
    cx0 = 0.0                             # 0..176: cabinets in the kitchen's SE corner, next to entry door D.01
    cab_d = 30.0
    # yaw 90: local +Y = world -X -> origin at the east end of the unit
    note("South tall cabinet depth", "30 cm (12\")", "D2.02-01 gives width 5'-9\" only; shallow so the entry door D.01 beside it stays usable")
    unit("SM_DEM_DIN_T01", (cx0 + S_CAB, cab_d, 0), 90, S_CAB, band_top, cab_d,
         [dict(name="DOOR_01_L", side="L", w=S_CAB / 2, z0=0, h=band_top), dict(name="DOOR_02_R", side="R", w=S_CAB / 2, z0=0, h=band_top)], shelves=4, mat_front="PANEL", mat_carc="CARCASS", toe=False)
    g.make_box("SM_DEM_DIN_T01_BAND", (cx0, 0, band_top + 1), (cx0 + S_CAB, cab_d, CEIL), "STONE_BLUE", col=col, bevel_mm=2)

    # ---- table, chairs, chandelier
    fc = "Furniture"
    cx, cy = TBL_CX, TBL_CY
    g.make_box("SM_DEM_DIN_TABLE_TOP", (cx - TBL_L / 2, cy - TBL_W / 2, TBL_H - 3), (cx + TBL_L / 2, cy + TBL_W / 2, TBL_H), "STONE", col=fc, bevel_mm=3)
    for i, dx in enumerate([-TBL_L * 0.3, TBL_L * 0.3]):
        g.make_cylinder(f"SM_DEM_DIN_TABLE_LEG_{i+1:02d}", 28, TBL_H - 3, "WOOD", (cx + dx, cy, 0), col=fc)
    k = 0
    for side, yy in [(-1, cy - TBL_W / 2 - 30), (1, cy + TBL_W / 2 + 30)]:
        for dx in [-TBL_L * 0.36, -TBL_L * 0.12, TBL_L * 0.12, TBL_L * 0.36]:
            k += 1
            x = cx + dx
            g.make_parts(f"SM_DEM_DIN_CHAIR_{k:02d}", [
                ((x - 26, yy - 24, 42), (x + 26, yy + 24, 47), "FABRIC"),
                ((x - 26, yy - 24 + (0 if side < 0 else 44), 47), (x + 26, yy - 20 + (0 if side < 0 else 44), 86), "FABRIC"),
                ((x - 22, yy - 20, 0), (x - 19, yy - 17, 42), "METAL_BRONZE"), ((x + 19, yy - 20, 0), (x + 22, yy - 17, 42), "METAL_BRONZE"),
                ((x - 22, yy + 17, 0), (x - 19, yy + 20, 42), "METAL_BRONZE"), ((x + 19, yy + 17, 0), (x + 22, yy + 20, 42), "METAL_BRONZE"),
            ], col=fc, bevel_mm=3)
    # chandelier L07: ring 120 cm dia at 200 cm, bronze, with inner LED ring
    g.make_torus("SM_DEM_DIN_L07_RING", 60, 2.2, "METAL_BRONZE", (cx, cy, 200), col=fc)
    g.make_cylinder("SM_DEM_DIN_L07_STEM", 0.6, CEIL_DIN - 200, "METAL_BRONZE", (cx, cy, 200), col=fc)
    LIGHTS.append(dict(name="L07_CHANDELIER", type="Point", group="LightGroup.Dining.Pendant", lumens=2000, kelvin=2700, pos=(cx, cy, 196), radius=15))
    PLACEMENT["SM_DEM_DIN_L07_RING"] = dict(mechanism="Light", group="LightGroup.Dining.Pendant", lumens=2000, kelvin=2700, type="Point")
    # dining downlights (new L03 to match existing) 4 + accent on the feature wall 3
    for i, x in enumerate([400, 560]):
        for j, yy in enumerate([130, 390]):
            LIGHTS.append(dict(name=f"L03_DIN_{i+1}{j+1}", type="Spot", group="LightGroup.Dining.Ambient", lumens=700, kelvin=3000, ies="IES_Downlight_36deg", pos=(x, yy, CEIL_DIN - 1), dir=(0, 0, -1)))
            g.make_box(f"SM_DEM_DIN_LIGHT_L03_{i+1}{j+1}", (x - 5, yy - 5, CEIL_DIN - 0.5), (x + 5, yy + 5, CEIL_DIN), "PAINT", col=fc, bevel_mm=0.5)
    for i, x in enumerate([360, 445, 530]):
        LIGHTS.append(dict(name=f"L06_DIN_WASH_{i+1}", type="Spot", group="LightGroup.Dining.Accent", lumens=400, kelvin=2700, ies="IES_WallWasher", pos=(x, 45, CEIL_DIN - 1), dir=(0, -0.45, -1)))
    # cove LED around the dining soffit
    for nm, a, b, d in [("S", (KIT_N_W + 32, 31, CEIL - 6), (ROOM_W - 32, 32, CEIL - 5), (0, 1, 0.3)), ("N", (KIT_N_W + 32, ROOM_D - 32, CEIL - 6), (ROOM_W - 32, ROOM_D - 31, CEIL - 5), (0, -1, 0.3)),
                        ("W", (KIT_N_W + 31, 32, CEIL - 6), (KIT_N_W + 32, ROOM_D - 32, CEIL - 5), (1, 0, 0.3)), ("E", (ROOM_W - 32, 32, CEIL - 6), (ROOM_W - 31, ROOM_D - 32, CEIL - 5), (-1, 0, 0.3))]:
        g.make_box(f"SM_DEM_DIN_LED_COVE_{nm}", a, b, "LED", col=col, bevel_mm=0)
        PLACEMENT[f"SM_DEM_DIN_LED_COVE_{nm}"] = dict(mechanism="Light", group="LightGroup.Dining.Cove", lumens=1200, kelvin=3000, type="Rect")
        LIGHTS.append(dict(name=f"L02_DIN_COVE_{nm}", type="Rect", group="LightGroup.Dining.Cove", lumens=1200, kelvin=3000,
                           pos=((a[0] + b[0]) / 2, (a[1] + b[1]) / 2, CEIL - 5.5), dir=d, size=(max(b[0] - a[0], b[1] - a[1]) - 4, 1.0)))

    # ---- light switches: by the hall opening (dining) and by the hall door (kitchen)
    for nm, pos, yaw, groups in [("SM_DEM_DIN_SW01", (ROOM_W - S_A - S_FEAT + 10, 4.5, 110), 90, ["LightGroup.Dining"]),
                                 ("SM_DEM_KIT_SW01", (2.5, W_DOORZONE - 20, 110), 0, ["LightGroup.Kitchen"])]:
        g.make_parts(nm + "_PLATE", [((0, -6, -6), (0.6, 6, 6), "PAINT")], pivot=pos, yaw=yaw, col=col, bevel_mm=0.5)
        g.make_parts(nm + "_ROCKER", [((0.6, -1.6, -3.2), (1.2, 1.6, 3.2), "PAINT")], pivot=pos, yaw=yaw, col=col, bevel_mm=0.3)
        PLACEMENT[nm + "_ROCKER"] = dict(mechanism="Switch", groups=groups, plate=nm + "_PLATE")


# ======================================================================= rest of the apartment (shell only for now)
ROOMS_REST = [  # name, centre (UE cm, from the D1.00 room labels), light group, downlight offsets
    ("LIV", (767, 700), "LightGroup.Living.Ambient", [(-130, -110), (130, -110), (-130, 110), (130, 110)]),
    ("FOY", (373, 650), "LightGroup.Foyer.Accent", [(0, 40), (0, -60)]),
    ("HAL", (243, -235), "LightGroup.Foyer.Accent", [(0, 60), (0, -60)]),
    ("PBR", (661, -850), "LightGroup.Primary.Ambient", [(-90, -70), (90, -70), (-90, 70), (90, 70)]),
    ("PEN", (632, -480), "LightGroup.Ensuite.Ambient", [(-60, 0), (60, 0)]),
    ("WIC", (514, -130), "LightGroup.WIC.Cabinet", [(-60, 0), (60, 0)]),
    ("GBR", (-39, -760), "LightGroup.Guest.Ambient", [(-60, -50), (60, -50), (0, 70)]),
    ("GEN", (-95, -400), "LightGroup.Guest.Ambient", [(0, 0)]),
    ("DEN", (270, -760), "LightGroup.Den.Ambient", [(-60, -50), (60, -50), (0, 70)]),
    ("PWD", (127, -250), "LightGroup.Powder.Ambient", [(0, 0)]),
    ("LAU", (344, -342), "LightGroup.Foyer.Accent", [(0, 0)]),
    ("WCL", (590, -338), "LightGroup.Ensuite.Ambient", [(0, 0)]),
]


def build_rooms():
    if not os.path.exists(PLAN):
        return
    for code, (cx, cy), group, offs in ROOMS_REST:
        for k, (dx, dy) in enumerate(offs):
            x, y = cx + dx, cy + dy
            LIGHTS.append(dict(name=f"L03_{code}_{k+1:02d}", type="Spot", group=group, lumens=650, kelvin=3000, ies="IES_Downlight_36deg",
                               pos=(x, y, CEIL - 1), dir=(0, 0, -1)))
            g.make_box(f"SM_DEM_{code}_LIGHT_L03_{k+1:02d}", (x - 5, y - 5, CEIL - 0.5), (x + 5, y + 5, CEIL), "PAINT", col="Furniture", bevel_mm=0.5)
    note("Other rooms", "shell + downlights only (doors open, no millwork yet)", "D2.03-D2.13 millwork is the next build step")


# ======================================================================= export
def export(col_names, path):
    bpy.ops.object.select_all(action="DESELECT")
    n = 0
    for cn in col_names:
        c = bpy.data.collections.get(cn)
        if not c:
            continue
        for ob in c.objects:
            ob.select_set(True)
            n += 1
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True, apply_scale_options="FBX_SCALE_ALL",  # unit scale goes to the FBX header, nodes stay at scale 1 (UE drops node transforms)
                            
                             bake_space_transform=False, object_types={"MESH"}, use_mesh_modifiers=True, mesh_smooth_type="FACE",
                             use_tspace=True, add_leaf_bones=False, path_mode="AUTO", axis_forward="-Z", axis_up="Y")
    print(f"[NOAR] exported {n} objects -> {path}")


def write_sidecars():
    # placement: UE world transform of every object (pivot + yaw) + mechanism metadata
    for ob in bpy.data.objects:
        if ob.type != "MESH":
            continue
        p = ob.get("noar_pivot_ue", [0, 0, 0])
        yaw = ob.get("noar_yaw_ue", 0.0)
        PLACEMENT.setdefault(ob.name, dict(mechanism="Static"))
        PLACEMENT[ob.name].update(location=[round(float(v), 2) for v in p], yaw=float(yaw), collection=ob.users_collection[0].name,
                                  materials=[m.name for m in ob.data.materials])
    with open(os.path.join(OUT, "placement.json"), "w", encoding="utf-8") as f:
        json.dump(dict(frame="UE: X east, Y north, Z up, cm; origin SW interior corner kitchen/dining", objects=PLACEMENT), f, indent=1, ensure_ascii=False)
    with open(os.path.join(OUT, "lights.json"), "w", encoding="utf-8") as f:
        json.dump(dict(frame="UE cm", lights=LIGHTS), f, indent=1, ensure_ascii=False)
    with open(os.path.join(OUT, "DIMENSIONS_ASSUMED.md"), "w", encoding="utf-8") as f:
        f.write("# Dempster prototype room — dimensions NOT on the drawings (assumed, for review)\n\n")
        f.write("Everything else is taken from D1.00/D1.01/D2.01/D2.02 (ID drawings 09.30.2026).\n\n| What | Assumed value | Why / source |\n| --- | --- | --- |\n")
        for w, v, y in ASSUMED:
            f.write(f"| {w} | {v.replace(' | ', ' / ')} | {y} |\n")
        f.write(f"\nObjects: {len(PLACEMENT)}; lights: {len(LIGHTS)}.\n")


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system = "METRIC"
    bpy.context.scene.unit_settings.scale_length = 0.01   # 1 BU = 1 cm (see noar_geo.CM)
    build_shell()
    build_kitchen()
    build_dining()
    build_rooms()
    write_sidecars()
    export(["Architecture"], os.path.join(OUT, "SM_DEM_PROTO_Architecture.fbx"))
    export(["Millwork"], os.path.join(OUT, "SM_DEM_PROTO_Millwork.fbx"))
    export(["Furniture"], os.path.join(OUT, "SM_DEM_PROTO_Furniture.fbx"))
    import time
    with open(os.path.join(OUT, "export_version.txt"), "w") as f:
        f.write(time.strftime("%Y%m%d-%H%M%S"))
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "DEM_Prototype.blend"))
    print(f"[NOAR] {len(bpy.data.objects)} objects, {len(ASSUMED)} assumptions -> {OUT}")


if __name__ == "__main__":
    main()
