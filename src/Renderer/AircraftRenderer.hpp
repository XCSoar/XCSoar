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
}
