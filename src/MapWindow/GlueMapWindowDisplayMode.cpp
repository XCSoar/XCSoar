// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GlueMapWindow.hpp"
#include "UserMapScale.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Topography/Thread.hpp"
#include "Terrain/Thread.hpp"
#include "Interface.hpp"
#include "Profile/Profile.hpp"
#include "Screen/Layout.hpp"
#include "PageActions.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Globals.hpp"
#endif

#include <algorithm> // for std::clamp()
#include <cmath>

void
OffsetHistory::Reset() noexcept
{
  offsets.fill(PixelPoint{0, 0});
}

inline void
OffsetHistory::Add(PixelPoint p) noexcept
{
  offsets[pos] = p;
  pos = (pos + 1) % offsets.size();
}

inline PixelPoint
OffsetHistory::GetAverage() const noexcept
{
  int x = 0;
  int y = 0;

  for (auto i = offsets.begin(), end = offsets.end(); i != end; ++i) {
    x += i->x;
    y += i->y;
  }

  PixelPoint avg;
  avg.x = x / (int) offsets.size();
  avg.y = y / (int) offsets.size();

  return avg;
}

void
GlueMapWindow::SetPan(bool enable) noexcept
{
  switch (follow_mode) {
  case FOLLOW_SELF:
    if (!enable)
      return;

    follow_mode = FOLLOW_PAN;
    break;

  case FOLLOW_PAN:
    if (enable)
      return;

    follow_mode = FOLLOW_SELF;
    break;
  }

  FullRedraw();
}

void
GlueMapWindow::TogglePan() noexcept
{
  switch (follow_mode) {
  case FOLLOW_SELF:
    follow_mode = FOLLOW_PAN;
    break;

  case FOLLOW_PAN:
    follow_mode = FOLLOW_SELF;
    break;
  }

  FullRedraw();
}

void
GlueMapWindow::PanTo(const GeoPoint &location) noexcept
{
  follow_mode = FOLLOW_PAN;
  SetLocation(location);

  FullRedraw();
}

void
GlueMapWindow::UpdateScreenBounds() noexcept
{
  MapWindow::UpdateScreenBounds();

  if (topography_thread != nullptr &&
      visible_projection.IsValid() &&
      CommonInterface::GetMapSettings().topography_enabled)
    topography_thread->Trigger(visible_projection);

  /* always service terrain even if it's not used by the map, because
     it's used by other calculations, therefore don't check if terrain
     display is enabled */
  if (terrain_thread != nullptr &&
      visible_projection.IsValid())
    terrain_thread->Trigger(visible_projection);
}

void
GlueMapWindow::PersistCurrentScale() noexcept
{
  const bool circling =
    CommonInterface::GetUIState().display_mode == DisplayMode::CIRCLING;
  MapSettings &settings = CommonInterface::SetMapSettings();

  if (circling && settings.circle_zoom_enabled)
    settings.circling_scale = visible_projection.GetScale();
  else
    settings.cruise_scale = visible_projection.GetScale();

  SaveDisplayModeScales();
}

void
GlueMapWindow::SetMapScale(double scale) noexcept
{
#ifdef ENABLE_OPENGL
  CancelZoomAnimation();
#endif

  MapWindow::SetMapScale(scale);
  OnProjectionModified();
  PersistCurrentScale();
}

void
GlueMapWindow::SetFreeMapScale(double scale) noexcept
{
#ifdef ENABLE_OPENGL
  CancelZoomAnimation();
#endif

  visible_projection.SetFreeMapScale(scale);
  OnProjectionModified();
  PersistCurrentScale();
}

void
GlueMapWindow::AnimateFreeMapScale(double scale) noexcept
{
  scale = ClampUserMapScale(scale);

#ifndef ENABLE_OPENGL
  SetFreeMapScale(scale);
  QuickRedraw();
#else
  if (!visible_projection.IsValid()) {
    SetFreeMapScale(scale);
    QuickRedraw();
    return;
  }

  zoom_from_map_scale = visible_projection.GetMapScale();
  zoom_to_map_scale = scale;

  /* persist the target scale immediately */
  {
    const double saved_scale = visible_projection.GetScale();
    visible_projection.SetFreeMapScale(scale);
    PersistCurrentScale();
    visible_projection.SetScale(saved_scale);
  }

  if (zoom_from_map_scale <= 0 || zoom_to_map_scale <= 0 ||
      std::fabs(zoom_from_map_scale - scale) / zoom_from_map_scale < 0.001) {
    SetFreeMapScale(scale);
    QuickRedraw();
    return;
  }

  zoom_start_time = std::chrono::steady_clock::now();
  zoom_timer.Schedule(std::chrono::milliseconds(16));
  OnZoomTimer();
#endif
}

#ifdef ENABLE_OPENGL

void
GlueMapWindow::CancelZoomAnimation() noexcept
{
  if (!zoom_timer.IsPending())
    return;

  zoom_timer.Cancel();

  /* the target scale was already persisted; snap the visible
     projection so a mid-tween cancel does not leave an intermediate
     scale fighting the saved setting */
  if (zoom_to_map_scale > 0 && visible_projection.IsValid()) {
    visible_projection.SetFreeMapScale(zoom_to_map_scale);
    OnProjectionModified();
  }
}

void
GlueMapWindow::OnZoomTimer() noexcept
{
  constexpr auto duration = std::chrono::milliseconds(180);
  const auto elapsed = std::chrono::steady_clock::now() - zoom_start_time;
  const auto elapsed_ms =
    std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
  double t = double(elapsed_ms.count()) / double(duration.count());

  if (t >= 1) {
    visible_projection.SetFreeMapScale(zoom_to_map_scale);
    OnProjectionModified();
    QuickRedraw();
    zoom_timer.Cancel();
    return;
  }

  /* ease-out cubic; interpolate in log space so multiplicative zoom
     feels constant */
  t = 1 - (1 - t) * (1 - t) * (1 - t);
  const double from_log = std::log(zoom_from_map_scale);
  const double to_log = std::log(zoom_to_map_scale);
  const double scale = std::exp(from_log + (to_log - from_log) * t);

  visible_projection.SetFreeMapScale(scale);
  OnProjectionModified();
  QuickRedraw();
  zoom_timer.Schedule(std::chrono::milliseconds(16));
}

#endif

void
GlueMapWindow::RestoreMapScale() noexcept
{
#ifdef ENABLE_OPENGL
  /* stop a pending keyboard/wheel tween so OnZoomTimer cannot overwrite
     the restored cruise/circling scale with a stale target */
  CancelZoomAnimation();
#endif

  const MapSettings &settings = CommonInterface::GetMapSettings();
  const bool circling =
    CommonInterface::GetUIState().display_mode == DisplayMode::CIRCLING;

  double scale = settings.circle_zoom_enabled && circling
    ? settings.circling_scale
    : settings.cruise_scale;

#ifdef ENABLE_OPENGL
  if (OpenGL::max_map_scale > 0) {
    /* enforce the GPU-imposed zoom-out limit;
       min pixels/meter = map_resolution_factor / max_map_scale */
    const double min_scale =
      double(visible_projection.GetMinScreenDistance()) / 8.0
      / double(OpenGL::max_map_scale);
    scale = std::max(scale, min_scale);
  }
#endif

  visible_projection.SetScale(scale);
  OnProjectionModified();
}

inline void
GlueMapWindow::SaveDisplayModeScales() noexcept
{
  const MapSettings &settings = CommonInterface::GetMapSettings();

  Profile::Set(ProfileKeys::ClimbMapScale, (int)(settings.circling_scale * 10000));
  Profile::Set(ProfileKeys::CruiseMapScale, (int)(settings.cruise_scale * 10000));
}

inline void
GlueMapWindow::SwitchZoomClimb() noexcept
{
  const MapSettings &settings = CommonInterface::GetMapSettings();

  if (settings.circle_zoom_enabled)
    RestoreMapScale();
}

void
GlueMapWindow::UpdateDisplayMode() noexcept
{
  /* not using MapWindowBlackboard here because these methods are
     called by the main thread */
  enum DisplayMode new_mode = CommonInterface::GetUIState().display_mode;

  const bool was_circling = last_display_mode == DisplayMode::CIRCLING;
  const bool is_circling = new_mode == DisplayMode::CIRCLING;

  if (!was_circling && is_circling)
    offset_history.Reset();

  last_display_mode = new_mode;

  if (is_circling != was_circling)
    SwitchZoomClimb();
}

void
GlueMapWindow::UpdateScreenAngle() noexcept
{
  /* not using MapWindowBlackboard here because these methods are
     called by the main thread */
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const MapSettings &settings = CommonInterface::GetMapSettings();
  const UIState &ui_state = CommonInterface::GetUIState();

  // force north-up if the current page is a dedicated MAP_NORTH_UP page
  const PageLayout &layout = PageActions::GetConfiguredLayout();
  if (layout.main == PageLayout::Main::MAP_NORTH_UP) {
#ifdef HAVE_MULTI_TOUCH
    /* a north-up page rejects temporary twist; clear so the angle
       cannot reappear when leaving this page */
    manual_rotation = false;
#endif
    visible_projection.SetScreenAngle(Angle::Zero());
    OnProjectionModified();
    compass_visible = false;
    return;
  }

#ifdef HAVE_MULTI_TOUCH
  /* a two-finger twist temporarily overrides the configured
     orientation; the angle is held while panning and released (falling
     back to the configured orientation) once pan mode is left */
  if (manual_rotation) {
    if (IsPanning() || GestureOwnsMap()) {
      visible_projection.SetScreenAngle(manual_rotation_angle);
      OnProjectionModified();
      compass_visible = true;
      return;
    }

    manual_rotation = false;
  }
#endif

  MapOrientation orientation =
    ui_state.display_mode == DisplayMode::CIRCLING
    ? settings.circling_orientation
    : settings.cruise_orientation;

  if (orientation == MapOrientation::TARGET_UP &&
      calculated.task_stats.current_leg.vector_remaining.IsValid())
    visible_projection.SetScreenAngle(calculated.task_stats.current_leg.
                                      vector_remaining.bearing);
  else if (orientation == MapOrientation::HEADING_UP)
    visible_projection.SetScreenAngle(
      basic.attitude.heading_available ? basic.attitude.heading : Angle::Zero());
  else if (orientation == MapOrientation::NORTH_UP)
    visible_projection.SetScreenAngle(Angle::Zero());
  else if (orientation == MapOrientation::WIND_UP &&
           calculated.wind_available &&
           calculated.wind.norm >= 0.5)
    visible_projection.SetScreenAngle(calculated.wind.bearing);
  else
    // normal, glider forward
    visible_projection.SetScreenAngle(
      basic.track_available ? basic.track : Angle::Zero());

  OnProjectionModified();

  compass_visible = orientation != MapOrientation::NORTH_UP;
}

void
GlueMapWindow::UpdateMapScale() noexcept
{
  /* not using MapWindowBlackboard here because these methods are
     called by the main thread */
  const DerivedInfo &calculated = CommonInterface::Calculated();
  MapSettings &settings = CommonInterface::SetMapSettings();
  const bool circling =
    CommonInterface::GetUIState().display_mode == DisplayMode::CIRCLING;

  if (circling && settings.circle_zoom_enabled)
    return;

  if (!IsNearSelf() || GestureOwnsMap())
    return;

  auto distance = calculated.auto_zoom_distance;
  if (settings.auto_zoom_enabled && distance > 0) {
    // Calculate distance percentage between plane symbol and map edge
    // 50: centered  100: at edge of map
    int auto_zoom_factor = circling
      ? 50
      : 100 - settings.glider_screen_position;

    // Leave 5% of full distance for target display
    auto_zoom_factor -= 5;
    // Adjust to account for map scale units
    auto_zoom_factor *= 8;

    distance /= auto_zoom_factor / 100.;

    // Clip map auto zoom range to reasonable values
    distance = std::clamp(distance, 525.,
                          settings.max_auto_zoom_distance / 10.);

    visible_projection.SetFreeMapScale(distance);
    settings.cruise_scale = visible_projection.GetScale();

    OnProjectionModified();
  }
}

void
GlueMapWindow::SetLocation(const GeoPoint location) noexcept
{
  MapWindow::SetLocation(location);
  OnProjectionModified();
}

void
GlueMapWindow::SetLocationLazy(const GeoPoint location) noexcept
{
  if (!visible_projection.IsValid()) {
    SetLocation(location);
    return;
  }

  const auto distance_meters =
    visible_projection.GetGeoLocation().DistanceS(location);
  const auto distance_pixels =
    visible_projection.DistanceMetersToPixels(distance_meters);
  if (distance_pixels > 0.5)
    SetLocation(location);
}

void
GlueMapWindow::UpdateProjection() noexcept
{
  const PixelRect rc = GetClientRect();

  /* not using MapWindowBlackboard here because these methods are
     called by the main thread */
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const MapSettings &settings_map = CommonInterface::GetMapSettings();
  const bool circling =
    CommonInterface::GetUIState().display_mode == DisplayMode::CIRCLING;

  const auto center = rc.GetCenter();

  /* Gesture ownership is not pan UI: while still FOLLOWING the
     aircraft, freeze origin/GPS so two-finger down does not jump to
     a pan-style centred projection below the pan-entry threshold. */
  const bool freeze_for_gesture = GestureOwnsMap() && IsNearSelf();

  if (freeze_for_gesture) {
    /* keep the current screen origin */
  } else if (circling || !IsNearSelf())
    visible_projection.SetScreenOrigin(center);
  else if (settings_map.cruise_orientation == MapOrientation::NORTH_UP ||
           settings_map.cruise_orientation == MapOrientation::WIND_UP) {
    PixelPoint offset{0, 0};
    if (settings_map.glider_screen_position != 50 &&
        settings_map.map_shift_bias != MapShiftBias::NONE) {
      double x = 0, y = 0;
      if (settings_map.map_shift_bias == MapShiftBias::TRACK) {
        if (basic.track_available &&
            basic.ground_speed_available &&
             /* 8 m/s ~ 30 km/h */
            basic.ground_speed > 8) {
          auto angle = basic.track.Reciprocal() - visible_projection.GetScreenAngle();

          const auto sc = angle.SinCos();
          x = sc.first;
          y = sc.second;
        }
      } else if (settings_map.map_shift_bias == MapShiftBias::TARGET) {
        if (calculated.task_stats.current_leg.solution_remaining.IsDefined()) {
          auto angle = calculated.task_stats.current_leg.solution_remaining
              .vector.bearing.Reciprocal() - visible_projection.GetScreenAngle();

          const auto sc = angle.SinCos();
          x = sc.first;
          y = sc.second;
        }
      }
      double position_factor = (50. - settings_map.glider_screen_position) / 100.;
      offset.x = int(x * rc.GetWidth() * position_factor);
      offset.y = int(-y * rc.GetHeight() * position_factor);
      offset_history.Add(offset);
      offset = offset_history.GetAverage();
    }
    visible_projection.SetScreenOrigin(center + offset);
  } else
    visible_projection.SetScreenOrigin(center.x,
        ((rc.top - rc.bottom) * settings_map.glider_screen_position / 100) + rc.bottom);

  if (freeze_for_gesture || !IsNearSelf()) {
    /* no-op - the Projection's location is updated manually */
  } else if (circling && calculated.thermal_locator.estimate_valid) {
    const auto d_t = calculated.thermal_locator.estimate_location.DistanceS(basic.location);
    if (d_t <= 0) {
      SetLocationLazy(basic.location);
    } else {
      const auto d_max = 2 * visible_projection.GetMapScale();
      const auto t = std::min(d_t, d_max)/d_t;
      SetLocation(basic.location.Interpolate(calculated.thermal_locator.estimate_location,
                                               t));
    }
  } else if (basic.location_available)
    // Pan is off
    SetLocationLazy(basic.location);
  else if (!visible_projection.IsValid() && terrain != nullptr) {
    /* if there's no GPS fix yet and no home waypoint, start at the
       map center, to avoid showing a fully white map, which confuses
       users */
    if (const auto center = terrain->GetTerrainCenter();
        center.IsValid())
      SetLocation(center);
  }

  OnProjectionModified();
}
