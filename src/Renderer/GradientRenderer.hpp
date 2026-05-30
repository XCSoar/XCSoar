// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Angle;
class Canvas;
struct PixelRect;
class Color;
struct PixelPoint;

/**
 * Fill the specified rectangle with a color gradient.
 *
 * If  gradient drawing is not supported by the platform or the compile-
 * time option "EYE_CANDY" is disabled the solid fallback color is used.
 */
void DrawVerticalGradient(Canvas &canvas, const PixelRect &rc,
                          Color top_color, Color bottom_color,
                          Color fallback_color);

/**
 * Draw a vertical gradient using banded Canvas::DrawFilledRectangle()
 * calls.  Unlike DrawVerticalGradient() this works correctly inside
 * translated SubCanvas contexts (e.g. child windows) because it uses
 * the Canvas API rather than raw GL calls.
 */
void DrawBandedVerticalGradient(Canvas &canvas, const PixelRect &rc,
                                Color top_color, Color bottom_color);

/**
 * Sunken shadow on both edges of an annulus arc, fading toward the centre.
 * Uses smooth OpenGL vertex colours when EYE_CANDY is enabled; otherwise
 * banded Canvas::DrawAnnulus() strips.
 */
void DrawAnnulusEdgeShadowFade(Canvas &canvas, PixelPoint center,
                               unsigned inner_radius, unsigned outer_radius,
                               Angle arc_start, Angle arc_end,
                               Color face_color) noexcept;
