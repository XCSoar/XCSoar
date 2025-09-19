// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
#include "Engine/GlideSolvers/GlideState.hpp"
#include "Engine/GlideSolvers/GlideResult.hpp"
#include "Engine/GlideSolvers/MacCready.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/AbstractTask.hpp"
#include "Engine/Task/Points/TaskPoint.hpp"
#include "Engine/Task/Unordered/UnorderedTaskPoint.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Engine/Task/Unordered/GotoTask.hpp"
#include "time/BrokenTime.hpp"
#include "time/RoughTime.hpp"
#include "time/LocalTime.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/ProtectedRoutePlanner.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Color.hpp"
#include "Units/Units.hpp"
#include "Geo/GeoPoint.hpp"
#include "util/TruncateString.hpp"
#include "util/StaticArray.hxx"
#include "util/Macros.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Engine/Route/ReachResult.hpp"
#include "Look/WaypointLook.hpp"
#include "Profile/Profile.hpp"

#include <cassert>
#include <cmath> // Needed for std::isfinite
#include <stdio.h>

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

class WaypointVisitorMap final
  : public TaskPointConstVisitor
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

  WaypointIconRenderer icon_renderer;

public:
  WaypointLabelList labels;
  WaypointPtr last_task_waypoint = nullptr;

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
    _tcscpy(altitude_unit, Units::GetAltitudeName());
  }


protected:
  void FormatTitle(TCHAR *buffer, size_t buffer_size,
                   const Waypoint &way_point) const noexcept {
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

  void FormatLabel(TCHAR *buffer, size_t buffer_size,
                   const Waypoint &way_point,
                   WaypointReachability reachable,
                   const ReachResult &reach) const noexcept {
    FormatTitle(buffer, buffer_size - 20, way_point);

    if (!way_point.IsLandable() && !way_point.flags.watched)
      return;

    if (settings.arrival_height_display == WaypointRendererSettings::ArrivalHeightDisplay::REQUIRED_GR ||
        settings.arrival_height_display == WaypointRendererSettings::ArrivalHeightDisplay::REQUIRED_GR_AND_TERRAIN) {
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

      size_t length = _tcslen(buffer);
      if (length > 0)
        buffer[length++] = _T(':');

      if (settings.arrival_height_display == WaypointRendererSettings::ArrivalHeightDisplay::REQUIRED_GR_AND_TERRAIN &&
         reach.IsReachableTerrain()) {
          int uah_terrain = (int)Units::ToUserAltitude(reach.terrain);
          StringFormatUnsafe(buffer + length, _T("%.1f/%d%s"), (double) gr,
                            uah_terrain, altitude_unit);
          return;
         }

      StringFormatUnsafe(buffer + length, _T("%.1f"), (double) gr);
      return;
    }

    if (reachable == WaypointReachability::INVALID)
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
    sc.x += 5;
    if ((vwp.IsReachable() &&
         settings.landable_style == WaypointRendererSettings::LandableStyle::PURPLE_CIRCLE) ||
        settings.vector_landable_rendering)
      // make space for the green circle
      sc.x += 5;

    labels.Add(buffer, sc, text_mode, bold,
               vwp.reachable != WaypointReachability::INVALID ? vwp.reach.direct : INT_MIN,
               vwp.in_task, way_point.IsLandable(), way_point.IsAirport(),
               watchedWaypoint);
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
    WaypointPtr current_wp = nullptr;
    switch (tp.GetType()) {
    case TaskPointType::UNORDERED:
      current_wp = static_cast<const UnorderedTaskPoint &>(tp).GetWaypointPtr();
      AddWaypoint(current_wp, true);
      break;

    case TaskPointType::START:
    case TaskPointType::AST:
    case TaskPointType::AAT:
    case TaskPointType::FINISH:
      current_wp = static_cast<const OrderedTaskPoint &>(tp).GetWaypointPtr();
      AddWaypoint(current_wp, true);
      break;
    }
    // Update the last visited task waypoint
    if (current_wp) {
        last_task_waypoint = current_wp;
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

static void
MapWaypointLabelRender(Canvas &canvas, PixelSize clip_size,
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

// Helper function to draw a time ring on the map
static void DrawMapRing(Canvas &canvas,
                        const MapWindowProjection &projection,
                        const PixelPoint &center_px,
                        double radius_meters,
                        const Color &ring_color,
                        const TCHAR *label_text,
                        const PixelPoint &aircraft_pos) noexcept
{
  // Convert radius to pixels using the correct method
  const double radius_pixels_d = projection.DistanceMetersToPixels(radius_meters);

  // Define line width
  constexpr unsigned line_width = 4; // Increased line width for better visibility

  // Draw the circle
  if (radius_pixels_d >= 1.0) { // Only draw if radius is at least 1 pixel
    const unsigned radius_pixels = static_cast<unsigned>(radius_pixels_d);

    // Select the pen and brush for the ring
    // Force the line style to be SOLID to ensure consistent appearance across platforms
    canvas.Select(Pen(line_width, ring_color));
    canvas.SelectHollowBrush();

    // Draw the circle
    canvas.DrawCircle(center_px, radius_pixels);

    // Set up the text box mode for the label
    TextInBoxMode text_mode;
    text_mode.shape = LabelShape::OUTLINED;  // Use outlined style
    text_mode.move_in_view = true;           // Move the label in view if needed

    // Calculate 8 possible positions around the circle (at 45-degree intervals)
    const int label_offset = radius_pixels + 5;  // 5 pixels from the circle edge

    // Define the 8 possible positions
    PixelPoint positions[8];
    TextInBoxMode::Alignment alignments[8];
    TextInBoxMode::VerticalPosition vert_positions[8];

    // North (top)
    positions[0] = { center_px.x, center_px.y - label_offset };
    alignments[0] = TextInBoxMode::Alignment::CENTER;
    vert_positions[0] = TextInBoxMode::VerticalPosition::ABOVE;

    // Northeast
    positions[1] = { center_px.x + (int)(label_offset * 0.7071), center_px.y - (int)(label_offset * 0.7071) }; // Use 0.7071 for sqrt(2)/2
    alignments[1] = TextInBoxMode::Alignment::LEFT;
    vert_positions[1] = TextInBoxMode::VerticalPosition::ABOVE;

    // East (right)
    positions[2] = { center_px.x + label_offset, center_px.y };
    alignments[2] = TextInBoxMode::Alignment::LEFT;
    vert_positions[2] = TextInBoxMode::VerticalPosition::CENTERED;

    // Southeast
    positions[3] = { center_px.x + (int)(label_offset * 0.7071), center_px.y + (int)(label_offset * 0.7071) };
    alignments[3] = TextInBoxMode::Alignment::LEFT;
    vert_positions[3] = TextInBoxMode::VerticalPosition::BELOW;

    // South (bottom)
    positions[4] = { center_px.x, center_px.y + label_offset };
    alignments[4] = TextInBoxMode::Alignment::CENTER;
    vert_positions[4] = TextInBoxMode::VerticalPosition::BELOW;

    // Southwest
    positions[5] = { center_px.x - (int)(label_offset * 0.7071), center_px.y + (int)(label_offset * 0.7071) };
    alignments[5] = TextInBoxMode::Alignment::RIGHT;
    vert_positions[5] = TextInBoxMode::VerticalPosition::BELOW;

    // West (left)
    positions[6] = { center_px.x - label_offset, center_px.y };
    alignments[6] = TextInBoxMode::Alignment::RIGHT;
    vert_positions[6] = TextInBoxMode::VerticalPosition::CENTERED;

    // Northwest
    positions[7] = { center_px.x - (int)(label_offset * 0.7071), center_px.y - (int)(label_offset * 0.7071) };
    alignments[7] = TextInBoxMode::Alignment::RIGHT;
    vert_positions[7] = TextInBoxMode::VerticalPosition::ABOVE;

    // Find the position closest to the aircraft
    int closest_idx = 0;
    int min_distance_squared = INT_MAX;

    for (int i = 0; i < 8; i++) {
      int dx = positions[i].x - aircraft_pos.x;
      int dy = positions[i].y - aircraft_pos.y;
      int distance_squared = dx * dx + dy * dy;

      if (distance_squared < min_distance_squared) {
        min_distance_squared = distance_squared;
        closest_idx = i;
      }
    }

    // Set the alignment and vertical position for the closest position
    text_mode.align = alignments[closest_idx];
    text_mode.vertical_position = vert_positions[closest_idx];

    // Draw the label at the closest position
    TextInBox(canvas, label_text, positions[closest_idx], text_mode, projection.GetScreenSize());
  }
}


void
WaypointRenderer::Render(Canvas &canvas, LabelBlock &label_block,
                         const MapWindowProjection &projection,
                         const struct WaypointRendererSettings &settings,
                         const PolarSettings &polar_settings,
                         const TaskBehaviour &task_behaviour,
                         const MoreData &basic, const DerivedInfo &calculated,
                         const ProtectedTaskManager *task,
                         const ProtectedRoutePlanner *route_planner,
                         const ComputerSettings &computer_settings) noexcept
{
  if (way_points == nullptr || way_points->IsEmpty())
    return;

  WaypointVisitorMap v(canvas, projection, settings, look, task_behaviour, basic);

  GeoPoint ring_center; // Variable to store ring center
  bool draw_ring = false; // Flag to indicate if ring should be drawn


  WaypointPtr goto_wp = nullptr; // Store GOTO waypoint if applicable

  if (task != nullptr) {
    ProtectedTaskManager::Lease task_manager(*task);

    const TaskStats &task_stats = task_manager->GetStats();

    if (task_stats.task_valid)
      v.SetTaskValid();

    const AbstractTask *atask = task_manager->GetActiveTask();
    if (atask != nullptr) {
      // Check if it's a GOTO task first
      if (atask->GetType() == TaskType::GOTO) {
          const GotoTask* goto_task = static_cast<const GotoTask*>(atask);
          // Get the active (and only) point in a GotoTask
          const TaskWaypoint* goto_task_point = goto_task->GetActiveTaskPoint();
          if (goto_task_point != nullptr) {
              // GetActiveTaskPoint in GotoTask returns the internal UnorderedTaskPoint*
              const UnorderedTaskPoint* utp = static_cast<const UnorderedTaskPoint*>(goto_task_point);
              goto_wp = utp->GetWaypointPtr();
              // Add other TaskWaypoint derived types if necessary
          }
      }

      // Visit all task points (needed for drawing them anyway,
      // and populates v.last_task_waypoint for non-GOTO tasks)
      atask->AcceptTaskPointVisitor(v);

      // Determine ring center
      if (goto_wp) {
          // GOTO task takes precedence
          ring_center = goto_wp->location;
          draw_ring = true;
      } else if (v.last_task_waypoint) {
          // Fallback to last waypoint of the current task
          ring_center = v.last_task_waypoint->location;
          draw_ring = true;
      }
    }
  }


  way_points->VisitWithinRange(projection.GetGeoScreenCenter(),
                               projection.GetScreenDistanceMeters(),
                               [&v](const auto &w){ v.Add(w); });


  v.Calculate(route_planner, polar_settings, task_behaviour, calculated);

  // Draw waypoint icons and symbols
  v.Draw();

  // Draw time rings if a center was determined
  if (draw_ring) {
    // Always convert center to screen coordinates regardless of visibility
    auto center_px = projection.GeoToScreen(ring_center);
    // Get aircraft position for label placement optimization
    PixelPoint aircraft_pos = projection.GeoToScreen(basic.location);

    // Get the common speed from the FIN ETE calculation
    const TaskStats &task_stats = calculated.task_stats;
    double fin_ete_speed = 0;
    if (task_stats.task_valid && task_stats.total.remaining_effective.IsDefined()) {
      fin_ete_speed = task_stats.total.remaining_effective.GetSpeed() * 3.6; // m/s to km/h
    }
    // If speed is not available or zero, use a default value
    if (fin_ete_speed <= 0) {
      fin_ete_speed = 100; // Default 100 km/h
    }

    // --- Draw Original ArrivalTimeRing (User-defined time) ---
    {
      double radius_meters_arrival = 0;
      unsigned minutes_of_day = 17 * 60 + 0; // Default to 5:00 PM (1700)
      Profile::Get(ProfileKeys::ArrivalTimeRingTime, minutes_of_day);

      bool draw_original_ring = false; // Flag to control drawing

      if (basic.time_available) {
        RoughTimeDelta utc_offset = computer_settings.utc_offset;
        TimeStamp local_time = TimeLocal(basic.time, utc_offset);
        BrokenTime target_time(minutes_of_day / 60, minutes_of_day % 60, 0);
        BrokenTime current_broken_time = BrokenTime::FromSecondOfDayChecked(
            (unsigned)local_time.ToDuration().count());

        int current_seconds = current_broken_time.GetSecondOfDay();
        int target_seconds = target_time.GetSecondOfDay();
        int seconds_diff = target_seconds - current_seconds;
        if (seconds_diff < 0) {
          seconds_diff += 24 * 3600; // Add 24 hours if target is next day
        }
        double hours_to_target = seconds_diff / 3600.0;

        // *** Only calculate radius and set flag if time delta is <= 6 hours ***
        if (hours_to_target <= 6.0) {
            radius_meters_arrival = fin_ete_speed * hours_to_target * 1000; // km/h * h * 1000 = m
            draw_original_ring = true;
        }
      } else {
        // Fallback: Draw if time is not available (using default radius)
        constexpr double radius_miles = 10.0;
        constexpr double meters_per_mile = 1609.34;
        radius_meters_arrival = radius_miles * meters_per_mile;
        draw_original_ring = true;
      }

      // Only draw the original ring if the flag is set
      if (draw_original_ring) {
          // Format the label
          TCHAR arrival_label[32];
      _stprintf(arrival_label, _T("Arrival %02u:%02u"),
                minutes_of_day / 60, minutes_of_day % 60);

      // Define color
      constexpr Color arrival_ring_color = Color(255, 0, 255); // Magenta

          // Draw the ring using the helper function
          DrawMapRing(canvas, projection, center_px, radius_meters_arrival,
                      arrival_ring_color, arrival_label, aircraft_pos);
      }
    }

    // --- Draw New "Arrival Ring AAT" (AAT time remaining, if enabled) ---
    if (task_behaviour.arrival_ring_aat_enabled) { // Check if the setting is enabled
      const CommonStats &common_stats = calculated.common_stats;
      // Check if AAT time remaining is valid and we have an ordered task
      // (AAT time only makes sense for ordered tasks like AAT)
      if (task_stats.task_valid && task_stats.has_targets &&
          std::isfinite(common_stats.aat_time_remaining.count())) {

        // Get the active task to retrieve task-specific settings
        const AbstractTask *atask_ptr = nullptr;
        if (task != nullptr) {
            ProtectedTaskManager::Lease task_manager(*task);
            atask_ptr = task_manager->GetActiveTask();
        }

        // Convert AAT time remaining (seconds) to hours
        double hours_aat = common_stats.aat_time_remaining.count() / 3600.0;

        // Calculate radius based on AAT time remaining
        // Only draw if time remaining is positive
        if (hours_aat > 0) {
            double radius_meters_aat = fin_ete_speed * hours_aat * 1000; // km/h * h * 1000 = m

            // Format the AAT minimum time label
            TCHAR aat_label[32];
            // Default to global setting
            auto min_time_duration = task_behaviour.ordered_defaults.aat_min_time;

            // Try to get task-specific setting if it's an OrderedTask
            if (atask_ptr != nullptr && atask_ptr->GetType() == TaskType::ORDERED) {
                const OrderedTask* ordered_task_ptr = static_cast<const OrderedTask*>(atask_ptr);
                min_time_duration = ordered_task_ptr->GetOrderedTaskSettings().aat_min_time;
            }
            const unsigned total_minutes = static_cast<unsigned>(min_time_duration.count() / 60.0); // Assuming duration is in seconds
            const unsigned aat_hours = total_minutes / 60;
            const unsigned aat_minutes = total_minutes % 60;
            _stprintf(aat_label, _T("AAT %u:%02u"), aat_hours, aat_minutes);

            // Define color
            constexpr Color aat_ring_color = Color(0, 255, 255); // Cyan

            // Draw the ring using the helper function
            DrawMapRing(canvas, projection, center_px, radius_meters_aat,
                        aat_ring_color, aat_label, aircraft_pos);
        }
      }
    }
  }

  // Draw waypoint labels 
  MapWaypointLabelRender(canvas, projection.GetScreenSize(),
                         label_block, v.labels, look);
}
