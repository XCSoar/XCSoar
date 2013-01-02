/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_TASK_VIEW_HPP
#define XCSOAR_TASK_VIEW_HPP

#include "Screen/Point.hpp"

class Canvas;
class OrderedTask;
class OrderedTaskPoint;
class RasterTerrain;
class Airspaces;
struct GeoPoint;
struct MapSettings;
struct TaskLook;
struct AirspaceLook;

void
PaintTask(Canvas &canvas, const PixelRect &rc, const OrderedTask &task,
          bool location_available, const GeoPoint &location,
          const MapSettings &settings_map,
          const TaskLook &task_look,
          const AirspaceLook &airspace_look,
          const RasterTerrain *terrain, const Airspaces *airspaces);

void
PaintTaskPoint(Canvas &canvas, const PixelRect &rc,
               const OrderedTask &task, const OrderedTaskPoint &point,
               bool location_available, const GeoPoint &location,
               const MapSettings &settings_map,
               const TaskLook &task_look,
               const AirspaceLook &airspace_look,
               const RasterTerrain *terrain, const Airspaces *airspaces);

#endif
