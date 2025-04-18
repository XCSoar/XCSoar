// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Canvas;
struct PixelRect;
class Color;

/**
 * Fill the background with the specified color and a glass-like
 * effect.
 *
 * This is only implemented on OpenGL when the specified color is
 * bright white.  The compile-time option "EYE_CANDY" must be enabled.
 * In all other cases, the background is simply filled with the
 * specified color, without any additional effect.
 */
void
DrawGlassBackground(Canvas &canvas, const PixelRect &rc, Color color) noexcept;
