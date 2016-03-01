/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_WAYPOINT_ICON_RENDERER_HPP
#define XCSOAR_WAYPOINT_ICON_RENDERER_HPP

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
  enum Reachability
  {
    Invalid,
    Unreachable,
    ReachableStraight,
    ReachableTerrain,
  };

  WaypointIconRenderer(const WaypointRendererSettings &_settings,
                       const WaypointLook &_look,
                       Canvas &_canvas, bool _small_icons = false,
                       Angle _screen_rotation = Angle::Zero())
    :settings(_settings), look(_look),
     canvas(_canvas), small_icons(_small_icons),
     screen_rotation(_screen_rotation) {}

  void Draw(const Waypoint &waypoint, const PixelPoint &point,
            Reachability reachable = Unreachable, bool in_task = false);

private:
  void DrawLandable(const Waypoint &waypoint, const PixelPoint &point,
                    Reachability reachable = Unreachable);
};

#endif
