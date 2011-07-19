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

#include "WaypointRenderer.hpp"
#include "WaypointIconRenderer.hpp"
#include "MapWindow/MapWindowProjection.hpp"
#include "MapWindow/MapWindowLabels.hpp"
#include "SettingsMap.hpp"
#include "SettingsComputer.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Waypoint/WaypointVisitor.hpp"
#include "Engine/Navigation/Aircraft.hpp"
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
#include "Units/Units.hpp"
#include "Screen/Layout.hpp"
#include "Util/StaticArray.hpp"

#include <assert.h>
#include <stdio.h>

/**
 * Metadata for a Waypoint that is about to be drawn.
 */
struct VisibleWaypoint {
  const Waypoint *waypoint;

  RasterPoint point;

  short arrival_height_glide;
  short arrival_height_terrain;

  WaypointRenderer::Reachability reachable;

  bool in_task;

  void Set(const Waypoint &_waypoint, RasterPoint &_point,
           bool _in_task) {
    waypoint = &_waypoint;
    point = _point;
    arrival_height_glide = 0;
    arrival_height_terrain = 0;
    reachable = WaypointRenderer::Unreachable;
    in_task = _in_task;
  }

  void CalculateReachability(const ProtectedTaskManager &task,
                             const TaskBehaviour &task_behaviour)
  {
    const fixed elevation = waypoint->Altitude +
      task_behaviour.safety_height_arrival;
    const AGeoPoint p_dest (waypoint->Location, elevation);
    if (task.find_positive_arrival(p_dest, arrival_height_terrain, arrival_height_glide)) {
      const short h_base = iround(elevation);
      arrival_height_terrain -= h_base;
      arrival_height_glide -= h_base;
    }

    if (arrival_height_glide <= 0)
      reachable = WaypointRenderer::Unreachable;
    else if (task_behaviour.route_planner.reach_enabled() &&
             arrival_height_terrain <= 0)
      reachable = WaypointRenderer::ReachableStraight;
    else
      reachable = WaypointRenderer::ReachableTerrain;
  }

  void DrawSymbol(Canvas &canvas, const Projection &projection) const {
    WaypointIconRenderer wir(canvas, projection.GetMapScale() > fixed(4000),
                             projection.GetScreenAngle());
    wir.Draw(*waypoint, point, (WaypointIconRenderer::Reachability)reachable,
             in_task);
  }
};

class WaypointVisitorMap: 
  public WaypointVisitor, 
  public TaskPointConstVisitor
{
  const MapWindowProjection &projection;
  const SETTINGS_MAP &settings_map;
  const TaskBehaviour &task_behaviour;

  int pDisplayTextType;
  TCHAR sAltUnit[4];
  bool task_valid;

  /**
   * A list of waypoints that are going to be drawn.  This list is
   * filled in the Visitor methods.  In the second stage, their
   * reachability is calculated, and the third stage draws them.  This
   * should ensure that the drawing methods don't need to hold a
   * mutex.
   */
  StaticArray<VisibleWaypoint, 256> waypoints;

public:
  WaypointLabelList labels;

public:
  WaypointVisitorMap(const MapWindowProjection &_projection,
                     const SETTINGS_MAP &_settings_map,
                     const TaskBehaviour &_task_behaviour):
    projection(_projection),
    settings_map(_settings_map), task_behaviour(_task_behaviour),
    task_valid(false),
    labels(projection.GetScreenWidth(), projection.GetScreenHeight())
  {
    // if pan mode, show full names
    pDisplayTextType = settings_map.DisplayTextType;
    if (settings_map.EnablePan)
      pDisplayTextType = DISPLAYNAME;

    _tcscpy(sAltUnit, Units::GetAltitudeName());
  }

protected:
  void
  FormatTitle(TCHAR* Buffer, const Waypoint &way_point)
  {
    Buffer[0] = _T('\0');

    if (way_point.Name.length() >= NAME_SIZE - 20)
      return;

    switch (pDisplayTextType) {
    case DISPLAYNAME:
      _tcscpy(Buffer, way_point.Name.c_str());
      break;

    case DISPLAYFIRSTFIVE:
      CopyString(Buffer, way_point.Name.c_str(), 6);
      break;

    case DISPLAYFIRSTTHREE:
      CopyString(Buffer, way_point.Name.c_str(), 4);
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
  FormatLabel(TCHAR *buffer, const Waypoint &way_point,
              const int arrival_height_glide, const int arrival_height_terrain)
  {
    FormatTitle(buffer, way_point);

    if (!way_point.IsLandable() && !way_point.Flags.Watched)
      return;

    if (arrival_height_glide <= 0 && !way_point.Flags.Watched)
      return;

    if (settings_map.WaypointArrivalHeightDisplay == WP_ARRIVAL_HEIGHT_NONE)
      return;

    size_t length = _tcslen(buffer);
    int uah_glide = (int)Units::ToUserAltitude(fixed(arrival_height_glide));
    int uah_terrain = (int)Units::ToUserAltitude(fixed(arrival_height_terrain));

    if (settings_map.WaypointArrivalHeightDisplay == WP_ARRIVAL_HEIGHT_TERRAIN) {
      if (arrival_height_terrain > 0) {
        if (length > 0)
          buffer[length++] = _T(':');
        _stprintf(buffer + length, _T("%d%s"), uah_terrain, sAltUnit);
      }
      return;
    }

    if (length > 0)
      buffer[length++] = _T(':');

    if (settings_map.WaypointArrivalHeightDisplay == WP_ARRIVAL_HEIGHT_GLIDE_AND_TERRAIN &&
        arrival_height_glide > 0 && arrival_height_terrain > 0) {
      int altd = abs(arrival_height_glide - arrival_height_terrain);
      if (altd >= 10 && (altd * 100) / arrival_height_glide > 5) {
        _stprintf(buffer + length, _T("%d/%d%s"), uah_glide,
                  uah_terrain, sAltUnit);
        return;
      }
    }

    _stprintf(buffer + length, _T("%d%s"), uah_glide, sAltUnit);
  }

  void
  DrawWaypoint(Canvas &canvas, const VisibleWaypoint &vwp)
  {
    const Waypoint &way_point = *vwp.waypoint;
    bool watchedWaypoint = way_point.Flags.Watched;

    vwp.DrawSymbol(canvas, projection);

    // Determine whether to draw the waypoint label or not
    switch (settings_map.WaypointLabelSelection) {
    case wlsNoWaypoints:
      return;

    case wlsTaskWaypoints:
      if (!vwp.in_task && task_valid && !watchedWaypoint)
        return;
      break;

    case wlsTaskAndLandableWaypoints:
      if (!vwp.in_task && task_valid && !watchedWaypoint &&
          !way_point.IsLandable())
        return;
      break;

    default:
      break;
    }

    TextInBoxMode_t text_mode;
    if (vwp.reachable != WaypointRenderer::Unreachable &&
        way_point.IsLandable()) {
      text_mode.Mode = settings_map.LandableRenderMode;
      text_mode.Bold = true;
      text_mode.MoveInView = true;
    } else if (vwp.in_task) {
      text_mode.Mode = OutlinedInverted;
      text_mode.Bold = true;
    } else if (watchedWaypoint) {
      text_mode.Mode = Outlined;
      text_mode.Bold = false;
      text_mode.MoveInView = true;
    }

    TCHAR Buffer[NAME_SIZE+1];
    FormatLabel(Buffer, way_point,
                vwp.arrival_height_glide, vwp.arrival_height_terrain);

    RasterPoint sc = vwp.point;
    if ((vwp.reachable != WaypointRenderer::Unreachable &&
         Appearance.IndLandable == wpLandableWinPilot) ||
        Appearance.UseSWLandablesRendering)
      // make space for the green circle
      sc.x += 5;

    labels.Add(Buffer, sc.x + 5, sc.y, text_mode, vwp.arrival_height_glide,
               vwp.in_task, way_point.IsLandable(), way_point.IsAirport(),
               watchedWaypoint);
  }

  void AddWaypoint(const Waypoint &way_point, bool in_task) {
    if (waypoints.full())
      return;

    if (!projection.WaypointInScaleFilter(way_point) && !in_task)
      return;

    RasterPoint sc;
    if (!projection.GeoToScreenIfVisible(way_point.Location, sc))
      return;

    VisibleWaypoint &vwp = waypoints.append();
    vwp.Set(way_point, sc, in_task);
  }

public:
  void
  Visit(const Waypoint& way_point)
  {
    AddWaypoint(way_point, false);
  }

  void
  Visit(const UnorderedTaskPoint& tp)
  {
    AddWaypoint(tp.get_waypoint(), true);
  }

  void
  Visit(const StartPoint& tp)
  {
    AddWaypoint(tp.get_waypoint(), true);
  }

  void
  Visit(const FinishPoint& tp)
  {
    AddWaypoint(tp.get_waypoint(), true);
  }

  void
  Visit(const AATPoint& tp)
  {
    AddWaypoint(tp.get_waypoint(), true);
  }

  void
  Visit(const ASTPoint& tp)
  {
    AddWaypoint(tp.get_waypoint(), true);
  }


public:
  void set_task_valid() {
    task_valid = true;
  }

  void Calculate(const ProtectedTaskManager &task) {
    for (unsigned i = 0; i < waypoints.size(); ++i) {
      VisibleWaypoint &vwp = waypoints[i];
      const Waypoint &way_point = *vwp.waypoint;

      if (way_point.IsLandable() || way_point.Flags.Watched)
        vwp.CalculateReachability(task, task_behaviour);
    }
  }

  void Draw(Canvas &canvas) {
    for (unsigned i = 0; i < waypoints.size(); ++i)
      DrawWaypoint(canvas, waypoints[i]);
  }
};

static void
MapWaypointLabelRender(Canvas &canvas, unsigned width, unsigned height,
                       LabelBlock &label_block,
                       WaypointLabelList &labels)
{
  labels.Sort();

  for (unsigned i = 0; i < labels.size(); i++) {
    const WaypointLabelList::Label *E = &labels[i];
    TextInBox(canvas, E->Name, E->Pos.x, E->Pos.y, E->Mode,
              width, height, &label_block);
  }
}

void
WaypointRenderer::render(Canvas &canvas, LabelBlock &label_block,
                         const MapWindowProjection &projection,
                         const SETTINGS_MAP &settings_map,
                         const TaskBehaviour &task_behaviour,
                         const ProtectedTaskManager *task)
{
  if ((way_points == NULL) || way_points->empty())
    return;

  if (task) {

    canvas.set_text_color(COLOR_BLACK);

    WaypointVisitorMap v(projection, settings_map, task_behaviour);

    {
      ProtectedTaskManager::Lease task_manager(*task);

      // task items come first, this is the only way we know that an item is in task,
      // and we won't add it if it is already there
      if (task_manager->stats_valid()) {
        v.set_task_valid();
      }

      const AbstractTask *atask = task_manager->get_active_task();
      if (atask != NULL)
        atask->tp_CAccept(v);
    }

    way_points->visit_within_range(projection.GetGeoScreenCenter(),
                                   projection.GetScreenDistanceMeters(), v);

    v.Calculate(*task);
    v.Draw(canvas);

    MapWaypointLabelRender(canvas,
                           projection.GetScreenWidth(),
                           projection.GetScreenHeight(),
                           label_block, v.labels);
  }
}
