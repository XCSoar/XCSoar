#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The XCSoar Project

"""Draw a gesture icon from its stroke name.

The name is up, down, left, right, or a run of those letters
(ud, ldrdl). A return along the same line steps aside, so the
stroke stays visible. Add the icon by naming it in
Data/resources.txt; the build writes the SVG.
"""

from __future__ import annotations

import sys
from pathlib import Path

SIZE = 82
# Stroke box. The start dot and the end tick stay inside the frame.
BOX = 18
BOX_END = 64
DOT = 8
TICK = 6
# How far a reversal steps aside, in stroke lengths.
TURN = 0.4
# A stroke that would land on its start stops short of the dot.
CLOSE = 0.62

WORDS = {"up": "U", "down": "D", "left": "L", "right": "R"}
STEP = {"U": (0, -1), "D": (0, 1), "L": (-1, 0), "R": (1, 0)}


def sequence(name: str) -> str:
    name = name.removeprefix("gesture_")
    if name in WORDS:
        return WORDS[name]
    letters = name.upper()
    if not letters or any(ch not in STEP for ch in letters):
        raise SystemExit(f"not a gesture sequence: {name}")
    return letters


def trace(letters: str) -> list[tuple[float, float]]:
    points = [(0.0, 0.0)]
    x = y = 0.0
    previous = None
    for ch in letters:
        dx, dy = STEP[ch]
        if previous is not None and (dx, dy) == (-previous[0], -previous[1]):
            # Right of the stroke just drawn.
            x += previous[1] * TURN
            y += -previous[0] * TURN
            points.append((x, y))
        x += dx
        y += dy
        points.append((x, y))
        previous = (dx, dy)
    if len(points) >= 2:
        end = points[-1]
        for earlier in points[:-1]:
            same_x = abs(earlier[0] - end[0]) < 1e-6
            same_y = abs(earlier[1] - end[1]) < 1e-6
            if same_x and same_y:
                x0, y0 = points[-2]
                points[-1] = (x0 + CLOSE * (end[0] - x0),
                              y0 + CLOSE * (end[1] - y0))
                break
    return points


def place(points: list[tuple[float, float]]) -> list[tuple[float, float]]:
    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    span = max(max(xs) - min(xs), max(ys) - min(ys), 1.0)
    cx = (min(xs) + max(xs)) / 2
    cy = (min(ys) + max(ys)) / 2
    mid = (BOX + BOX_END) / 2
    scale = (BOX_END - BOX) / span
    return [(mid + (x - cx) * scale, mid + (y - cy) * scale)
            for x, y in points]


def svg(name: str) -> str:
    points = place(trace(sequence(name)))
    path = " ".join(
        ("M" if i == 0 else "L") + f"{x:.1f} {y:.1f}"
        for i, (x, y) in enumerate(points))
    x0, y0 = points[0]
    x1, y1 = points[-2]
    x2, y2 = points[-1]
    length = ((x2 - x1) ** 2 + (y2 - y1) ** 2) ** 0.5 or 1
    nx, ny = -(y2 - y1) / length, (x2 - x1) / length
    ink = ('fill="none" stroke="#3a5f8f" stroke-width="5"'
           ' stroke-linecap="round"')
    return (
        f'<svg xmlns="http://www.w3.org/2000/svg"'
        f' width="{SIZE}" height="{SIZE}"'
        f' viewBox="0 0 {SIZE} {SIZE}">\n'
        f'  <rect x="1" y="1" width="80" height="80" rx="4" ry="4"'
        f' fill="white" stroke="black" stroke-width="2"/>\n'
        f'  <path d="{path}" {ink} stroke-linejoin="round"/>\n'
        f'  <line x1="{x2 - nx * TICK:.1f}" y1="{y2 - ny * TICK:.1f}"'
        f' x2="{x2 + nx * TICK:.1f}" y2="{y2 + ny * TICK:.1f}" {ink}/>\n'
        f'  <circle cx="{x0:.1f}" cy="{y0:.1f}" r="{DOT}" fill="#3a5f8f"/>\n'
        f'</svg>\n'
    )


def self_check() -> None:
    down = trace("D")
    assert down == [(0.0, 0.0), (0.0, 1.0)]
    hairpin = trace("DU")
    assert hairpin[0] == (0.0, 0.0)
    assert hairpin[-1][1] == 0.0 and hairpin[-1][0] != 0.0
    loop = trace("ULDR")
    assert loop[-1] != (0.0, 0.0)
    root = Path(__file__).resolve().parents[1]
    text = (root / "Data" / "resources.txt").read_text()
    for line in text.splitlines():
        if '"gesture_' not in line:
            continue
        name = line.split('"')[1].removeprefix("gesture_")
        svg(name)


if __name__ == "__main__":
    if len(sys.argv) == 2 and sys.argv[1] == "--check":
        self_check()
    elif len(sys.argv) == 2:
        sys.stdout.write(svg(sys.argv[1]))
    else:
        raise SystemExit("usage: GenerateGestureSvg.py <name>|--check")
