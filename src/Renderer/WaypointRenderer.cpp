// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointRenderer.hpp"
#include "Renderer/MapWaypointDrawLimits.hpp"
#include "Renderer/TextInBox.hpp"
#include "WaypointRendererSettings.hpp"
#include "WaypointIconRenderer.hpp"
#include "WaypointLabelList.hpp"
#include "Projection/MapWindowProjection.hpp"
#include "Computer/Settings.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"
#include "Engine/Util/Gradient.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Engine/GlideSolvers/GlideState.hpp"
#include "Engine/GlideSolvers/GlideResult.hpp"
#include "Engine/GlideSolvers/MacCready.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/AbstractTask.hpp"
#include "Engine/Task/Unordered/UnorderedTaskPoint.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/ProtectedRoutePlanner.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Units/Units.hpp"
#include "util/TruncateString.hpp"
#include "util/StaticArray.hxx"
#include "util/Macros.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Engine/Route/ReachResult.hpp"
#include "Look/WaypointLook.hpp"

#include <cassert>
#include <climits>
#include <cstdio>

/**
 * Metadata for a Waypoint that is about to be drawn.
 */
struct VisibleWaypoint {
  WaypointPtr waypoint;

  PixelPoint point;

  ReachResult reach;

  WaypointReachability reachable;

  bool in_task;

  void Set(const WaypointPtr &_waypoint, PixelPoint &_point,
           bool _in_task) noexcept {
    waypoint = _waypoint;
    point = _point;
    reach.Clear();
    reachable = WaypointReachability::INVALID;
    in_task = _in_task;
  }

  bool IsReachable() const noexcept {
    return ::IsReachable(reachable);
  }

  void CalculateReachabilityDirect(const MoreData &basic,
                                   const SpeedVector &wind,
                                   const MacCready &mac_cready,
                                   const TaskBehaviour &task_behaviour) noexcept {
    assert(basic.location_available);
    assert(basic.NavAltitudeAvailable());

    if (!waypoint->has_elevation)
      return;

    const auto elevation = waypoint->elevation +
      task_behaviour.safety_height_arrival;
    const GlideState state(GeoVector(basic.location, waypoint->location),
                           elevation, basic.nav_altitude, wind);

    const GlideResult result = mac_cready.SolveStraight(state);
    if (!result.IsOk())
      return;

    reach.direct = result.pure_glide_altitude_difference;
    if (result.pure_glide_altitude_difference > 0)
      reachable = WaypointReachability::TERRAIN;
    else
      reachable = WaypointReachability::UNREACHABLE;
  }

  bool CalculateRouteArrival(const ProtectedRoutePlanner &route_planner,
                             const TaskBehaviour &task_behaviour) noexcept {
    if (!waypoint->has_elevation)
      return false;

    const double elevation = waypoint->elevation +
      task_behaviour.safety_height_arrival;
    const AGeoPoint p_dest (waypoint->location, elevation);

    auto _reach = route_planner.FindPositiveArrival(p_dest);
    if (!_reach)
      return false;

    reach = *_reach;
    reach.Subtract(elevation);
    return true;
  }

  void CalculateReachability(const ProtectedRoutePlanner &route_planner,
                             const TaskBehaviour &task_behaviour) noexcept
  {
    if (!CalculateRouteArrival(route_planner, task_behaviour))
      return;

    if (!reach.IsReachableDirect())
      reachable = WaypointReachability::UNREACHABLE;
    else if (task_behaviour.route_planner.IsReachEnabled() &&
             !reach.IsReachableTerrain())
      reachable = WaypointReachability::STRAIGHT;
    else
      reachable = WaypointReachability::TERRAIN;
  }

  void DrawSymbol(WaypointIconRenderer &wir) const noexcept {
    wir.Draw(*waypoint, point, reachable,
             in_task);
  }
};

static void
FormatWaypointMapTitle(char *buffer, size_t buffer_size,
                       const WaypointRendererSettings &settings,
                       const Waypoint &way_point) noexcept
{
  buffer[0] = '\0';

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

  case WaypointRendererSettings::DisplayTextType::FIRST_WORD: {
    CopyTruncateString(buffer, buffer_size, way_point.name.c_str());
    char *tmp = strstr(buffer, " ");
    if (tmp != nullptr)
      tmp[0] = '\0';
    break;
  }

  case WaypointRendererSettings::DisplayTextType::SHORT_NAME:
    if (!way_point.shortname.empty())
      CopyTruncateString(buffer, buffer_size, way_point.shortname.c_str());
    else
      CopyTruncateString(buffer, buffer_size, way_point.name.c_str(), 5);
    break;

  case WaypointRendererSettings::DisplayTextType::OBSOLETE_DONT_USE_NUMBER:
  case WaypointRendererSettings::DisplayTextType::OBSOLETE_DONT_USE_NAMEIFINTASK:
    assert(false);
    gcc_unreachable();
  }
}

static void
FormatWaypointMapLabel(char *buffer, size_t buffer_size,
                       const WaypointRendererSettings &settings,
                       const TaskBehaviour &task_behaviour,
                       const MoreData &basic,
                       const char altitude_unit[4],
                       const Waypoint &way_point,
                       WaypointReachability reachable,
                       const ReachResult &reach) noexcept
{
  FormatWaypointMapTitle(buffer, buffer_size - 20, settings, way_point);

  if (!way_point.IsLandable() && !way_point.flags.watched)
    return;

  if (settings.arrival_height_display ==
          WaypointRendererSettings::ArrivalHeightDisplay::REQUIRED_GR ||
      settings.arrival_height_display ==
          WaypointRendererSettings::ArrivalHeightDisplay::REQUIRED_GR_AND_TERRAIN) {
    if (!basic.location_available || !basic.NavAltitudeAvailable() ||
        !way_point.has_elevation)
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

    size_t length = strlen(buffer);
    if (length > 0)
      buffer[length++] = ':';

    if (settings.arrival_height_display ==
            WaypointRendererSettings::ArrivalHeightDisplay::
                REQUIRED_GR_AND_TERRAIN &&
        reach.IsReachableTerrain()) {
      int uah_terrain = (int)Units::ToUserAltitude(reach.terrain);
      StringFormatUnsafe(buffer + length, "%.1f/%d%s", (double)gr,
                         uah_terrain, altitude_unit);
      return;
    }

    StringFormatUnsafe(buffer + length, "%.1f", (double)gr);
    return;
  }

  if (reachable == WaypointReachability::INVALID)
    return;

  if (!reach.IsReachableDirect() && !way_point.flags.watched)
    return;

  if (settings.arrival_height_display ==
      WaypointRendererSettings::ArrivalHeightDisplay::NONE)
    return;

  size_t length = strlen(buffer);
  int uah_glide = (int)Units::ToUserAltitude(reach.direct);
  int uah_terrain = (int)Units::ToUserAltitude(reach.terrain);

  if (settings.arrival_height_display ==
      WaypointRendererSettings::ArrivalHeightDisplay::TERRAIN) {
    if (reach.IsReachableTerrain()) {
      if (length > 0)
        buffer[length++] = ':';
      StringFormatUnsafe(buffer + length, "%d%s",
                         uah_terrain, altitude_unit);
    }
    return;
  }

  if (length > 0)
    buffer[length++] = ':';

  if (settings.arrival_height_display ==
          WaypointRendererSettings::ArrivalHeightDisplay::GLIDE_AND_TERRAIN &&
      reach.IsReachableDirect() && reach.IsReachableTerrain() &&
      reach.IsDeltaConsiderable()) {
    StringFormatUnsafe(buffer + length, "%d/%d%s", uah_glide,
                       uah_terrain, altitude_unit);
    return;
  }

  StringFormatUnsafe(buffer + length, "%d%s", uah_glide, altitude_unit);
}

static void
WaypointMapLabelStyle(TextInBoxMode &text_mode, bool &bold,
                      const WaypointRendererSettings &settings,
                      const VisibleWaypoint &vwp,
                      const Waypoint &way_point) noexcept
{
  text_mode = TextInBoxMode{};
  bold = false;
  const bool watched_waypoint = way_point.flags.watched;
  if (vwp.IsReachable() && way_point.IsLandable()) {
    text_mode.shape = settings.landable_render_mode;
    bold = true;
    text_mode.move_in_view = true;
  } else if (vwp.in_task) {
    text_mode.shape = LabelShape::OUTLINED_INVERTED;
    bold = true;
  } else if (watched_waypoint) {
    text_mode.shape = LabelShape::OUTLINED;
    text_mode.move_in_view = true;
  }
}

static PixelPoint
WaypointMapLabelScreenPosition(const VisibleWaypoint &vwp,
                               const WaypointRendererSettings &settings) noexcept
{
  PixelPoint sc = vwp.point;
  sc.x += 5;
  if ((vwp.IsReachable() &&
       settings.landable_style ==
         WaypointRendererSettings::LandableStyle::PURPLE_CIRCLE) ||
      settings.vector_landable_rendering)
    sc.x += 5;
  return sc;
}

static void
AppendWaypointMapLabelFromVisible(WaypointLabelList &labels,
                                  const VisibleWaypoint &vwp,
                                  const WaypointRendererSettings &settings,
                                  const char *buffer) noexcept
{
  const Waypoint &way_point = *vwp.waypoint;
  TextInBoxMode text_mode;
  bool bold;
  WaypointMapLabelStyle(text_mode, bold, settings, vwp, way_point);
  const PixelPoint sc = WaypointMapLabelScreenPosition(vwp, settings);
  labels.Add(buffer, sc, text_mode, bold,
               vwp.reachable != WaypointReachability::INVALID ? vwp.reach.direct
                                                             : INT_MIN,
               vwp.in_task, way_point.IsLandable(), way_point.IsAirport(),
               way_point.flags.watched);
}

class WaypointVisitorMap final
  : public TaskPointConstVisitor
{
  const MapWindowProjection &projection;
  const WaypointRendererSettings &settings;
  const WaypointLook &look;
  const TaskBehaviour &task_behaviour;
  const MoreData &basic;

  char altitude_unit[4];
  bool task_valid;

  /**
   * A list of waypoints that are going to be drawn.  This list is
   * filled in the Visitor methods.  In the second stage, their
   * reachability is calculated, and the third stage draws them.  This
   * should ensure that the drawing methods don't need to hold a
   * mutex.
   */
  StaticArray<VisibleWaypoint, MAX_MAP_WAYPOINT_DRAW> waypoints;

  WaypointIconRenderer icon_renderer;

public:
  WaypointLabelList labels;

public:
  WaypointVisitorMap(Canvas &_canvas,
                     const MapWindowProjection &_projection,
                     const WaypointRendererSettings &_settings,
                     const WaypointLook &_look,
                     const TaskBehaviour &_task_behaviour,
                     const MoreData &_basic) noexcept
    :projection(_projection),
     settings(_settings), look(_look), task_behaviour(_task_behaviour),
     basic(_basic),
     task_valid(false),
     icon_renderer(settings, look,
                   _canvas,
                   projection.GetMapScale() > 4000,
                   projection.GetScreenAngle()),
     labels(projection.GetScreenRect())
  {
    strcpy(altitude_unit, Units::GetAltitudeName());
  }


protected:
  void FormatLabel(char *buffer, size_t buffer_size,
                   const Waypoint &way_point,
                   WaypointReachability reachable,
                   const ReachResult &reach) const noexcept {
    FormatWaypointMapLabel(buffer, buffer_size, settings, task_behaviour, basic,
                           altitude_unit, way_point, reachable, reach);
  }

  void DrawWaypoint(const VisibleWaypoint &vwp) noexcept {
    const Waypoint &way_point = *vwp.waypoint;
    bool watchedWaypoint = way_point.flags.watched;

    vwp.DrawSymbol(icon_renderer);

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

    char buffer[NAME_SIZE + 1];
    FormatLabel(buffer, ARRAY_SIZE(buffer),
                way_point, vwp.reachable, vwp.reach);

    AppendWaypointMapLabelFromVisible(labels, vwp, settings, buffer);
  }

  void AddWaypoint(const WaypointPtr &way_point, bool in_task) noexcept {
    if (waypoints.full())
      return;

    if (!projection.WaypointInScaleFilter(*way_point) && !in_task)
      return;

    if (auto p = projection.GeoToScreenIfVisible(way_point->location)) {
      VisibleWaypoint &vwp = waypoints.append();
      vwp.Set(way_point, *p, in_task);
    }
  }

public:
  void Add(const WaypointPtr &way_point) noexcept {
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
  void SetTaskValid() noexcept {
    task_valid = true;
  }

  void CalculateRoute(const ProtectedRoutePlanner &route_planner) noexcept {
    for (VisibleWaypoint &vwp : waypoints) {
      const Waypoint &way_point = *vwp.waypoint;

      if (way_point.IsLandable() || way_point.flags.watched)
        vwp.CalculateReachability(route_planner, task_behaviour);
    }
  }

  void CalculateDirect(const PolarSettings &polar_settings,
                       const TaskBehaviour &task_behaviour,
                       const DerivedInfo &calculated) noexcept {
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
                 const DerivedInfo &calculated) noexcept {
    if (route_planner != nullptr && !route_planner->IsTerrainReachEmpty())
      CalculateRoute(*route_planner);
    else
      CalculateDirect(polar_settings, task_behaviour, calculated);
  }

  void Draw() noexcept {
    for (const VisibleWaypoint &vwp : waypoints)
      DrawWaypoint(vwp);
  }
};

void
RenderWaypointLabelList(Canvas &canvas, PixelSize clip_size,
                        LabelBlock &label_block,
                        WaypointLabelList &labels,
                        const WaypointLook &look) noexcept
{
  labels.Sort();

  for (const auto &l : labels) {
    canvas.Select(l.bold ? *look.bold_font : *look.font);

    TextInBox(canvas, l.Name, l.Pos, l.Mode, clip_size, &label_block);
  }
}

void
RenderOrderedTaskWaypointLabels(Canvas &canvas,
                                LabelBlock &label_block,
                                const MapWindowProjection &projection,
                                const WaypointRendererSettings &settings,
                                const WaypointLook &look,
                                const OrderedTask &task) noexcept
{
  if (look.font == nullptr || look.bold_font == nullptr)
    return;

  WaypointLabelList labels(projection.GetScreenRect());

  TextInBoxMode text_mode{};
  text_mode.shape = LabelShape::OUTLINED_INVERTED;

  for (unsigned i = 0; i < task.TaskSize(); ++i) {
    const OrderedTaskPoint &tp = task.GetTaskPoint(i);
    const WaypointPtr wp = tp.GetWaypointPtr();
    if (!wp)
      continue;

    const auto sc_opt =
      projection.GeoToScreenIfVisible(tp.GetLocationRemaining());
    if (!sc_opt)
      continue;

    PixelPoint sc = *sc_opt;
    sc.x += 5;

    char buffer[NAME_SIZE + 1];
    FormatWaypointMapTitle(buffer, ARRAY_SIZE(buffer), settings, *wp);
    if (buffer[0] == '\0')
      continue;

    labels.Add(buffer, sc, text_mode, true, INT_MIN,
               true, wp->IsLandable(), wp->IsAirport(),
               wp->flags.watched);
  }

  RenderWaypointLabelList(canvas, projection.GetScreenSize(),
                          label_block, labels, look);
}

void
RenderWaypointForMapPreview(Canvas &canvas, LabelBlock &label_block,
                            const MapWindowProjection &projection,
                            const WaypointRendererSettings &settings,
                            const WaypointLook &look,
                            const TaskBehaviour &task_behaviour,
                            const PolarSettings &polar_settings,
                            const MoreData &basic,
                            const DerivedInfo &calculated,
                            WaypointPtr waypoint,
                            bool in_task) noexcept
{
  if (!waypoint || look.font == nullptr || look.bold_font == nullptr)
    return;

  const auto sc_opt =
    projection.GeoToScreenIfVisible(waypoint->location);
  if (!sc_opt)
    return;

  VisibleWaypoint vwp;
  PixelPoint pt = *sc_opt;
  vwp.Set(waypoint, pt, in_task);

  if (basic.location_available && basic.NavAltitudeAvailable() &&
      (waypoint->IsLandable() || waypoint->flags.watched)) {
    const GlidePolar &glide_polar =
      task_behaviour.route_planner.reach_polar_mode ==
          RoutePlannerConfig::Polar::TASK
        ? polar_settings.glide_polar_task
        : calculated.glide_polar_safety;
    const MacCready mac_cready(task_behaviour.glide, glide_polar);
    vwp.CalculateReachabilityDirect(basic, calculated.GetWindOrZero(),
                                    mac_cready, task_behaviour);
  }

  WaypointIconRenderer icon_renderer(settings, look, canvas,
                                     projection.GetMapScale() > 4000,
                                     projection.GetScreenAngle());
  vwp.DrawSymbol(icon_renderer);

  char altitude_unit[4];
  strcpy(altitude_unit, Units::GetAltitudeName());

  const Waypoint &way_point = *waypoint;

  char buffer[NAME_SIZE + 1];
  FormatWaypointMapLabel(buffer, ARRAY_SIZE(buffer), settings, task_behaviour,
                         basic, altitude_unit, way_point, vwp.reachable,
                         vwp.reach);
  if (buffer[0] == '\0')
    return;

  WaypointLabelList labels(projection.GetScreenRect());
  AppendWaypointMapLabelFromVisible(labels, vwp, settings, buffer);

  RenderWaypointLabelList(canvas, projection.GetScreenSize(), label_block,
                          labels, look);
}

void
WaypointRenderer::Render(Canvas &canvas, LabelBlock &label_block,
                         const MapWindowProjection &projection,
                         const struct WaypointRendererSettings &settings,
                         const PolarSettings &polar_settings,
                         const TaskBehaviour &task_behaviour,
                         const MoreData &basic, const DerivedInfo &calculated,
                         const ProtectedTaskManager *task,
                         const ProtectedRoutePlanner *route_planner) noexcept
{
  if (way_points == nullptr || way_points->IsEmpty())
    return;

  WaypointVisitorMap v(canvas, projection, settings, look, task_behaviour, basic);

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
                               projection.GetScreenDistanceMeters(),
                               [&v](const auto &w){ v.Add(w); });

  v.Calculate(route_planner, polar_settings, task_behaviour, calculated);

  v.Draw();

  RenderWaypointLabelList(canvas, projection.GetScreenSize(),
                          label_block, v.labels, look);
}
