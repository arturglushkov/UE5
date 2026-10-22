"""
noar_plan_shell.py - whole-apartment shell from the vector floor plan (Data/<Apt>/plan_<apt>.json, extracted from
ID drawing D1.00 with Scripts/Geometry/extract_plan.py): wall faces, door headers, floor, ceiling, glazing, balcony.
Coordinates: UE cm, same origin as the prototype (kitchen west inner face x dining south face).
"""
import json
import math
import noar_geo as g

WALL_FACE_T = 2.0          # each poche outline line becomes a 2 cm skin; both faces of a wall are in the plan
DOOR_HEAD_90 = 229.0       # 90" interior doors
DOOR_HEAD_96 = 244.0       # 96" doors / wide openings
GL_H = 260.0


def poly_strips(poly, max_step=10.0):
    """Scanline decomposition of a (possibly concave) polygon into axis-aligned rectangles (x0, y0, x1, y1).
    Rows break at every vertex Y; slanted edges are approximated in rows of max_step cm."""
    ys = sorted({v[1] for v in poly})
    cuts = []
    for a, b in zip(ys, ys[1:]):
        n = max(1, int((b - a) // max_step)) if any(abs(p[0] - q[0]) > 0.5 and min(p[1], q[1]) <= a and max(p[1], q[1]) >= b
                                                   for p, q in zip(poly, poly[1:] + poly[:1])) else 1
        for k in range(n):
            cuts.append((a + (b - a) * k / n, a + (b - a) * (k + 1) / n))
    rects = []
    edges = list(zip(poly, poly[1:] + poly[:1]))
    for y0, y1 in cuts:
        ym = (y0 + y1) / 2
        xs = []
        for (ax, ay), (bx, by) in edges:
            if (ay <= ym < by) or (by <= ym < ay):
                xs.append(ax + (ym - ay) * (bx - ax) / (by - ay))
        xs.sort()
        for i in range(0, len(xs) - 1, 2):
            rects.append((xs[i], y0, xs[i + 1], y1))
    return rects


def slab(name, poly, z0, z1, mat, col):
    parts = [((x0, y0, z0), (x1, y1, z1), mat) for x0, y0, x1, y1 in poly_strips(poly)]
    for k in range(0, len(parts), 150):
        g.make_parts(name if len(parts) <= 150 else f"{name}_{k // 150 + 1:02d}", parts[k:k + 150], col=col, bevel_mm=0)


def build(plan_path, ceil, notes):
    p = json.load(open(plan_path))
    col = "Architecture"
    env = [tuple(v) for v in p["envelope"]]
    slab("SM_DEM_ARCH_FLOOR_PLAN", env, -2, 0, "FLOOR", col)
    slab("SM_DEM_ARCH_CEIL_PLAN", env, ceil, ceil + 2, "CEILING", col)
    parts = [g.seg_box(a, b, WALL_FACE_T, 0, ceil + 1, "PAINT") for a, b in p["walls"]]
    # split into a few meshes (Nanite / culling friendlier than one giant mesh)
    for k in range(0, len(parts), 80):
        g.make_parts(f"SM_DEM_ARCH_WALLS_PLAN_{k // 80 + 1:02d}", parts[k:k + 80], col=col, bevel_mm=0)
    heads = []
    for a, b in p["headers"]:
        w = math.dist(a, b)
        z = DOOR_HEAD_96 if w > 100 else DOOR_HEAD_90
        heads.append(g.seg_box(a, b, WALL_FACE_T, z, ceil + 1, "PAINT"))
    if heads:
        g.make_parts("SM_DEM_ARCH_HEADERS_PLAN", heads, col=col, bevel_mm=0)
    # glazing: aluminium frame + glass, header above
    for i, (a, b) in enumerate(p["glass"]):
        L = math.dist(a, b)
        n = max(1, int(round(L / 100)))
        dx, dy = (b[0] - a[0]) / n, (b[1] - a[1]) / n
        frame, glass = [], []
        for k in range(n):
            pa = (a[0] + dx * k, a[1] + dy * k)
            pb = (a[0] + dx * (k + 1), a[1] + dy * (k + 1))
            glass.append(g.seg_box(pa, pb, 1.0, 4, GL_H - 4, "GLASS"))
            frame.append(g.seg_box(pa, pb, 6.0, 0, 4, "METAL_BLACK"))
            frame.append(g.seg_box(pa, pb, 6.0, GL_H - 4, GL_H, "METAL_BLACK"))
            m = (pa[0] + (pb[0] - pa[0]) * 0.0, pa[1] + (pb[1] - pa[1]) * 0.0)
            ux, uy = (pb[0] - pa[0]), (pb[1] - pa[1])
            ln = math.hypot(ux, uy) or 1.0
            ux, uy = ux / ln * 2, uy / ln * 2
            frame.append(g.seg_box((m[0] - ux, m[1] - uy), (m[0] + ux, m[1] + uy), 6.0, 0, GL_H, "METAL_BLACK"))
        g.make_parts(f"SM_DEM_ARCH_GLAZING_{i + 1:02d}", frame + glass, col=col, bevel_mm=0)
        g.make_parts(f"SM_DEM_ARCH_GLAZING_HEAD_{i + 1:02d}", [g.seg_box(a, b, 14.0, GL_H, ceil + 1, "PAINT")], col=col, bevel_mm=0)
    notes("Glazing", "floor-to-ceiling 260 cm in ~1 m panels on every exterior opening", "window sizes are architectural, not in the ID set")
    # envelope skin: closes every gap of the traced outline that is not glazing (seen as black void otherwise)
    area = sum(a[0] * b[1] - b[0] * a[1] for a, b in zip(env, env[1:] + env[:1])) / 2.0
    sign = 1.0 if area > 0 else -1.0          # CCW -> outward normal is (dy, -dx)
    skin = []
    for a, b in zip(env, env[1:] + env[:1]):
        L = math.dist(a, b)
        if L < 1:
            continue
        ux, uy = (b[0] - a[0]) / L, (b[1] - a[1]) / L
        nx, ny = uy * sign, -ux * sign
        cover = []
        for ga, gb in p["glass"]:
            # glass on this edge: both ends within 30 cm of the edge line
            d1 = abs((ga[0] - a[0]) * nx + (ga[1] - a[1]) * ny)
            d2 = abs((gb[0] - a[0]) * nx + (gb[1] - a[1]) * ny)
            if d1 < 30 and d2 < 30:
                t1 = (ga[0] - a[0]) * ux + (ga[1] - a[1]) * uy
                t2 = (gb[0] - a[0]) * ux + (gb[1] - a[1]) * uy
                cover.append((max(0, min(t1, t2)), min(L, max(t1, t2))))
        cover.sort()
        t = 0.0
        free = []
        for c0, c1 in cover:
            if c0 > t:
                free.append((t, c0))
            t = max(t, c1)
        if t < L:
            free.append((t, L))
        off = 12.0
        for t0, t1 in free:
            if t1 - t0 < 1:
                continue
            pa = (a[0] + ux * t0 + nx * off, a[1] + uy * t0 + ny * off)
            pb = (a[0] + ux * t1 + nx * off, a[1] + uy * t1 + ny * off)
            if abs(ux) > 0.99 or abs(uy) > 0.99:
                skin.append(g.seg_box(pa, pb, 6.0, -2, ceil + 2, "PAINT"))
            else:   # slanted edge: approximate with short boxes
                n = max(1, int((t1 - t0) / 20))
                for k in range(n):
                    qa = (pa[0] + (pb[0] - pa[0]) * k / n, pa[1] + (pb[1] - pa[1]) * k / n)
                    qb = (pa[0] + (pb[0] - pa[0]) * (k + 1) / n, pa[1] + (pb[1] - pa[1]) * (k + 1) / n)
                    skin.append(g.seg_box(qa, qb, 8.0, -2, ceil + 2, "PAINT"))
    g.make_parts("SM_DEM_ARCH_ENVELOPE_SKIN", skin, col=col, bevel_mm=0)

    # balcony
    bal = [tuple(v) for v in p["balcony"]]
    slab("SM_DEM_ARCH_BALCONY_SLAB", bal, -20, -1, "CONCRETE", col)
    rail = []
    for a, b in zip(bal[1:3], bal[2:4]):    # east and south open edges
        L = math.dist(a, b)
        steps = max(1, int(L / 50))
        for k in range(steps):
            t0, t1 = k / steps, (k + 1) / steps
            pa = (a[0] + (b[0] - a[0]) * t0, a[1] + (b[1] - a[1]) * t0)
            pb = (a[0] + (b[0] - a[0]) * t1, a[1] + (b[1] - a[1]) * t1)
            rail.append(g.seg_box(pa, pb, 2.0, -1, 107, "GLASS"))
            rail.append(g.seg_box(pa, pb, 6.0, 107, 111, "METAL_BLACK"))
    g.make_parts("SM_DEM_ARCH_BALCONY_RAIL", rail, col=col, bevel_mm=0)
    notes("Balcony outline", "trapezoid traced from D1.00 (east edge slanted), glass rail 107 cm", "architectural set not provided")
    return p
