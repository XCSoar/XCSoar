/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_WAY_POINT_RENDERER_HPP
#define XCSOAR_WAY_POINT_RENDERER_HPP

#include "Util/NonCopyable.hpp"
#include "Screen/Point.hpp"
#include "Engine/Waypoint/Waypoint.hpp"

#include <stddef.h>

struct SETTINGS_MAP;
struct WaypointLook;
class Canvas;
class LabelBlock;
class MapWindowProjection;
class Waypoints;
class GlidePolar;
class TaskBehaviour;
class ProtectedTaskManager;

/**
 * Renders way point icons and labels into a #Canvas.
 */
class WaypointRenderer : private NonCopyable {
  const Waypoints *way_points;

  const WaypointLook &look;

public:
  enum Reachability
  {
    Unreachable,
    ReachableStraight,
    ReachableTerrain,
  };

  WaypointRenderer(const Waypoints *_way_points,
                   const WaypointLook &_look)
    :way_points(_way_points), look(_look) {}

  void set_way_points(const Waypoints *_way_points) {
    way_points = _way_points;
  }

  void render(Canvas &canvas, LabelBlock &label_block,
              const MapWindowProjection &projection,
              const SETTINGS_MAP &settings_map,
              const TaskBehaviour &task_behaviour,
              const ProtectedTaskManager *task);
};

#endif
