// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelPoint;
struct MapSettings;
struct AircraftLook;
class Canvas;
class Angle;

namespace AircraftRenderer
{
  void Draw(Canvas &canvas, const MapSettings &settings_map,
            const AircraftLook &look,
            Angle angle, PixelPoint aircraft_pos);

  /**
   * Draw the simple aircraft outline.  #scale is a
   * PolygonRotateShift() scale (coordinates in +/-50 map to #scale
   * pixels); use this for list icons sized to a row.
   */
  void DrawSimple(Canvas &canvas, const AircraftLook &look,
                  Angle angle, PixelPoint aircraft_pos,
                  int scale, bool large = false) noexcept;
}
