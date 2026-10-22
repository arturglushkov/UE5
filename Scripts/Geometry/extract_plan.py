"""
extract_plan.py - reads the vector floor plan (ID drawing D1.00) and writes Data/<Apt>/plan_<apt>.json in UE cm.

    pip install pymupdf
    python3 Scripts/Geometry/extract_plan.py <ID_DRAWINGS.pdf> [page_index=6]

What is automatic: wall outlines (the 0.84 / 1.26 pt poche lines), door headers (colinear wall faces with a 25-80 pt gap),
room labels. What is traced by hand below (ENV_PX / GLASS_PX / BAL_PX, pixel coords of the 3 px/pt raster / 2):
apartment envelope, exterior glazing, balcony outline - the drawing does not carry them as clean polylines.
Scale: 1/4" = 1'-0" -> 1 pt = 1.693333 cm. Origin: kitchen west wall inner face (x 522.36 pt) x dining south face (y 732.04 pt).
"""
import sys
import json
import os
import pymupdf as fitz

K = 1.693333
OX, OY = 522.36, 732.04
ENV_PX = [(470, 25), (1140, 25), (1140, 462), (820, 462), (820, 500), (822, 820), (745, 840), (745, 1080), (975, 1080), (975, 1765),
          (590, 1765), (590, 1705), (25, 1705), (25, 1175), (70, 1175), (70, 1085), (210, 1085), (210, 440), (478, 440)]
GLASS_PX = [((821, 500), (821, 820)), ((878, 462), (1058, 462)), ((1140, 85), (1140, 392)), ((968, 1500), (968, 1680)),
            ((693, 1762), (882, 1762)), ((140, 1705), (290, 1705)), ((375, 1705), (525, 1705))]
BAL_PX = [(822, 470), (1290, 470), (1200, 1765), (975, 1765), (975, 1080), (745, 1080), (745, 840), (822, 820)]


def U(x, y):
    return (round((x - OX) * K, 1), round(-(y - OY) * K, 1))


def PX(px, py):
    return (px * 0.69 + 370, py * 0.69 + 120)


def main(pdf, page=6):
    doc = fitz.open(pdf)
    p = doc[page]
    M = p.rotation_matrix
    polys = []
    for d in p.get_drawings():
        if d["type"] != "s" or round(d.get("width") or 0, 2) not in (0.84, 1.26):
            continue
        pts = []
        for it in d["items"]:
            if it[0] == "l":
                a, b = it[1] * M, it[2] * M
                if not pts or abs(pts[-1][0] - a.x) > 0.01 or abs(pts[-1][1] - a.y) > 0.01:
                    if pts:
                        polys.append(pts)
                    pts = [(a.x, a.y)]
                pts.append((b.x, b.y))
            elif pts:
                polys.append(pts)
                pts = []
        if pts:
            polys.append(pts)
    polys = [pl for pl in polys if all(abs(a[0] - b[0]) < 0.05 or abs(a[1] - b[1]) < 0.05 for a, b in zip(pl, pl[1:]))]
    segs = [(a, b) for pl in polys if max(x for x, _ in pl) <= 1400 and min(y for _, y in pl) <= 1440
            for a, b in zip(pl, pl[1:]) if abs(a[0] - b[0]) + abs(a[1] - b[1]) >= 0.5]

    def horiz(s):
        return abs(s[0][1] - s[1][1]) < 0.3
    hdr = set()
    for i, s in enumerate(segs):
        for t in segs[i + 1:]:
            for H in (True, False):
                if horiz(s) != H or horiz(t) != H:
                    continue
                c = 1 if H else 0     # coordinate that must match (y for horizontal)
                v = 0 if H else 1     # running coordinate
                if abs(s[0][c] - t[0][c]) >= 0.5:
                    continue
                sa = sorted([s[0][v], s[1][v]])
                ta = sorted([t[0][v], t[1][v]])
                for lo, hi in ((sa[1], ta[0]), (ta[1], sa[0])):
                    if 25 <= hi - lo <= 80 and not any(horiz(u) == H and abs(u[0][c] - s[0][c]) < 0.5 and
                                                       min(u[0][v], u[1][v]) < hi - 0.5 and max(u[0][v], u[1][v]) > lo + 0.5 for u in segs):
                        a = (lo, s[0][c]) if H else (s[0][c], lo)
                        b = (hi, s[0][c]) if H else (s[0][c], hi)
                        hdr.add((round(a[0], 1), round(a[1], 1), round(b[0], 1), round(b[1], 1)))
    labels = []
    for w in p.get_text("words"):
        r = fitz.Rect(w[:4]) * M
        if r.x0 < 1400:
            labels.append([U((r.x0 + r.x1) / 2, (r.y0 + r.y1) / 2), w[4]])
    out = dict(frame="UE cm: X east, Y north; origin = kitchen west inner face x dining south face", scale_cm_per_pt=K,
               walls=[[U(*a), U(*b)] for a, b in segs], headers=[[U(h[0], h[1]), U(h[2], h[3])] for h in hdr],
               envelope=[U(*PX(*q)) for q in ENV_PX], glass=[[U(*PX(*a)), U(*PX(*b))] for a, b in GLASS_PX],
               balcony=[U(*PX(*q)) for q in BAL_PX], labels=labels)
    dst = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "Data", "Dempster", "plan_dempster.json")
    json.dump(out, open(dst, "w"), indent=0)
    print(f"{len(segs)} wall faces, {len(hdr)} headers -> {dst}")


if __name__ == "__main__":
    main(sys.argv[1], int(sys.argv[2]) if len(sys.argv) > 2 else 6)
