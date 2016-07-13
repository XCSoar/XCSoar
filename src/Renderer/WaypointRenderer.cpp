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
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/AbstractTask.hpp"
#include "Engine/Task/Unordered/UnorderedTaskPoint.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/ProtectedRoutePlanner.hpp"
#include "Screen/Canvas.hpp"
#include "Units/Units.hpp"
#include "Util/TruncateString.hpp"
#include "Util/StaticArray.hxx"
#include "Util/Macros.hpp"
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
  WaypointPtr waypoint;

  PixelPoint point;

  ReachResult reach;

  WaypointRenderer::Reachability reachable;

  bool in_task;

  void Set(const WaypointPtr &_waypoint, PixelPoint &_point,
           bool _in_task) {
    waypoint = _waypoint;
    point = _point;
    reach.Clear();
    reachable = WaypointRenderer::Invalid;
    in_task = _in_task;
  }

  bool IsReachable() const {
    return reachable == WaypointRenderer::ReachableStraight ||
      reachable == WaypointRenderer::ReachableTerrain;
  }

  void CalculateReachabilityDirect(const MoreData &basic,
                                   const SpeedVector &wind,
                                   const MacCready &mac_cready,
                                   const TaskBehaviour &task_behaviour) {
    assert(basic.location_available);
    assert(basic.NavAltitudeAvailable());

    const auto elevation = waypoint->elevation +
      task_behaviour.safety_height_arrival;
    const GlideState state(GeoVector(basic.location, waypoint->location),
                           elevation, basic.nav_altitude, wind);

    const GlideResult result = mac_cready.SolveStraight(state);
    if (!result.IsOk())
      return;

    reach.direct = result.pure_glide_altitude_difference;
    if (result.pure_glide_altitude_difference > 0)
      reachable = WaypointRenderer::ReachableTerrain;
  }

  bool CalculateRouteArrival(const RoutePlannerGlue &route_planner,
                             const TaskBehaviour &task_behaviour) {
    const double elevation = waypoint->elevation +
      task_behaviour.safety_height_arrival;
    const AGeoPoint p_dest (waypoint->location, elevation);
    if (!route_planner.FindPositiveArrival(p_dest, reach))
      return false;

    reach.Subtract(elevation);
    return true;
  }

  void CalculateReachability(const RoutePlannerGlue &route_planner,
                             const TaskBehaviour &task_behaviour)
  {
    if (!CalculateRouteArrival(route_planner, task_behaviour))
      return;

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

class WaypointVisitorMap final
  : public WaypointVisitor, public TaskPointConstVisitor
{
  const MapWindowProjection &projection;
  const WaypointRendererSettings &settings;
  const WaypointLook &look;
  const TaskBehaviour &task_behaviour;
  const MoreData &basic;

  TCHAR altitude_unit[4];
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
     task_valid(false),
     labels(projection.GetScreenWidth(), projection.GetScreenHeight())
  {
    _tcscpy(altitude_unit, Units::GetAltitudeName());
  }

protected:
  void FormatTitle(TCHAR *buffer, size_t buffer_size,
                   const Waypoint &way_point) const {
    buffer[0] = _T('\0');

    switch (settings.display_text_type) {
    case WaypointRendererSettings::DisplayTextType::NAME:
      CopyTruncateString(buffer, buffer_size, way_point.name.c_str());
      break;

    case WaypointRendererSettings::DisplayTextType::FIRST_FIVE:
      CopyTruncateString(buffer, buffer_size, way_point.name.c_str(), 5);
      break;

    case WaypointRendererSettings::DisplayTextType::FIRST_THREE:
      CopyTruncateString(buffer, buffer_size, way_point.name.c_str(), 3);
      break;

    case WaypointRendererSettings::DisplayTextType::NONE:
      buffer[0] = '\0';
      break;

    case WaypointRendererSettings::DisplayTextType::FIRST_WORD:
      CopyTruncateString(buffer, buffer_size, way_point.name.c_str());
      TCHAR *tmp;
      tmp = _tcsstr(buffer, _T(" "));
      if (tmp != nullptr)
        tmp[0] = '\0';
      break;

    case WaypointRendererSettings::DisplayTextType::OBSOLETE_DONT_USE_NUMBER:
    case WaypointRendererSettings::DisplayTextType::OBSOLETE_DONT_USE_NAMEIFINTASK:
      assert(false);
      gcc_unreachable();
    }
  }

  void FormatLabel(TCHAR *buffer, size_t buffer_size,
                   const Waypoint &way_point,
                   WaypointRenderer::Reachability reachable,
                   const ReachResult &reach) const {
    FormatTitle(buffer, buffer_size - 20, way_point);

    if (!way_point.IsLandable() && !way_point.flags.watched)
      return;

    if (settings.arrival_height_display == WaypointRendererSettings::ArrivalHeightDisplay::REQUIRED_GR) {
      if (!basic.location_available || !basic.NavAltitudeAvailable())
        return;

      const auto safety_height = task_behaviour.safety_height_arrival;
      const auto target_altitude = way_point.elevation + safety_height;
      const auto delta_h = basic.nav_altitude - target_altitude;
      if (delta_h <= 0)
        /* no L/D if below waypoint */
        return;

      const auto distance = basic.location.DistanceS(way_point.location);
      const auto gr = distance / delta_h;
      if (!GradientValid(gr))
        return;

      size_t length = _tcslen(buffer);
      if (length > 0)
        buffer[length++] = _T(':');
      StringFormatUnsafe(buffer + length, _T("%.1f"), (double) gr);
      return;
    }

    if (reachable == WaypointRenderer::Invalid)
      return;

    if (!reach.IsReachableDirect() && !way_point.flags.watched)
      return;

    if (settings.arrival_height_display == WaypointRendererSettings::ArrivalHeightDisplay::NONE)
      return;

    size_t length = _tcslen(buffer);
    int uah_glide = (int)Units::ToUserAltitude(reach.direct);
    int uah_terrain = (int)Units::ToUserAltitude(reach.terrain);

    if (settings.arrival_height_display == WaypointRendererSettings::ArrivalHeightDisplay::TERRAIN) {
      if (reach.IsReachableTerrain()) {
        if (length > 0)
          buffer[length++] = _T(':');
        StringFormatUnsafe(buffer + length, _T("%d%s"),
                           uah_terrain, altitude_unit);
      }
      return;
    }

    if (length > 0)
      buffer[length++] = _T(':');

    if (settings.arrival_height_display == WaypointRendererSettings::ArrivalHeightDisplay::GLIDE_AND_TERRAIN &&
        reach.IsReachableDirect() && reach.IsReachableTerrain() &&
        reach.IsDeltaConsiderable()) {
      StringFormatUnsafe(buffer + length, _T("%d/%d%s"), uah_glide,
                         uah_terrain, altitude_unit);
      return;
    }

    StringFormatUnsafe(buffer + length, _T("%d%s"), uah_glide, altitude_unit);
  }

  void DrawWaypoint(Canvas &canvas, const VisibleWaypoint &vwp) {
    const Waypoint &way_point = *vwp.waypoint;
    bool watchedWaypoint = way_point.flags.watched;

    vwp.DrawSymbol(settings, look, canvas,
                   projection.GetMapScale() > 4000,
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
    if (vwp.IsReachable() && way_point.IsLandable()) {
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

    TCHAR buffer[NAME_SIZE+1];
    FormatLabel(buffer, ARRAY_SIZE(buffer),
                way_point, vwp.reachable, vwp.reach);

    auto sc = vwp.point;
    if ((vwp.IsReachable() &&
         settings.landable_style == WaypointRendererSettings::LandableStyle::PURPLE_CIRCLE) ||
        settings.vector_landable_rendering)
      // make space for the green circle
      sc.x += 5;

    labels.Add(buffer, sc.x + 5, sc.y, text_mode, bold, vwp.reach.direct,
               vwp.in_task, way_point.IsLandable(), way_point.IsAirport(),
               watchedWaypoint);
  }

  void AddWaypoint(const WaypointPtr &way_point, bool in_task) {
    if (waypoints.full())
      return;

    if (!projection.WaypointInScaleFilter(*way_point) && !in_task)
      return;

    PixelPoint sc;
    if (!projection.GeoToScreenIfVisible(way_point->location, sc))
      return;

    VisibleWaypoint &vwp = waypoints.append();
    vwp.Set(way_point, sc, in_task);
  }

public:
  void Visit(const WaypointPtr &way_point) override {
    AddWaypoint(way_point, false);
  }

  void Visit(const TaskPoint &tp) override {
    switch (tp.GetType()) {
    case TaskPointType::UNORDERED:
      AddWaypoint(((const UnorderedTaskPoint &)tp).GetWaypointPtr(), true);
      break;

    case TaskPointType::START:
    case TaskPointType::AST:
    case TaskPointType::AAT:
    case TaskPointType::FINISH:
      AddWaypoint(((const OrderedTaskPoint &)tp).GetWaypointPtr(), true);
      break;
    }
  }

public:
  void SetTaskValid() {
    task_valid = true;
  }

  void CalculateRoute(const ProtectedRoutePlanner &route_planner) {
    const ProtectedRoutePlanner::Lease lease(route_planner);

    for (VisibleWaypoint &vwp : waypoints) {
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

    for (VisibleWaypoint &vwp : waypoints) {
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
    if (route_planner != nullptr && !route_planner->IsTerrainReachEmpty())
      CalculateRoute(*route_planner);
    else
      CalculateDirect(polar_settings, task_behaviour, calculated);
  }

  void Draw(Canvas &canvas) {
    for (const VisibleWaypoint &vwp : waypoints)
      DrawWaypoint(canvas, vwp);
  }
};

static void
MapWaypointLabelRender(Canvas &canvas, unsigned width, unsigned height,
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
  if (way_points == nullptr || way_points->IsEmpty())
    return;

  WaypointVisitorMap v(projection, settings, look, task_behaviour, basic);

  if (task != nullptr) {
    ProtectedTaskManager::Lease task_manager(*task);

    const TaskStats &task_stats = task_manager->GetStats();

    // task items come first, this is the only way we know that an item is in task,
    // and we won't add it if it is already there
    if (task_stats.task_valid)
      v.SetTaskValid();

    const AbstractTask *atask = task_manager->GetActiveTask();
    if (atask != nullptr)
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
