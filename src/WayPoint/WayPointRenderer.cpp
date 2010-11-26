/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "WayPointRenderer.hpp"
#include "MapWindowProjection.hpp"
#include "MapWindowLabels.hpp"
#include "SettingsMap.hpp"
#include "SettingsComputer.hpp"
#include "Task/Visitors/TaskVisitor.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Waypoint/WaypointVisitor.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/GlideSolvers/GlideResult.hpp"
#include "Engine/Task/Tasks/OrderedTask.hpp"
#include "Engine/Task/Tasks/AbortTask.hpp"
#include "Engine/Task/Tasks/GotoTask.hpp"
#include "Engine/Task/Tasks/BaseTask/UnorderedTaskPoint.hpp"
#include "Engine/Task/Tasks/TaskSolvers/TaskSolution.hpp"
#include "Engine/Task/TaskPoints/AATPoint.hpp"
#include "Engine/Task/TaskPoints/ASTPoint.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Screen/Icon.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Canvas.hpp"
#include "Appearance.hpp"
#include "Units.hpp"

#include <stdio.h>

class WaypointVisitorMap: 
  public WaypointVisitor, 
  public TaskPointConstVisitor,
  public TaskVisitor
{
  const MapWindowProjection &projection;
  const SETTINGS_MAP &settings_map;
  const TaskBehaviour &task_behaviour;

public:
  WaypointVisitorMap(const MapWindowProjection &_projection,
                     const SETTINGS_MAP &_settings_map,
                     const TaskBehaviour &_task_behaviour,
                     const AIRCRAFT_STATE &_aircraft_state, Canvas &_canvas,
                     const GlidePolar &polar):
    projection(_projection),
    settings_map(_settings_map), task_behaviour(_task_behaviour),
    aircraft_state(_aircraft_state),
    canvas(_canvas),
    glide_polar(polar),
    task_valid(false),
    labels(projection.GetMapRect())
  {
    // if pan mode, show full names
    pDisplayTextType = settings_map.DisplayTextType;
    if (settings_map.EnablePan)
      pDisplayTextType = DISPLAYNAME;

    _tcscpy(sAltUnit, Units::GetAltitudeName());
  }

  void
  FormatTitle(TCHAR* Buffer, const Waypoint &way_point, bool in_task)
  {
    Buffer[0] = _T('\0');

    switch (pDisplayTextType) {
    case DISPLAYNAMEIFINTASK:
    case DISPLAYNAME:
      _tcscpy(Buffer, way_point.Name.c_str());
      break;

    case DISPLAYNUMBER:
      _stprintf(Buffer, _T("%d"), way_point.id);
      break;

    case DISPLAYFIRSTFIVE:
      _tcsncpy(Buffer, way_point.Name.c_str(), 5);
      Buffer[5] = '\0';
      break;

    case DISPLAYFIRSTTHREE:
      _tcsncpy(Buffer, way_point.Name.c_str(), 3);
      Buffer[3] = '\0';
      break;

    case DISPLAYNONE:
      Buffer[0] = '\0';
      break;

    case DISPLAYUNTILSPACE:
      _tcscpy(Buffer, way_point.Name.c_str());
      TCHAR *tmp;
      tmp = _tcsstr(Buffer, _T(" "));
      if (tmp != NULL)
        tmp[0] = '\0';

      break;

    default:
      assert(0);
      break;
    }
  }

  void
  DrawWaypoint(const Waypoint& way_point, bool in_task = false)
  {
    RasterPoint sc;
    if (!projection.GeoToScreenIfVisible(way_point.Location, sc))
      return;

    if (!projection.WaypointInScaleFilter(way_point) && !in_task)
      return;

    TextInBoxMode_t text_mode;
    if (in_task)
      text_mode.Mode = Outlined;

    bool do_write_label = in_task || (settings_map.DeclutterLabels < 2);
    bool reachable = false;

    const MaskedIcon *icon = &Graphics::SmallIcon;

    int AltArrivalAGL = 0;

    if (way_point.is_landable()) {

      const UnorderedTaskPoint t(way_point, task_behaviour);
      const GlideResult r =
        TaskSolution::glide_solution_remaining(t, aircraft_state, glide_polar);

      if (r.glide_reachable()) {
        reachable = true;

        if ((settings_map.DeclutterLabels < 1) || in_task) {
          AltArrivalAGL = (int)Units::ToUserUnit(r.AltitudeDifference,
                                                 Units::AltitudeUnit);

          // show all reachable landing field altitudes unless we want a
          // decluttered screen.
        } 
        if ((settings_map.DeclutterLabels < 2) || in_task) {
          if (in_task || (settings_map.DeclutterLabels < 1))
            text_mode.Mode = RoundedBlack;

          // show all reachable landing field labels unless we want a
          // decluttered screen.
          do_write_label = true;
        }

        icon = way_point.Flags.Airport ? &Graphics::AirportReachableIcon :
                                         &Graphics::FieldReachableIcon;
      } else {
        icon = way_point.Flags.Airport ? &Graphics::AirportUnreachableIcon :
                                         &Graphics::FieldUnreachableIcon;
      }

    } else {
      // non landable turnpoint
      if (projection.GetMapScale() <= fixed(4000))
        icon = &Graphics::TurnPointIcon;
    }

    icon->draw(canvas, sc.x, sc.y);

    TCHAR Buffer[32];

    if (!in_task && (pDisplayTextType == DISPLAYNAMEIFINTASK)) {
      if (task_valid || !(way_point.is_landable() || (way_point.is_airport())))
        return;
    }

    if (do_write_label)
      FormatTitle(Buffer, way_point, in_task);
    else
      Buffer[0] = _T('\0');

    if (AltArrivalAGL != 0) {
      size_t length = _tcslen(Buffer);
      if (length > 0)
        Buffer[length++] = _T(':');

      _stprintf(Buffer + length, _T("%d%s"), AltArrivalAGL, sAltUnit);
    }

    if (reachable && Appearance.IndLandable == wpLandableDefault)
      // make space for the green circle
      sc.x += 5;

    labels.Add(Buffer, sc.x + 5, sc.y, text_mode, AltArrivalAGL,
               in_task, way_point.is_landable(), way_point.is_airport());
  }

  void
  Visit(const AbortTask& task)
  {
    task.tp_CAccept(*this);
  }

  void
  Visit(const OrderedTask& task)
  {
    task.tp_CAccept(*this);
  }

  void
  Visit(const GotoTask& task)
  {
    task.tp_CAccept(*this);
  }

  void
  Visit(const Waypoint& way_point)
  {
    DrawWaypoint(way_point, false);
  }

  void
  Visit(const UnorderedTaskPoint& tp)
  {
    DrawWaypoint(tp.get_waypoint(), true);
  }

  void
  Visit(const StartPoint& tp)
  {
    DrawWaypoint(tp.get_waypoint(), true);
  }

  void
  Visit(const FinishPoint& tp)
  {
    DrawWaypoint(tp.get_waypoint(), true);
  }

  void
  Visit(const AATPoint& tp)
  {
    DrawWaypoint(tp.get_waypoint(), true);
  }

  void
  Visit(const ASTPoint& tp)
  {
    DrawWaypoint(tp.get_waypoint(), true);
  }

private:
  const AIRCRAFT_STATE aircraft_state;
  Canvas &canvas;
  int pDisplayTextType;
  TCHAR sAltUnit[4];
  const GlidePolar glide_polar;
  bool task_valid;

public:
  void set_task_valid() {
    task_valid = true;
  }
  WayPointLabelList labels;
};

static void
MapWaypointLabelRender(Canvas &canvas, const RECT &MapRect,
                       LabelBlock &label_block,
                       const WayPointLabelList &labels)
{
  // first draw task waypoints
  for (unsigned i = 0; i < labels.size(); i++) {
    const WayPointLabelList::Label *E = &labels[i];
    // draws if they are in task unconditionally,
    // otherwise, does comparison
    if (E->inTask)
      TextInBox(canvas, E->Name, E->Pos.x, E->Pos.y, E->Mode, MapRect, &label_block);
  }

  // now draw airports in order of range (closest last)
  for (unsigned i = 0; i < labels.size(); i++) {
    const WayPointLabelList::Label *E = &labels[i];
    // draws if they are not in task,
    // otherwise, does comparison
    if (!E->inTask && E->isAirport)
      TextInBox(canvas, E->Name, E->Pos.x, E->Pos.y, E->Mode, MapRect, &label_block);
  }

  // now draw landable waypoints in order of range (closest last)
  for (unsigned i = 0; i < labels.size(); i++) {
    const WayPointLabelList::Label *E = &labels[i];
    // draws if they are not in task,
    // otherwise, does comparison
    if (!E->inTask && !E->isAirport && E->isLandable)
      TextInBox(canvas, E->Name, E->Pos.x, E->Pos.y, E->Mode, MapRect, &label_block);
  }

  // now draw normal waypoints in order of range (furthest away last)
  for (unsigned i = 0; i < labels.size(); i++) {
    const WayPointLabelList::Label *E = &labels[i];
    if (!E->inTask && !E->isAirport && !E->isLandable)
      TextInBox(canvas, E->Name, E->Pos.x, E->Pos.y, E->Mode, MapRect, &label_block);
  }
}

void
WayPointRenderer::render(Canvas &canvas, LabelBlock &label_block,
                         const MapWindowProjection &projection,
                         const SETTINGS_MAP &settings_map,
                         const TaskBehaviour &task_behaviour,
                         const GlidePolar &glide_polar,
                         const AIRCRAFT_STATE &aircraft_state,
                         const ProtectedTaskManager *task)
{
  if (way_points == NULL || way_points->empty())
    return;

  canvas.set_text_color(Color::BLACK);

  WaypointVisitorMap v(projection, settings_map, task_behaviour,
                       aircraft_state,
                       canvas, glide_polar);

  // task items come first, this is the only way we know that an item is in task,
  // and we won't add it if it is already there
  if (task != NULL) {
    ProtectedTaskManager::Lease task_manager(*task);
    if (task_manager->stats_valid()) {
      v.set_task_valid();
    }
    task_manager->CAccept(v);
  }

  way_points->visit_within_range(projection.GetGeoLocation(),
                                 projection.GetScreenDistanceMeters(), v);

  v.labels.Sort();
  MapWaypointLabelRender(canvas, projection.GetMapRect(),
                         label_block, v.labels);
}
