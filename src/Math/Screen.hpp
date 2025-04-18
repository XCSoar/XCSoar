// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/** Library for calculating on-screen coordinates */

#pragma once

#include <span>

struct BulkPixelPoint;
struct PixelPoint;
class Angle;

[[gnu::pure]]
PixelPoint
ScreenClosestPoint(const PixelPoint &p1, const PixelPoint &p2,
                   const PixelPoint &p3, int offset) noexcept;

/**
 * Shifts, rotates and scales the given polygon.
 *
 * @param poly Points specifying the polygon
 * @param shift The polygon is placed with position (0,0) centered here.
 * @param angle Angle of rotation
 * @param scale An input polygon with coordinates in the range -50 to +50
 *        is scaled to fill a square with the size of the 'scale' argument.
 *        (The scale value 100 preserves the size of the input polygon.)
 *        For best scaling precision, avoid 'scale' values smaller than
 *        the intended size of the polygon.
 */
void
PolygonRotateShift(std::span<BulkPixelPoint> poly, PixelPoint shift,
                   Angle angle, int scale = 100) noexcept;
