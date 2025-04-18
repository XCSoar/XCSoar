// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WaypointReachability.hpp"
#include "Math/Angle.hpp"

struct PixelPoint;
struct WaypointRendererSettings;
struct WaypointLook;
class Canvas;
struct Waypoint;

class WaypointIconRenderer
{
  const WaypointRendererSettings &settings;
  const WaypointLook &look;
  Canvas &canvas;
  bool small_icons;
  Angle screen_rotation;

public:
  WaypointIconRenderer(const WaypointRendererSettings &_settings,
                       const WaypointLook &_look,
                       Canvas &_canvas, bool _small_icons = false,
                       Angle _screen_rotation = Angle::Zero()) noexcept
    :settings(_settings), look(_look),
     canvas(_canvas), small_icons(_small_icons),
     screen_rotation(_screen_rotation) {}

  void Draw(const Waypoint &waypoint, const PixelPoint &point,
            WaypointReachability reachable=WaypointReachability::UNREACHABLE,
            bool in_task = false) noexcept;

private:
  void DrawLandable(const Waypoint &waypoint, const PixelPoint &point,
                    WaypointReachability reachable=WaypointReachability::UNREACHABLE) noexcept;
};
