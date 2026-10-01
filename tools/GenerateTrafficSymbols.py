#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The XCSoar Project

"""Convert the FLARM aircraft-type icons in Data/icons/traffic/*.svg into
polygon tables for src/Renderer/AircraftTypeSymbolData.cpp.

Each SVG is a 28x28 icon consisting of a white "halo" silhouette and a
black glyph on top of it.  Both are exported: the renderer fills the
halo with the traffic colour and draws the glyph in the outline colour
on top, which reproduces the icon design with polygons that work on
every canvas backend (OpenGL, GDI, memory canvas / Kobo).

Coordinates are emitted in a template space where the 28-unit viewBox
maps to -500..+500, centred on the icon centre.

Usage: tools/GenerateTrafficSymbols.py > src/Renderer/AircraftTypeSymbolData.cpp
"""

import math
import re
import sys
import os
import xml.etree.ElementTree as ET

SVG_NS = "{http://www.w3.org/2000/svg}"
ICON_DIR = os.path.join(os.path.dirname(__file__), "..", "Data", "icons", "traffic")

# FLARM aircraft type -> SVG basename (see FlarmTraffic::AircraftType).
# Unknown, flying saucer and reserved share the generic "unknown" icon.
TYPES = {
    0: "unknown",
    1: "glider",
    2: "tow_plane",
    3: "helicopter",
    4: "parachute",
    5: "drop_plane",
    6: "hang_glider",
    7: "para_glider",
    8: "powered_aircraft",
    9: "jet_aircraft",
    10: "unknown",
    11: "balloon",
    12: "airship",
    13: "uav",
    14: "unknown",
    15: "static_object",
}

MAX_POINTS = 96       # per polygon, keep the render-time stack buffer small
CURVE_SEGMENTS = 8    # line segments per Bezier curve before simplification
ARC_STEP_DEG = 12.0   # arc sampling step before simplification
SIMPLIFY_TOLERANCE = 0.12  # viewBox units (~4 template units)


# ---------------------------------------------------------------- transforms

def mat_mul(a, b):
    """Multiply two affine matrices given as (a, b, c, d, e, f)."""
    a0, b0, c0, d0, e0, f0 = a
    a1, b1, c1, d1, e1, f1 = b
    return (a0 * a1 + c0 * b1,
            b0 * a1 + d0 * b1,
            a0 * c1 + c0 * d1,
            b0 * c1 + d0 * d1,
            a0 * e1 + c0 * f1 + e0,
            b0 * e1 + d0 * f1 + f0)


def mat_apply(m, x, y):
    a, b, c, d, e, f = m
    return (a * x + c * y + e, b * x + d * y + f)


IDENTITY = (1.0, 0.0, 0.0, 1.0, 0.0, 0.0)


def parse_transform(text):
    m = IDENTITY
    if not text:
        return m
    for name, args in re.findall(r"(\w+)\s*\(([^)]*)\)", text):
        v = [float(x) for x in re.split(r"[\s,]+", args.strip()) if x]
        if name == "matrix":
            t = tuple(v)
        elif name == "translate":
            t = (1, 0, 0, 1, v[0], v[1] if len(v) > 1 else 0.0)
        elif name == "scale":
            sx = v[0]
            sy = v[1] if len(v) > 1 else sx
            t = (sx, 0, 0, sy, 0, 0)
        elif name == "rotate":
            r = math.radians(v[0])
            t = (math.cos(r), math.sin(r), -math.sin(r), math.cos(r), 0, 0)
            if len(v) == 3:
                t = mat_mul(mat_mul((1, 0, 0, 1, v[1], v[2]), t),
                            (1, 0, 0, 1, -v[1], -v[2]))
        else:
            raise ValueError("unsupported transform " + name)
        m = mat_mul(m, t)
    return m


# ------------------------------------------------------------- path parsing

NUMBER_RE = re.compile(r"[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?")


def tokenize_path(d):
    tokens = []
    for m in re.finditer(r"[MmLlHhVvCcSsQqTtAaZz]|" + NUMBER_RE.pattern, d):
        tokens.append(m.group(0))
    return tokens


def bezier_cubic(p0, p1, p2, p3, n):
    pts = []
    for i in range(1, n + 1):
        t = i / n
        u = 1 - t
        x = u * u * u * p0[0] + 3 * u * u * t * p1[0] + 3 * u * t * t * p2[0] + t * t * t * p3[0]
        y = u * u * u * p0[1] + 3 * u * u * t * p1[1] + 3 * u * t * t * p2[1] + t * t * t * p3[1]
        pts.append((x, y))
    return pts


def bezier_quad(p0, p1, p2, n):
    pts = []
    for i in range(1, n + 1):
        t = i / n
        u = 1 - t
        x = u * u * p0[0] + 2 * u * t * p1[0] + t * t * p2[0]
        y = u * u * p0[1] + 2 * u * t * p1[1] + t * t * p2[1]
        pts.append((x, y))
    return pts


def arc_points(p0, rx, ry, phi_deg, large_arc, sweep, p1):
    """SVG elliptical arc (endpoint parameterisation) -> list of points."""
    x1, y1 = p0
    x2, y2 = p1
    if rx == 0 or ry == 0:
        return [p1]
    rx, ry = abs(rx), abs(ry)
    phi = math.radians(phi_deg)
    cos_phi, sin_phi = math.cos(phi), math.sin(phi)
    dx2, dy2 = (x1 - x2) / 2.0, (y1 - y2) / 2.0
    x1p = cos_phi * dx2 + sin_phi * dy2
    y1p = -sin_phi * dx2 + cos_phi * dy2
    lam = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry)
    if lam > 1:
        s = math.sqrt(lam)
        rx *= s
        ry *= s
    num = rx * rx * ry * ry - rx * rx * y1p * y1p - ry * ry * x1p * x1p
    den = rx * rx * y1p * y1p + ry * ry * x1p * x1p
    coef = math.sqrt(max(0.0, num / den)) if den else 0.0
    if large_arc == sweep:
        coef = -coef
    cxp = coef * rx * y1p / ry
    cyp = -coef * ry * x1p / rx
    cx = cos_phi * cxp - sin_phi * cyp + (x1 + x2) / 2.0
    cy = sin_phi * cxp + cos_phi * cyp + (y1 + y2) / 2.0

    def angle(ux, uy, vx, vy):
        dot = ux * vx + uy * vy
        length = math.hypot(ux, uy) * math.hypot(vx, vy)
        a = math.acos(max(-1.0, min(1.0, dot / length)))
        if ux * vy - uy * vx < 0:
            a = -a
        return a

    theta1 = angle(1, 0, (x1p - cxp) / rx, (y1p - cyp) / ry)
    dtheta = angle((x1p - cxp) / rx, (y1p - cyp) / ry,
                   (-x1p - cxp) / rx, (-y1p - cyp) / ry)
    if not sweep and dtheta > 0:
        dtheta -= 2 * math.pi
    elif sweep and dtheta < 0:
        dtheta += 2 * math.pi

    n = max(1, int(math.ceil(abs(math.degrees(dtheta)) / ARC_STEP_DEG)))
    pts = []
    for i in range(1, n + 1):
        t = theta1 + dtheta * i / n
        ex = rx * math.cos(t)
        ey = ry * math.sin(t)
        pts.append((cos_phi * ex - sin_phi * ey + cx,
                    sin_phi * ex + cos_phi * ey + cy))
    pts[-1] = p1
    return pts


def flatten_path(d):
    """Return a list of closed subpaths, each a list of (x, y) tuples."""
    tokens = tokenize_path(d)
    i = 0
    subpaths = []
    current = []
    pos = (0.0, 0.0)
    start = (0.0, 0.0)
    last_ctrl = None
    cmd = None

    def num():
        nonlocal i
        v = float(tokens[i])
        i += 1
        return v

    while i < len(tokens):
        if re.match(r"[A-Za-z]", tokens[i]):
            cmd = tokens[i]
            i += 1
        elif cmd in ("M", "m"):
            # implicit lineto after moveto
            cmd = "L" if cmd == "M" else "l"
        rel = cmd.islower()
        c = cmd.upper()

        if c == "M":
            if current:
                subpaths.append(current)
            x, y = num(), num()
            if rel:
                x, y = pos[0] + x, pos[1] + y
            pos = start = (x, y)
            current = [pos]
            last_ctrl = None
        elif c == "Z":
            if current:
                subpaths.append(current)
            current = []
            pos = start
            last_ctrl = None
        elif c == "L":
            x, y = num(), num()
            if rel:
                x, y = pos[0] + x, pos[1] + y
            pos = (x, y)
            current.append(pos)
            last_ctrl = None
        elif c == "H":
            x = num()
            if rel:
                x += pos[0]
            pos = (x, pos[1])
            current.append(pos)
            last_ctrl = None
        elif c == "V":
            y = num()
            if rel:
                y += pos[1]
            pos = (pos[0], y)
            current.append(pos)
            last_ctrl = None
        elif c in ("C", "S"):
            if c == "C":
                x1, y1 = num(), num()
                if rel:
                    x1, y1 = pos[0] + x1, pos[1] + y1
            else:
                if last_ctrl is not None:
                    x1, y1 = 2 * pos[0] - last_ctrl[0], 2 * pos[1] - last_ctrl[1]
                else:
                    x1, y1 = pos
            x2, y2 = num(), num()
            x, y = num(), num()
            if rel:
                x2, y2 = pos[0] + x2, pos[1] + y2
                x, y = pos[0] + x, pos[1] + y
            current.extend(bezier_cubic(pos, (x1, y1), (x2, y2), (x, y),
                                        CURVE_SEGMENTS))
            last_ctrl = (x2, y2)
            pos = (x, y)
        elif c in ("Q", "T"):
            if c == "Q":
                x1, y1 = num(), num()
                if rel:
                    x1, y1 = pos[0] + x1, pos[1] + y1
            else:
                if last_ctrl is not None:
                    x1, y1 = 2 * pos[0] - last_ctrl[0], 2 * pos[1] - last_ctrl[1]
                else:
                    x1, y1 = pos
            x, y = num(), num()
            if rel:
                x, y = pos[0] + x, pos[1] + y
            current.extend(bezier_quad(pos, (x1, y1), (x, y), CURVE_SEGMENTS))
            last_ctrl = (x1, y1)
            pos = (x, y)
        elif c == "A":
            rx, ry, rot = num(), num(), num()
            large = int(num()) != 0
            sweep = int(num()) != 0
            x, y = num(), num()
            if rel:
                x, y = pos[0] + x, pos[1] + y
            current.extend(arc_points(pos, rx, ry, rot, large, sweep, (x, y)))
            pos = (x, y)
            last_ctrl = None
        else:
            raise ValueError("unsupported path command " + cmd)

    if current:
        subpaths.append(current)
    return subpaths


# ------------------------------------------------------------ simplification

def point_line_distance(p, a, b):
    if a == b:
        return math.hypot(p[0] - a[0], p[1] - a[1])
    dx, dy = b[0] - a[0], b[1] - a[1]
    t = ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / (dx * dx + dy * dy)
    t = max(0.0, min(1.0, t))
    return math.hypot(p[0] - (a[0] + t * dx), p[1] - (a[1] + t * dy))


def douglas_peucker(points, tolerance):
    if len(points) < 3:
        return list(points)
    dmax, index = 0.0, 0
    for k in range(1, len(points) - 1):
        d = point_line_distance(points[k], points[0], points[-1])
        if d > dmax:
            dmax, index = d, k
    if dmax > tolerance:
        left = douglas_peucker(points[:index + 1], tolerance)
        right = douglas_peucker(points[index:], tolerance)
        return left[:-1] + right
    return [points[0], points[-1]]


def simplify_closed(points, tolerance):
    """Douglas-Peucker for a closed ring; the ring is split at the point
    farthest from the first one so that both halves get simplified."""
    if len(points) < 4:
        return list(points)
    if points[0] == points[-1]:
        points = points[:-1]
    far = max(range(len(points)),
              key=lambda k: math.hypot(points[k][0] - points[0][0],
                                       points[k][1] - points[0][1]))
    first = douglas_peucker(points[:far + 1], tolerance)
    second = douglas_peucker(points[far:] + [points[0]], tolerance)
    ring = first[:-1] + second[:-1]
    return ring


# ----------------------------------------------------------------- SVG walk

def style_value(elem, name):
    v = elem.get(name)
    if v is not None:
        return v.strip()
    style = elem.get("style", "")
    m = re.search(r"(?:^|;)\s*" + name + r"\s*:\s*([^;]+)", style)
    return m.group(1).strip() if m else None


def is_white(v):
    return v is not None and v.lower() in ("#fff", "#ffffff", "white")


def walk(elem, transform, out):
    t = mat_mul(transform, parse_transform(elem.get("transform")))
    tag = elem.tag.replace(SVG_NS, "")
    if tag in ("path", "ellipse", "circle"):
        out.append((tag, elem, t))
    for child in elem:
        walk(child, t, out)


ELLIPSE_SEGMENTS = 24


def ellipse_points(elem):
    cx = float(elem.get("cx", "0"))
    cy = float(elem.get("cy", "0"))
    if elem.tag.endswith("circle"):
        rx = ry = float(elem.get("r"))
    else:
        rx = float(elem.get("rx"))
        ry = float(elem.get("ry"))
    return [(cx + rx * math.cos(2 * math.pi * i / ELLIPSE_SEGMENTS),
             cy + ry * math.sin(2 * math.pi * i / ELLIPSE_SEGMENTS))
            for i in range(ELLIPSE_SEGMENTS)]


def load_icon(filename):
    """Return (viewbox, halo polygons, glyph polygons, highlights) of
    one icon in viewBox coordinates.  The halo is the union of the
    white-filled paths, the glyph the union of the other paths and
    ellipses; white ellipses are highlights inside the glyph that show
    the halo colour again."""
    tree = ET.parse(filename)
    root = tree.getroot()
    viewbox = [float(v) for v in root.get("viewBox").split()]
    if viewbox[2] != viewbox[3]:
        raise ValueError(filename + ": viewBox must be square")

    paths = []
    walk(root, IDENTITY, paths)

    halo, glyph, highlights = [], [], []
    for tag, elem, t in paths:
        white = is_white(style_value(elem, "fill"))
        if tag == "path":
            target = halo if white else glyph
            for sub in flatten_path(elem.get("d")):
                target.append([mat_apply(t, x, y) for x, y in sub])
        else:
            target = highlights if white else glyph
            target.append([mat_apply(t, x, y) for x, y in ellipse_points(elem)])

    if not halo:
        raise ValueError(filename + ": no white halo path found")
    if not glyph:
        raise ValueError(filename + ": no glyph path found")
    return viewbox, halo, glyph, highlights


def signed_area(points):
    a = 0.0
    for i in range(len(points)):
        x1, y1 = points[i]
        x2, y2 = points[(i + 1) % len(points)]
        a += x1 * y2 - x2 * y1
    return a / 2


def to_template(viewbox, polygons, force_hole=None):
    """Scale viewBox coordinates to the -500..+500 template space.

    Returns a list of (points, hole) tuples; a polygon whose winding is
    opposite to that of the largest polygon is a hole."""
    size = viewbox[2]
    cx = viewbox[0] + size / 2
    cy = viewbox[1] + size / 2
    scale = 1000.0 / size
    if not polygons:
        return []
    outer_sign = signed_area(max(polygons, key=lambda p: abs(signed_area(p)))) > 0

    result = []
    for pts in polygons:
        hole = (signed_area(pts) > 0) != outer_sign
        if force_hole is not None:
            hole = force_hole
        tolerance = SIMPLIFY_TOLERANCE
        ring = simplify_closed(pts, tolerance)
        while len(ring) > MAX_POINTS:
            tolerance *= 1.5
            ring = simplify_closed(pts, tolerance)
        ints = []
        for x, y in ring:
            p = (int(round((x - cx) * scale)), int(round((y - cy) * scale)))
            if not ints or ints[-1] != p:
                ints.append(p)
        if len(ints) > 1 and ints[0] == ints[-1]:
            ints.pop()
        if len(ints) >= 3:
            result.append((ints, hole))
    return result


# ------------------------------------------------------------------- output

def main():
    out = sys.stdout
    out.write("// SPDX-License-Identifier: GPL-2.0-or-later\n")
    out.write("// Copyright The XCSoar Project\n\n")
    out.write("/*\n * Generated by tools/GenerateTrafficSymbols.py from\n"
              " * the SVG icons in Data/icons/traffic -- do not edit by hand.\n"
              " *\n"
              " * Coordinates are in template space: the 28-unit icon box\n"
              " * spans -500..+500, north is up (-y).\n"
              " * Each type has a halo (filled with the traffic colour) and\n"
              " * a glyph drawn on top in the outline colour; the flag marks\n"
              " * holes and highlights in the glyph, which show the traffic\n"
              " * colour again.\n"
              " */\n\n")
    out.write('#include "AircraftTypeSymbolData.hpp"\n\n')

    def write_polygons(prefix, polygons):
        arrays = []
        for k, (ring, hole) in enumerate(polygons):
            ident = f"{prefix}_{k}"
            arrays.append((ident, hole))
            out.write(f"static constexpr BulkPixelPoint {ident}[] = {{\n")
            line = " "
            for x, y in ring:
                item = f" {{{x}, {y}}},"
                if len(line) + len(item) > 78:
                    out.write(line + "\n")
                    line = " "
                line += item
            out.write(line + "\n};\n\n")
        out.write(f"static constexpr AircraftTypeSymbolPolygon {prefix}[] = {{\n")
        for ident, hole in arrays:
            out.write(f"  {{ {ident}, {'true' if hole else 'false'} }},\n")
        out.write("};\n\n")

    for name in sorted(set(TYPES.values())):
        filename = os.path.join(ICON_DIR, name + ".svg")
        viewbox, halo, glyph, highlights = load_icon(filename)
        write_polygons(f"{name}_halo", to_template(viewbox, halo))
        write_polygons(f"{name}_glyph",
                       to_template(viewbox, glyph) +
                       to_template(viewbox, highlights, force_hole=True))

    out.write("const AircraftTypeSymbol aircraft_type_symbols[16] = {\n")
    for type_id in range(16):
        if type_id in TYPES:
            name = TYPES[type_id]
            out.write(f"  /* {type_id:2d} */ {{ {name}_halo, {name}_glyph }},\n")
        else:
            out.write(f"  /* {type_id:2d} */ {{ {{}}, {{}} }},\n")
    out.write("};\n")


if __name__ == "__main__":
    main()
