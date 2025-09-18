// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GlueMapWindow.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Topography/Thread.hpp"
#include "Terrain/Thread.hpp"
#include "Interface.hpp"
#include "Profile/Profile.hpp"
#include "Screen/Layout.hpp"
#include "PageActions.hpp"

#include <algorithm> // for std::clamp()

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
  visible_projection.UpdateScreenBounds();

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
GlueMapWindow::SetMapScale(double scale) noexcept
{
  MapWindow::SetMapScale(scale);
  OnProjectionModified();

  const bool circling =
    CommonInterface::GetUIState().display_mode == DisplayMode::CIRCLING;
  MapSettings &settings = CommonInterface::SetMapSettings();

  if (circling && settings.circle_zoom_enabled)
    // save cruise scale
    settings.circling_scale = visible_projection.GetScale();
  else
    settings.cruise_scale = visible_projection.GetScale();

  SaveDisplayModeScales();
}

void
GlueMapWindow::RestoreMapScale() noexcept
{
  const MapSettings &settings = CommonInterface::GetMapSettings();
  const bool circling =
    CommonInterface::GetUIState().display_mode == DisplayMode::CIRCLING;

  visible_projection.SetScale(settings.circle_zoom_enabled && circling
                              ? settings.circling_scale
                              : settings.cruise_scale);
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

  // force north-up if the current page is MAP_NORTH_UP
  const PageLayout &layout = PageActions::GetConfiguredLayout();
  if (layout.main == PageLayout::Main::MAP_NORTH_UP) {
    visible_projection.SetScreenAngle(Angle::Zero());
    OnProjectionModified();
    compass_visible = false;
    return;
  }

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

  if (!IsNearSelf())
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

  if (circling || !IsNearSelf())
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

  if (!IsNearSelf()) {
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
