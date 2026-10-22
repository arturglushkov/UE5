"""render_preview.py - quick Workbench previews of Export/Dempster/DEM_Prototype.blend (QA, not a deliverable).
python3 Scripts/Geometry/render_preview.py"""
import bpy, math, os, sys
from mathutils import Vector
OUT = "Export/Dempster"
bpy.ops.wm.open_mainfile(filepath=os.path.join(OUT, "DEM_Prototype.blend"))
sc = bpy.context.scene
sc.render.engine = "BLENDER_WORKBENCH"; sc.display.shading.light = "STUDIO"; sc.display.shading.color_type = "MATERIAL"; sc.display.shading.show_cavity = True
sc.render.resolution_x, sc.render.resolution_y = 1600, 1100
B = lambda x, y, z: (x * 0.01, -y * 0.01, z * 0.01)   # UE cm -> Blender m (renders are mirrored L/R vs UE)
def shot(n, loc, tgt, fov=80, ortho=None, hide_ceiling=False):
    cd = bpy.data.cameras.new(n); ob = bpy.data.objects.new(n, cd); sc.collection.objects.link(ob)
    ob.location = loc; ob.rotation_euler = (Vector(tgt) - Vector(loc)).to_track_quat('-Z', 'Y').to_euler()
    if ortho: cd.type = "ORTHO"; cd.ortho_scale = ortho
    else: cd.lens_unit = "FOV"; cd.angle = math.radians(fov)
    cd.clip_end = 100; sc.camera = ob
    for o in bpy.data.objects:
        o.hide_render = hide_ceiling and o.type == "MESH" and any(k in o.name for k in ("CEIL", "SOFFIT", "BULKHEAD", "DRAPERY_POCKET"))
    sc.render.filepath = os.path.join(OUT, f"preview_{n}.png"); bpy.ops.render.render(write_still=True)
shot("top", B(345, 520, 1400), B(345, 520, 0), ortho=12.5, hide_ceiling=True)
shot("kitchen", B(600, 120, 170), B(150, 400, 100))
shot("dining", B(330, 490, 165), B(330, 0, 120))
shot("balcony", B(670, 300, 160), B(150, 330, 110))
