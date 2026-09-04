// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelRect;

/**
 * Draw a soft black drop shadow around the given rectangle, to make it
 * look like it floats above what is painted behind it.  The shadow
 * surrounds the rectangle evenly on all four sides, and its corners
 * are rounded, the way the corners of a blurred rectangle are.
 *
 * This must be called before the box itself is painted: the shadow is
 * drawn as a solid shape which is blurred at its edges, so the area
 * covered by the box gets painted over as well.
 *
 * The rectangle is relative to the current Canvas, and the shadow
 * extends beyond it, which means the caller must be allowed to paint
 * outside of its own window.
 *
 * This is implemented with OpenGL and does nothing on other platforms.
 */
void
DrawBoxShadow(const PixelRect &rc) noexcept;
