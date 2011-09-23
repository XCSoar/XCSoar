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

#include "Gauge/TaskView.hpp"
#include "Screen/Canvas.hpp"
#include "ChartProjection.hpp"
#include "BackgroundDrawHelper.hpp"
#include "Renderer/RenderObservationZone.hpp"
#include "Renderer/RenderTaskPoint.hpp"
#include "Renderer/RenderTask.hpp"
#include "Engine/Task/Tasks/BaseTask/OrderedTaskPoint.hpp"
#include "Engine/Task/Tasks/OrderedTask.hpp"
#include "SettingsMap.hpp"

static void
PaintTask(Canvas &canvas, const WindowProjection &projection,
          const OrderedTask &task,
          const GeoPoint &location, const SETTINGS_MAP &settings_map,
          const TaskLook &task_look,
          const AirspaceLook &airspace_look,
          const RasterTerrain *terrain)
{
  BackgroundDrawHelper background;
  background.set_terrain(terrain);
  background.Draw(canvas, projection, settings_map.terrain);

  RenderObservationZone ozv(task_look, airspace_look, settings_map.airspace);
  RenderTaskPoint tpv(canvas, NULL, projection, settings_map, task_look,
                      task.get_task_projection(),
                      ozv, false, false, location);
  RenderTask dv(tpv, projection.GetScreenBounds());
  dv.Visit(task);
}

void
PaintTask(Canvas &canvas, const PixelRect &rc, const OrderedTask &task,
          const GeoPoint &location, const SETTINGS_MAP &settings_map,
          const TaskLook &task_look,
          const AirspaceLook &airspace_look,
          const RasterTerrain *terrain)
{
  ChartProjection projection(rc, task, location);
  PaintTask(canvas, projection, task, location, settings_map,
            task_look, airspace_look, terrain);
}

void
PaintTaskPoint(Canvas &canvas, const PixelRect &rc,
               const OrderedTask &task, const OrderedTaskPoint &point,
               const GeoPoint &location, const SETTINGS_MAP &settings_map,
               const TaskLook &task_look,
               const AirspaceLook &airspace_look,
               const RasterTerrain *terrain)
{
  ChartProjection projection(rc, point, point.GetLocation());
  PaintTask(canvas, projection, task, location, settings_map,
            task_look, airspace_look, terrain);
}
