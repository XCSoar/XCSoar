// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/**
 * @file
 * @brief Small Windows GDI helper functions
 */

#pragma once

class Canvas;
class Angle;
struct PixelPoint;
struct PixelRect;

bool
Segment(Canvas &canvas, PixelPoint center, unsigned radius,
        Angle start, Angle end, bool horizon=false) noexcept;

bool
Annulus(Canvas &canvas, PixelPoint center, unsigned radius,
        Angle start, Angle end, unsigned inner_radius) noexcept;

bool
KeyHole(Canvas &canvas, PixelPoint center, unsigned radius,
        Angle start, Angle end, unsigned inner_radius) noexcept;

void
RoundRect(Canvas &canvas, PixelRect r, unsigned radius) noexcept;

bool
Arc(Canvas &canvas, PixelPoint center, unsigned radius,
    Angle start, Angle end) noexcept;
