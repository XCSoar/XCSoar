// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelPoint;
class Angle;

/**
 * Temporarily changes the transformation matrix. Meant as replacement
 * for PolygonRotateShift().
 *
 * WARNING: Pen widths also get scaled!
 */
class CanvasRotateShift
{
public:
  CanvasRotateShift(PixelPoint pos, Angle angle,
                    float scale=1.f) noexcept;

  ~CanvasRotateShift() noexcept;

  CanvasRotateShift(const CanvasRotateShift &) = delete;
  CanvasRotateShift &operator=(const CanvasRotateShift &) = delete;
};
