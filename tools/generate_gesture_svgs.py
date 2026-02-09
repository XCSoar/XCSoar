#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The XCSoar Project
"""
Generate gesture SVG icons from gesture codes.

Gesture codes use U/D/L/R for Up/Down/Left/Right directions.
The resulting SVGs use Aresti notation: circle=start, dash=end.
"""

import os
import sys

# SVG dimensions and styling
WIDTH = 82
HEIGHT = 82
STROKE_COLOR = "#3a5f8f"
STROKE_WIDTH = 5
CIRCLE_RADIUS = 8

# Grid layout (3x3 grid within the 82x82 canvas)
# Positions: center=41, edges=16/66
GRID = {
    'center': 41,
    'min': 16,
    'max': 66,
}

# Direction vectors
DIRECTIONS = {
    'U': (0, -1),  # Up
    'D': (0, 1),   # Down
    'L': (-1, 0),  # Left
    'R': (1, 0),   # Right
}

# Map gesture codes to full names for filenames
GESTURE_NAMES = {
    'U': 'up',
    'D': 'down',
    'L': 'left',
    'R': 'right',
    'UD': 'ud',
    'DU': 'du',
    'DR': 'dr',
    'DL': 'dl',
    'RD': 'rd',
    'RL': 'rl',
    'URD': 'urd',
    'URDL': 'urdl',
    'LDR': 'ldr',
    'LDRDL': 'ldrdl',
    'ULDR': 'uldr',
}


def get_start_position(gesture_code):
    """Determine optimal start position based on gesture directions."""
    # Count movements in each direction
    up = gesture_code.count('U')
    down = gesture_code.count('D')
    left = gesture_code.count('L')
    right = gesture_code.count('R')
    
    # Start position should allow gesture to fit
    # If more down than up, start at top; if more up than down, start at bottom
    if down > up:
        y = GRID['min']
    elif up > down:
        y = GRID['max']
    else:
        y = GRID['center']
    
    # If more right than left, start at left; if more left than right, start at right
    if right > left:
        x = GRID['min']
    elif left > right:
        x = GRID['max']
    else:
        x = GRID['center']
    
    return x, y


def gesture_to_points(gesture_code):
    """Convert a gesture code to a list of (x, y) points."""
    x, y = get_start_position(gesture_code)
    points = [(x, y)]
    
    step = GRID['max'] - GRID['min']  # 50 pixels per step
    
    for direction in gesture_code:
        dx, dy = DIRECTIONS[direction]
        x += dx * step
        y += dy * step
        # Clamp to grid bounds
        x = max(GRID['min'], min(GRID['max'], x))
        y = max(GRID['min'], min(GRID['max'], y))
        points.append((x, y))
    
    return points


def points_to_path(points):
    """Convert points to SVG path data."""
    if not points:
        return ""
    
    path_data = f"M{points[0][0]} {points[0][1]}"
    for x, y in points[1:]:
        path_data += f" L{x} {y}"
    
    return path_data


def generate_end_dash(last_point, prev_point):
    """Generate the end dash (perpendicular to last segment)."""
    x, y = last_point
    px, py = prev_point
    
    # Determine dash orientation (perpendicular to movement)
    dx = x - px
    
    dash_len = 6
    if dx != 0:  # Horizontal movement -> vertical dash
        return f'<line x1="{x}" y1="{y - dash_len}" x2="{x}" y2="{y + dash_len}" stroke="{STROKE_COLOR}" stroke-width="{STROKE_WIDTH}" stroke-linecap="round"/>'
    else:  # Vertical movement -> horizontal dash
        return f'<line x1="{x - dash_len}" y1="{y}" x2="{x + dash_len}" y2="{y}" stroke="{STROKE_COLOR}" stroke-width="{STROKE_WIDTH}" stroke-linecap="round"/>'


def generate_svg(gesture_code):
    """Generate complete SVG for a gesture."""
    points = gesture_to_points(gesture_code)
    
    if len(points) < 2:
        return None
    
    path_data = points_to_path(points)
    start_x, start_y = points[0]
    end_dash = generate_end_dash(points[-1], points[-2])
    
    svg = f'''<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" height="{HEIGHT}" viewBox="0 0 {WIDTH} {HEIGHT}">
\t<rect x="1" y="1" width="80" height="80" rx="4" ry="4" fill="white" stroke="black" stroke-width="2"/>
\t<path d="{path_data}" fill="none" stroke="{STROKE_COLOR}" stroke-width="{STROKE_WIDTH}" stroke-linecap="round" stroke-linejoin="round"/>
\t<circle cx="{start_x}" cy="{start_y}" r="{CIRCLE_RADIUS}" fill="{STROKE_COLOR}"/>
\t{end_dash}
</svg>
'''
    return svg


def main():
    output_dir = os.path.join(os.path.dirname(__file__), '..', 'doc', 'manual', 'figures')
    
    if len(sys.argv) > 1:
        output_dir = sys.argv[1]
    
    os.makedirs(output_dir, exist_ok=True)
    
    for code, name in GESTURE_NAMES.items():
        svg_content = generate_svg(code)
        if svg_content:
            filename = f"gesture_{name}.svg"
            filepath = os.path.join(output_dir, filename)
            with open(filepath, 'w') as f:
                f.write(svg_content)
            print(f"Generated: {filename}")


if __name__ == '__main__':
    main()
