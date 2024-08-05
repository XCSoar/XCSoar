// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelRect;
struct HorizonLook;
struct AttitudeState;
class Canvas;

struct PixelPoint;
class Angle;

namespace HorizonRenderer
{
  void Draw(Canvas &canvas, const PixelRect &rc,
            const HorizonLook &look,
            const AttitudeState &attitude);

  int lines_intersect(PixelPoint p1, PixelPoint p2, PixelPoint p3,
                      PixelPoint p4, PixelPoint &intersect);

  void rotate(PixelPoint point, PixelPoint center, Angle a,
              PixelPoint &rotated);

  void drawAircraftSymbol(Canvas &canvas, const PixelPoint &center, int radius,
                          const HorizonLook &look);

  } // namespace HorizonRenderer
