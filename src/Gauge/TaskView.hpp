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

#ifndef XCSOAR_TASK_VIEW_HPP
#define XCSOAR_TASK_VIEW_HPP

struct PixelRect;
class Canvas;
class OrderedTask;
class OrderedTaskPoint;
class RasterTerrain;
class Airspaces;
class WindowProjection;
struct GeoPoint;
struct MapSettings;
struct TaskLook;
struct AirspaceLook;
struct OverlayLook;

/**
 * Draw Task with the given projection.
 *
 * @param fai_sectors render FAI triangle sectors?
 * @highlight_index highlight the task point as beeing manually edited
 */
void
PaintTask(Canvas &canvas, const WindowProjection &projection,
          const OrderedTask &task,
          const GeoPoint &location,
          const MapSettings &settings_map,
          const TaskLook &task_look,
          const AirspaceLook &airspace_look,
          const RasterTerrain *terrain, const Airspaces *airspaces,
          bool fai_sectors,
          int highlight_index);

/**
 * Draw the whole Task into a rectangle.
 *
 * @param fai_sectors render FAI triangle sectors?
 * @highlight_index highlight the task point as beeing manually edited
 */
void
PaintTask(Canvas &canvas, const PixelRect &rc, const OrderedTask &task,
          const GeoPoint &location,
          const MapSettings &settings_map,
          const TaskLook &task_look,
          const AirspaceLook &airspace_look,
          const OverlayLook &overlay_look,
          const RasterTerrain *terrain, const Airspaces *airspaces,
          bool fai_sectors=false,
          int highlight_index = -1);

/**
 * Draw a detailed view of a TaskPoint into a rectangle.
 * @highlight_index highlight the task point as beeing manually edited
 */
void
PaintTaskPoint(Canvas &canvas, const PixelRect &rc,
               const OrderedTask &task, const OrderedTaskPoint &point,
               const GeoPoint &location,
               const MapSettings &settings_map,
               const TaskLook &task_look,
               const AirspaceLook &airspace_look,
               const OverlayLook &overlay_look,
               const RasterTerrain *terrain, const Airspaces *airspaces,
               int highlight_index = -1);

#endif
