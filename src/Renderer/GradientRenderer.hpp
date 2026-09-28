// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Canvas;
struct PixelRect;
class Color;

/**
 * Fill the specified rectangle with a color gradient.
 *
 * If  gradient drawing is not supported by the platform or the compile-
 * time option "EYE_CANDY" is disabled the solid fallback color is used.
 */
void DrawVerticalGradient(Canvas &canvas, const PixelRect &rc,
                          Color top_color, Color bottom_color,
                          Color fallback_color);
