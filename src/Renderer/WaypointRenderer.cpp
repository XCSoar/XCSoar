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

#include "WaypointRenderer.hpp"
#include "WaypointRendererSettings.hpp"
#include "WaypointIconRenderer.hpp"
#include "WaypointLabelList.hpp"
#include "Projection/MapWindowProjection.hpp"
#include "Computer/Settings.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"
#include "Engine/Util/Gradient.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Engine/Waypoint/WaypointVisitor.hpp"
#include "Engine/GlideSolvers/GlideState.hpp"
#include "Engine/GlideSolvers/GlideResult.hpp"
#include "Engine/GlideSolvers/MacCready.hpp"
#include "Engine/Task/AbstractTask.hpp"
#include "Engine/Task/Unordered/UnorderedTaskPoint.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/ProtectedRoutePlanner.hpp"
#include "Screen/Canvas.hpp"
#include "Units/Units.hpp"
#include "Util/StaticArray.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Engine/Route/ReachResult.hpp"
#include "Look/WaypointLook.hpp"

#include <assert.h>
#include <stdio.h>

/**
 * Metadata for a Waypoint that is about to be drawn.
 */
struct VisibleWaypoint {
  const Waypoint *waypoint;

  RasterPoint point;

  ReachResult reach;

  WaypointRenderer::Reachability reachable;

  bool in_task;

  void Set(const Waypoint &_waypoint, RasterPoint &_point,
           bool _in_task) {
    waypoint = &_waypoint;
    point = _point;
    reach.Clear();
    reachable = WaypointRenderer::Unreachable;
    in_task = _in_task;
  }

  void CalculateReachabilityDirect(const MoreData &basic,
                                   const SpeedVector &wind,
                                   const MacCready &mac_cready,
                                   const TaskBehaviour &task_behaviour) {
    assert(basic.location_available);
    assert(basic.NavAltitudeAvailable());

    const fixed elevation = waypoint->elevation +
      task_behaviour.safety_height_arrival;
    const GlideState state(GeoVector(basic.location, waypoint->location),
                           elevation, basic.nav_altitude, wind);

    const GlideResult result = mac_cready.SolveStraight(state);
    if (!result.IsOk())
      return;

    reach.direct = result.pure_glide_altitude_difference;
    if (positive(result.pure_glide_altitude_difference))
      reachable = WaypointRenderer::ReachableTerrain;
  }

  void CalculateRouteArrival(const RoutePlannerGlue &route_planner,
                             const TaskBehaviour &task_behaviour) {
    const RoughAltitude elevation(waypoint->elevation +
                                  task_behaviour.safety_height_arrival);
    const AGeoPoint p_dest (waypoint->location, elevation);
    if (route_planner.FindPositiveArrival(p_dest, reach))
      reach.Subtract(elevation);
  }

  void CalculateReachability(const RoutePlannerGlue &route_planner,
                             const TaskBehaviour &task_behaviour)
  {
    CalculateRouteArrival(route_planner, task_behaviour);

    if (!reach.IsReachableDirect())
      reachable = WaypointRenderer::Unreachable;
    else if (task_behaviour.route_planner.IsReachEnabled() &&
             !reach.IsReachableTerrain())
      reachable = WaypointRenderer::ReachableStraight;
    else
      reachable = WaypointRenderer::ReachableTerrain;
  }

  void DrawSymbol(const struct WaypointRendererSettings &settings,
                  const WaypointLook &look,
                  Canvas &canvas, bool small_icons, Angle screen_rotation) const {
    WaypointIconRenderer wir(settings, look,
                             canvas, small_icons, screen_rotation);
    wir.Draw(*waypoint, point, (WaypointIconRenderer::Reachability)reachable,
             in_task);
  }
};

class WaypointVisitorMap: 
  public WaypointVisitor, 
  public TaskPointConstVisitor
{
  const MapWindowProjection &projection;
  const WaypointRendererSettings &settings;
  const WaypointLook &look;
  const TaskBehaviour &task_behaviour;
  const MoreData &basic;
  /**
   * is the ordered task a MAT
   */
  bool is_mat;

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
                     const WaypointRendererSettings &_settings,
                     const WaypointLook &_look,
                     const TaskBehaviour &_task_behaviour,
                     const MoreData &_basic)
    :projection(_projection),
     settings(_settings), look(_look), task_behaviour(_task_behaviour),
     basic(_basic),
     is_mat(false),
     task_valid(false),
     labels(projection.GetScreenWidth(), projection.GetScreenHeight())
  {
    _tcscpy(sAltUnit, Units::GetAltitudeName());
  }

  /**
   * Indicate the ordered task is a MAT
   */
  void SetIsMat(bool v) {
    is_mat = v;
  }

protected:
  void
  FormatTitle(TCHAR* Buffer, const Waypoint &way_point)
  {
    Buffer[0] = _T('\0');

    if (way_point.name.length() >= NAME_SIZE - 20)
      return;

    switch (settings.display_text_type) {
    case WaypointRendererSettings::DisplayTextType::NAME:
      _tcscpy(Buffer, way_point.name.c_str());
      break;

    case WaypointRendererSettings::DisplayTextType::FIRST_FIVE:
      CopyString(Buffer, way_point.name.c_str(), 6);
      break;

    case WaypointRendererSettings::DisplayTextType::FIRST_THREE:
      CopyString(Buffer, way_point.name.c_str(), 4);
      break;

    case WaypointRendererSettings::DisplayTextType::NONE:
      Buffer[0] = '\0';
      break;

    case WaypointRendererSettings::DisplayTextType::FIRST_WORD:
      _tcscpy(Buffer, way_point.name.c_str());
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
              const ReachResult &reach)
  {
    FormatTitle(buffer, way_point);

    if (!way_point.IsLandable() && !way_point.flags.watched)
      return;

    if (settings.arrival_height_display == WaypointRendererSettings::ArrivalHeightDisplay::REQUIRED_GR) {
      if (!basic.location_available || !basic.NavAltitudeAvailable())
        return;

      const fixed safety_height = task_behaviour.safety_height_arrival;
      const fixed target_altitude = way_point.elevation + safety_height;
      const fixed delta_h = basic.nav_altitude - target_altitude;
      if (!positive(delta_h))
        /* no L/D if below waypoint */
        return;

      const fixed distance = basic.location.Distance(way_point.location);
      const fixed gr = distance / delta_h;
      if (!GradientValid(gr))
        return;

      size_t length = _tcslen(buffer);
      if (length > 0)
        buffer[length++] = _T(':');
      _stprintf(buffer + length, _T("%.1f"), (double) gr);
      return;
    }

    if (!reach.IsReachableDirect() && !way_point.flags.watched)
      return;

    if (settings.arrival_height_display == WaypointRendererSettings::ArrivalHeightDisplay::NONE)
      return;

    size_t length = _tcslen(buffer);
    int uah_glide = (int)Units::ToUserAltitude(fixed(reach.direct));
    int uah_terrain = (int)Units::ToUserAltitude(fixed(reach.terrain));

    if (settings.arrival_height_display == WaypointRendererSettings::ArrivalHeightDisplay::TERRAIN) {
      if (reach.IsReachableTerrain()) {
        if (length > 0)
          buffer[length++] = _T(':');
        _stprintf(buffer + length, _T("%d%s"), uah_terrain, sAltUnit);
      }
      return;
    }

    if (length > 0)
      buffer[length++] = _T(':');

    if (settings.arrival_height_display == WaypointRendererSettings::ArrivalHeightDisplay::GLIDE_AND_TERRAIN &&
        reach.IsReachableDirect() && reach.IsReachableTerrain() &&
        reach.IsDeltaConsiderable()) {
      _stprintf(buffer + length, _T("%d/%d%s"), uah_glide,
                uah_terrain, sAltUnit);
      return;
    }

    _stprintf(buffer + length, _T("%d%s"), uah_glide, sAltUnit);
  }

  void
  DrawWaypoint(Canvas &canvas, const VisibleWaypoint &vwp)
  {
    const Waypoint &way_point = *vwp.waypoint;
    bool watchedWaypoint = way_point.flags.watched;

    vwp.DrawSymbol(settings, look, canvas,
                   projection.GetMapScale() > fixed(4000),
                   projection.GetScreenAngle());

    // Determine whether to draw the waypoint label or not
    switch (settings.label_selection) {
    case WaypointRendererSettings::LabelSelection::NONE:
      return;

    case WaypointRendererSettings::LabelSelection::TASK:
      if (!vwp.in_task && task_valid && !watchedWaypoint)
        return;
      break;

    case WaypointRendererSettings::LabelSelection::TASK_AND_AIRFIELD:
      if (!vwp.in_task && task_valid && !watchedWaypoint &&
          !way_point.IsAirport())
        return;
      break;

    case WaypointRendererSettings::LabelSelection::TASK_AND_LANDABLE:
      if (!vwp.in_task && task_valid && !watchedWaypoint &&
          !way_point.IsLandable())
        return;
      break;

    default:
      break;
    }

    TextInBoxMode text_mode;
    bool bold = false;
    if (vwp.reachable != WaypointRenderer::Unreachable &&
        way_point.IsLandable()) {
      text_mode.shape = settings.landable_render_mode;
      bold = true;
      text_mode.move_in_view = true;
    } else if (vwp.in_task) {
      text_mode.shape = LabelShape::OUTLINED_INVERTED;
      bold = true;
    } else if (watchedWaypoint) {
      text_mode.shape = LabelShape::OUTLINED;
      text_mode.move_in_view = true;
    }

    TCHAR Buffer[NAME_SIZE+1];
    FormatLabel(Buffer, way_point, vwp.reach);

    RasterPoint sc = vwp.point;
    if ((vwp.reachable != WaypointRenderer::Unreachable &&
         settings.landable_style == WaypointRendererSettings::LandableStyle::PURPLE_CIRCLE) ||
        settings.vector_landable_rendering)
      // make space for the green circle
      sc.x += 5;

    labels.Add(Buffer, sc.x + 5, sc.y, text_mode, bold, vwp.reach.direct,
               vwp.in_task, way_point.IsLandable(), way_point.IsAirport(),
               watchedWaypoint);
  }

  void AddWaypoint(const Waypoint &way_point, bool in_task) {
    if (waypoints.full())
      return;

    if (!projection.WaypointInScaleFilter(way_point) && !in_task)
      return;

    RasterPoint sc;
    if (!projection.GeoToScreenIfVisible(way_point.location, sc))
      return;

    VisibleWaypoint &vwp = waypoints.append();
    vwp.Set(way_point, sc, in_task);
  }

public:
  void
  Visit(const Waypoint& way_point)
  {
    AddWaypoint(way_point, way_point.IsTurnpoint() && is_mat);
  }

  virtual void Visit(const TaskPoint &tp) override {
    switch (tp.GetType()) {
    case TaskPointType::UNORDERED:
      AddWaypoint(((const UnorderedTaskPoint &)tp).GetWaypoint(), true);
      break;

    case TaskPointType::START:
    case TaskPointType::AST:
    case TaskPointType::AAT:
    case TaskPointType::FINISH:
      AddWaypoint(((const OrderedTaskPoint &)tp).GetWaypoint(), true);
      break;
    }
  }

public:
  void set_task_valid() {
    task_valid = true;
  }

  void CalculateRoute(const ProtectedRoutePlanner &route_planner) {
    const ProtectedRoutePlanner::Lease lease(route_planner);

    for (auto it = waypoints.begin(), end = waypoints.end(); it != end; ++it) {
      VisibleWaypoint &vwp = *it;
      const Waypoint &way_point = *vwp.waypoint;

      if (way_point.IsLandable() || way_point.flags.watched)
        vwp.CalculateReachability(lease, task_behaviour);
    }
  }

  void CalculateDirect(const PolarSettings &polar_settings,
                       const TaskBehaviour &task_behaviour,
                       const DerivedInfo &calculated) {
    if (!basic.location_available || !basic.NavAltitudeAvailable())
      return;

    const GlidePolar &glide_polar =
      task_behaviour.route_planner.reach_polar_mode == RoutePlannerConfig::Polar::TASK
      ? polar_settings.glide_polar_task
      : calculated.glide_polar_safety;
    const MacCready mac_cready(task_behaviour.glide, glide_polar);

    for (auto it = waypoints.begin(), end = waypoints.end(); it != end; ++it) {
      VisibleWaypoint &vwp = *it;
      const Waypoint &way_point = *vwp.waypoint;

      if (way_point.IsLandable() || way_point.flags.watched)
        vwp.CalculateReachabilityDirect(basic, calculated.GetWindOrZero(),
                                        mac_cready, task_behaviour);
    }
  }

  void Calculate(const ProtectedRoutePlanner *route_planner,
                 const PolarSettings &polar_settings,
                 const TaskBehaviour &task_behaviour,
                 const DerivedInfo &calculated) {
    if (route_planner != NULL && !route_planner->IsReachEmpty())
      CalculateRoute(*route_planner);
    else
      CalculateDirect(polar_settings, task_behaviour, calculated);
  }

  void Draw(Canvas &canvas) {
    for (auto it = waypoints.begin(), end = waypoints.end(); it != end; ++it)
      DrawWaypoint(canvas, *it);
  }
};

static void
MapWaypointLabelRender(Canvas &canvas, UPixelScalar width, UPixelScalar height,
                       LabelBlock &label_block,
                       WaypointLabelList &labels,
                       const WaypointLook &look)
{
  labels.Sort();

  for (const auto &l : labels) {
    canvas.Select(l.bold ? *look.bold_font : *look.font);

    TextInBox(canvas, l.Name, l.Pos.x, l.Pos.y, l.Mode,
              width, height, &label_block);
  }
}

void
WaypointRenderer::render(Canvas &canvas, LabelBlock &label_block,
                         const MapWindowProjection &projection,
                         const struct WaypointRendererSettings &settings,
                         const PolarSettings &polar_settings,
                         const TaskBehaviour &task_behaviour,
                         const MoreData &basic, const DerivedInfo &calculated,
                         const ProtectedTaskManager *task,
                         const ProtectedRoutePlanner *route_planner)
{
  if ((way_points == NULL) || way_points->IsEmpty())
    return;

  WaypointVisitorMap v(projection, settings, look, task_behaviour, basic);

  if (task != NULL) {
    ProtectedTaskManager::Lease task_manager(*task);

    const TaskStats &task_stats = task_manager->GetStats();

    v.SetIsMat(task_stats.is_mat);

    // task items come first, this is the only way we know that an item is in task,
    // and we won't add it if it is already there
    if (task_stats.task_valid)
      v.set_task_valid();

    const AbstractTask *atask = task_manager->GetActiveTask();
    if (atask != NULL)
      atask->AcceptTaskPointVisitor(v);
  }

  way_points->VisitWithinRange(projection.GetGeoScreenCenter(),
                                 projection.GetScreenDistanceMeters(), v);

  v.Calculate(route_planner, polar_settings, task_behaviour, calculated);

  v.Draw(canvas);

  MapWaypointLabelRender(canvas,
                         projection.GetScreenWidth(),
                         projection.GetScreenHeight(),
                         label_block, v.labels, look);
}
