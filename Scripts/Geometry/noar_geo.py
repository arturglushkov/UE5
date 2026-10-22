"""
noar_geo.py - geometry helpers for building NOAR millwork in Blender (bpy) in UE conventions.

World frame used by every builder: UE left-handed, cm, X east, Y north, Z up, origin at the SW interior
corner of the kitchen/dining floor. Blender is right-handed, so we mirror Y when writing vertices and
negate yaw on objects; the UE FBX importer mirrors Y back, which gives exactly the UE frame we designed in.

Pivot conventions (Standard v1.1 section 4):
  DOOR   pivot on hinge edge at the front plane, local +X out of the cabinet, +Z up; L hinge = door extends +Y
  DRAWER pivot back-bottom-centre, travel +X
  FLAP   pivot on top edge (lift-up) or bottom edge (drop-down), local +X out, rotation about Y
  static pivot anywhere (we use world origin for shell, unit front-centre for carcasses)
"""
import bpy
import bmesh
import math
from mathutils import Vector

# 1 Blender unit = 1 cm (scene scale_length = 0.01) so the FBX carries centimetres with UnitScaleFactor 1:
# UE imports it 1:1 whatever importer / unit conversion is active.
CM = 1.0
UV_SCALE = 1.0 / (100.0 * CM)   # UV: 1 unit = 1 m

# ------------------------------------------------------------------ collections / materials

def collection(name):
    col = bpy.data.collections.get(name)
    if not col:
        col = bpy.data.collections.new(name)
        bpy.context.scene.collection.children.link(col)
    return col


_MAT_COLORS = {
    "FRONT": (0.86, 0.86, 0.85), "CARCASS": (0.78, 0.78, 0.76), "INTERIOR": (0.82, 0.80, 0.76),
    "TOP": (0.92, 0.91, 0.88), "STONE": (0.90, 0.89, 0.86), "STONE_BLUE": (0.55, 0.62, 0.70),
    "PANEL": (0.62, 0.48, 0.32), "WOOD": (0.60, 0.46, 0.30), "MIRROR_BRONZE": (0.45, 0.33, 0.22),
    "GLASS_BRONZE": (0.45, 0.33, 0.22), "GLASS": (0.9, 0.95, 1.0), "METAL_BRONZE": (0.55, 0.40, 0.25),
    "METAL_BLACK": (0.05, 0.05, 0.05), "STAINLESS": (0.7, 0.7, 0.72), "PAINT": (0.95, 0.94, 0.90),
    "CEILING": (0.97, 0.97, 0.96), "FLOOR": (0.80, 0.78, 0.74), "LED": (1.0, 0.95, 0.85),
    "FABRIC": (0.85, 0.82, 0.76), "APPLIANCE": (0.15, 0.15, 0.15), "CONCRETE": (0.6, 0.6, 0.6),
}


def material(name):
    """Material slot names are what UE sees. Configurable roles use bare role names (FRONT, TOP, PANEL...)."""
    m = bpy.data.materials.get(name)
    if not m:
        m = bpy.data.materials.new(name)
        base = name.split("__")[0]
        c = _MAT_COLORS.get(base, (0.7, 0.7, 0.7))
        m.diffuse_color = (*c, 1.0)
        m.use_nodes = True
        bsdf = m.node_tree.nodes.get("Principled BSDF")
        if bsdf:
            bsdf.inputs["Base Color"].default_value = (*c, 1.0)
    return m

# ------------------------------------------------------------------ mesh building

def _box_verts(mn, mx):
    x0, y0, z0 = mn
    x1, y1, z1 = mx
    return [(x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0), (x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)]


_BOX_FACES = [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]


def _ue_local_to_blender(v):
    """UE local cm (x, y, z) -> Blender local m (x, -y, z)."""
    return Vector((v[0] * CM, -v[1] * CM, v[2] * CM))


def box_uv(bm, uv_layer, scale=None):
    scale = UV_SCALE if scale is None else scale
    """World-scale box projection: 1 UV unit = 1 m (Standard: floor/materials tile in metres)."""
    for f in bm.faces:
        n = f.normal
        ax, ay, az = abs(n.x), abs(n.y), abs(n.z)
        for loop in f.loops:
            p = loop.vert.co
            if az >= ax and az >= ay:
                uv = (p.x, p.y)
            elif ax >= ay:
                uv = (p.y, p.z)
            else:
                uv = (p.x, p.z)
            loop[uv_layer].uv = (uv[0] * scale, uv[1] * scale)


def make_parts(name, parts, pivot=(0, 0, 0), yaw=0.0, col="Millwork", bevel_mm=1.0, smooth=False):
    """
    parts: list of (min_local_ue, max_local_ue, material_name) boxes in UE local cm relative to the pivot.
    pivot: UE world cm; yaw: UE degrees (local +X direction = world yaw).
    Returns the Blender object. Bevel modifier (real bevel, applied on export) on every edge.
    """
    bm = bmesh.new()
    mat_names = []
    for mn, mx, mat in parts:
        if mat not in mat_names:
            mat_names.append(mat)
        mi = mat_names.index(mat)
        vs = [bm.verts.new(_ue_local_to_blender(v)) for v in _box_verts(mn, mx)]
        bm.verts.ensure_lookup_table()
        for f in _BOX_FACES:
            face = bm.faces.new([vs[i] for i in f])
            face.material_index = mi
    bm.normal_update()
    # Blender's mirrored Y flips winding; recalc normals outward
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    uv_layer = bm.loops.layers.uv.new("UVMap")
    box_uv(bm, uv_layer)

    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    for mn in mat_names:
        me.materials.append(material(mn))

    ob = bpy.data.objects.new(name, me)
    ob.location = Vector((pivot[0] * CM, -pivot[1] * CM, pivot[2] * CM))
    ob.rotation_euler = (0.0, 0.0, math.radians(-yaw))
    collection(col).objects.link(ob)

    if bevel_mm > 0:
        mod = ob.modifiers.new("Bevel", "BEVEL")
        mod.width = bevel_mm * 0.1 * CM
        mod.segments = 2 if bevel_mm >= 1.0 else 1
        mod.limit_method = "ANGLE"
        mod.angle_limit = math.radians(60)
        mod.harden_normals = True
    if smooth:
        for p in me.polygons:
            p.use_smooth = True
    ob["noar_pivot_ue"] = list(pivot)
    ob["noar_yaw_ue"] = yaw
    return ob


def make_box(name, mn, mx, mat, pivot=(0, 0, 0), yaw=0.0, col="Millwork", bevel_mm=1.0):
    return make_parts(name, [(mn, mx, mat)], pivot, yaw, col, bevel_mm)


def make_cylinder(name, radius, height, mat, pivot, yaw=0.0, col="Furniture", segments=48, axis="Z"):
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=segments,
                          radius1=radius * CM, radius2=radius * CM, depth=height * CM)
    if axis == "Z":
        bmesh.ops.translate(bm, verts=bm.verts, vec=(0, 0, height * CM / 2))
    uv_layer = bm.loops.layers.uv.new("UVMap")
    box_uv(bm, uv_layer)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    me.materials.append(material(mat))
    for p in me.polygons:
        p.use_smooth = True
    ob = bpy.data.objects.new(name, me)
    ob.location = Vector((pivot[0] * CM, -pivot[1] * CM, pivot[2] * CM))
    ob.rotation_euler = (0.0, 0.0, math.radians(-yaw))
    collection(col).objects.link(ob)
    ob["noar_pivot_ue"] = list(pivot)
    ob["noar_yaw_ue"] = yaw
    return ob


def make_torus(name, major_r, minor_r, mat, pivot, col="Furniture"):
    bm = bmesh.new()
    bmesh.ops.create_circle(bm, cap_ends=False, segments=64, radius=major_r * CM)
    # extrude ring into a tube: simpler to build torus via bpy primitive
    bm.free()
    bpy.ops.mesh.primitive_torus_add(major_radius=major_r * CM, minor_radius=minor_r * CM, major_segments=64, minor_segments=12,
                                     location=(pivot[0] * CM, -pivot[1] * CM, pivot[2] * CM))
    ob = bpy.context.active_object
    ob.name = name
    ob.data.name = name
    for c in list(ob.users_collection):
        c.objects.unlink(ob)
    collection(col).objects.link(ob)
    ob.data.materials.append(material(mat))
    for p in ob.data.polygons:
        p.use_smooth = True
    ob["noar_pivot_ue"] = list(pivot)
    ob["noar_yaw_ue"] = 0.0
    return ob

# ------------------------------------------------------------------ cabinet builders (local frame: +X out, +Y lateral, +Z up)

T_FRONT = 1.9        # 3/4" front
T_CARC = 1.9         # 3/4" carcass panel
GAP = 0.3            # 3 mm reveal between fronts
TOE = 10.0           # toe kick height
TOE_SET = 5.0        # toe kick setback


def carcass_parts(width, height, depth, mat="CARCASS", mat_int="INTERIOR", shelves=0, toe=True, back=True):
    """5-sided box behind the front plane (x from -depth to 0), y from 0..width, z 0..height. Parts list."""
    z0 = TOE if toe else 0.0
    parts = []
    parts.append(((-depth, 0, z0), (-depth + T_CARC, width, height), mat))                 # back
    parts.append(((-depth, 0, z0), (0, T_CARC, height), mat))                               # left side
    parts.append(((-depth, width - T_CARC, z0), (0, width, height), mat))                   # right side
    parts.append(((-depth, 0, z0), (0, width, z0 + T_CARC), mat_int))                       # bottom
    parts.append(((-depth, 0, height - T_CARC), (0, width, height), mat_int))               # top
    if toe:
        parts.append(((-depth, 0, 0), (-TOE_SET, width, TOE), "METAL_BLACK"))               # toe kick
    for i in range(shelves):
        z = z0 + (height - z0) * (i + 1) / (shelves + 1)
        parts.append(((-depth + T_CARC, T_CARC, z), (-2.0, width - T_CARC, z + T_CARC), mat_int))
    return parts


def door_parts(width, height, side="L", mat="FRONT", handle=True):
    """Door box in hinge-local frame: x 0..T_FRONT (out), y 0..w (L) or -w..0 (R), z 0..h. Handle = finger pull groove placeholder."""
    w = width - GAP
    h = height - GAP
    if side == "L":
        parts = [((0, 0, 0), (T_FRONT, w, h), mat)]
        hx = (T_FRONT, w - 6.0, h / 2 - 8)
    else:
        parts = [((0, -w, 0), (T_FRONT, 0, h), mat)]
        hx = (T_FRONT, -w + 3.0, h / 2 - 8)
    if handle:
        parts.append(((hx[0], hx[1], hx[2]), (hx[0] + 1.0, hx[1] + 3.0, hx[2] + 16.0), "METAL_BRONZE"))
    return parts


def drawer_parts(width, front_h, body_depth, body_h=None, mat="FRONT", mat_box="INTERIOR"):
    """Drawer in back-bottom-centre-local frame: box x -0..body_depth? No: pivot at BACK bottom centre; body extends +X to the front.
    x: 0 (back) .. body_depth (front plane); front panel from body_depth to body_depth+T_FRONT; y centred; z 0..front_h."""
    w = width - GAP
    bh = body_h or max(8.0, front_h - 6.0)
    y0, y1 = -w / 2, w / 2
    d = body_depth
    parts = [((d, y0, 0), (d + T_FRONT, y1, front_h - GAP), mat)]                        # front
    parts.append(((0, y0 + 2, 1.0), (d, y1 - 2, 1.0 + 1.2), mat_box))                     # bottom
    parts.append(((0, y0 + 2, 1.0), (d, y0 + 2 + 1.3, bh), "STAINLESS"))                  # LEGRABOX side L
    parts.append(((0, y1 - 2 - 1.3, 1.0), (d, y1 - 2, bh), "STAINLESS"))                  # side R
    parts.append(((0, y0 + 2, 1.0), (1.3, y1 - 2, bh), mat_box))                          # back
    return parts


def make_prism(name, poly, z0, z1, mat, col="Architecture", pivot=(0, 0, 0)):
    """Vertical prism from a UE-frame polygon (list of (x, y) cm, any winding, concave OK) between z0 and z1."""
    bm = bmesh.new()
    bot = [bm.verts.new(_ue_local_to_blender((x - pivot[0], y - pivot[1], z0 - pivot[2]))) for x, y in poly]
    top = [bm.verts.new(_ue_local_to_blender((x - pivot[0], y - pivot[1], z1 - pivot[2]))) for x, y in poly]
    bm.faces.new(bot)
    bm.faces.new(list(reversed(top)))
    n = len(poly)
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new([bot[i], bot[j], top[j], top[i]])
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 4])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    uv = bm.loops.layers.uv.new("UVMap")
    box_uv(bm, uv)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    me.materials.append(material(mat))
    ob = bpy.data.objects.new(name, me)
    ob.location = Vector((pivot[0] * CM, -pivot[1] * CM, pivot[2] * CM))
    collection(col).objects.link(ob)
    ob["noar_pivot_ue"] = list(pivot)
    ob["noar_yaw_ue"] = 0.0
    return ob


def seg_box(a, b, thick, z0, z1, mat):
    """Box part along an axis-aligned (or nearly) segment a->b, thickness centred on the line."""
    (x0, y0), (x1, y1) = a, b
    h = thick / 2.0
    if abs(x1 - x0) >= abs(y1 - y0):
        return ((min(x0, x1), y0 - h, z0), (max(x0, x1), y0 + h, z1), mat)
    return ((x0 - h, min(y0, y1), z0), (x0 + h, max(y0, y1), z1), mat)
