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

#include "GlueMapWindow.hpp"
#include "Interface.hpp"
#include "Profile/Profile.hpp"
#include "Screen/Layout.hpp"

ZoomClimb_t::ZoomClimb_t():
  CruiseScale(fixed_one / 60),
  ClimbScale(fixed_one / 2),
  last_isclimb(false) {}


const RasterPoint OffsetHistory::zeroPoint = {0, 0};

void
OffsetHistory::reset()
{
  for (unsigned int i = 0; i < historySize; i++)
    offsets[i] = zeroPoint;
}

void
OffsetHistory::add(int x, int y)
{
  RasterPoint point;
  point.x = x;
  point.y = y;
  offsets[pos] = point;
  pos = (pos + 1) % historySize;
}

RasterPoint
OffsetHistory::average() const
{
  int x = 0;
  int y = 0;

  for (unsigned int i = 0; i < historySize; i++) {
    x += offsets[i].x;
    y += offsets[i].y;
  }

  RasterPoint avg;
  avg.x = x / (int) historySize;
  avg.y = y / (int) historySize;

  return avg;
}

void
GlueMapWindow::SetPan(bool enable)
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

  UpdateProjection();
  FullRedraw();
}

void
GlueMapWindow::TogglePan()
{
  switch (follow_mode) {
  case FOLLOW_SELF:
    follow_mode = FOLLOW_PAN;
    break;

  case FOLLOW_PAN:
    follow_mode = FOLLOW_SELF;
    break;
  }

  UpdateProjection();
  FullRedraw();
}

void
GlueMapWindow::PanTo(const GeoPoint &location)
{
  follow_mode = FOLLOW_PAN;
  visible_projection.SetGeoLocation(location);

  UpdateProjection();
  FullRedraw();
}

void
GlueMapWindow::SetMapScale(const fixed x)
{
  MapWindow::SetMapScale(x);

  if (GetDisplayMode() == DM_CIRCLING && SettingsMap().CircleZoom)
    // save cruise scale
    zoomclimb.ClimbScale = visible_projection.GetScale();
  else
    zoomclimb.CruiseScale = visible_projection.GetScale();

  SaveDisplayModeScales();
}

void
GlueMapWindow::LoadDisplayModeScales()
{
  fixed tmp;
  if (Profile::Get(szProfileClimbMapScale, tmp))
    zoomclimb.ClimbScale = tmp / 10000;
  else
    zoomclimb.ClimbScale = fixed_one / Layout::FastScale(2);

  if (Profile::Get(szProfileCruiseMapScale, tmp))
    zoomclimb.CruiseScale = tmp / 10000;
  else
    zoomclimb.CruiseScale = fixed_one / Layout::FastScale(60);
}

void
GlueMapWindow::SaveDisplayModeScales()
{
  Profile::Set(szProfileClimbMapScale, (int)(zoomclimb.ClimbScale * 10000));
  Profile::Set(szProfileCruiseMapScale, (int)(zoomclimb.CruiseScale * 10000));
}

void
GlueMapWindow::SwitchZoomClimb()
{
  bool isclimb = (GetDisplayMode() == DM_CIRCLING);

  if (SettingsMap().CircleZoom) {
    if (isclimb != zoomclimb.last_isclimb) {
      if (isclimb) {
        // save cruise scale
        zoomclimb.CruiseScale = visible_projection.GetScale();
        // switch to climb scale
        visible_projection.SetScale(zoomclimb.ClimbScale);
      } else {
        // leaving climb
        // save cruise scale
        zoomclimb.ClimbScale = visible_projection.GetScale();
        // switch to climb scale
        visible_projection.SetScale(zoomclimb.CruiseScale);
      }

      SaveDisplayModeScales();
      zoomclimb.last_isclimb = isclimb;
    }
  }
}

void
GlueMapWindow::UpdateDisplayMode()
{
  /* not using MapWindowBlackboard here because these methods are
     called by the main thread */
  enum DisplayMode new_mode =
    GetNewDisplayMode(CommonInterface::GetUIState(),
                      CommonInterface::Calculated());

  if (DisplayMode != new_mode && new_mode == DM_CIRCLING)
    offsetHistory.reset();

  DisplayMode = new_mode;
  SwitchZoomClimb();
}

void
GlueMapWindow::UpdateScreenAngle()
{
  /* not using MapWindowBlackboard here because these methods are
     called by the main thread */
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const SETTINGS_MAP &settings = CommonInterface::SettingsMap();

  DisplayOrientation_t orientation =
      (GetDisplayMode() == DM_CIRCLING) ?
          settings.OrientationCircling : settings.OrientationCruise;

  if (orientation == TARGETUP &&
      calculated.task_stats.current_leg.solution_remaining.IsDefined())
    visible_projection.SetScreenAngle(calculated.task_stats.current_leg.
                                      solution_remaining.vector.Bearing);
  else if (orientation == NORTHUP || !basic.track_available)
    visible_projection.SetScreenAngle(Angle::zero());
  else
    // normal, glider forward
    visible_projection.SetScreenAngle(basic.track);

  compass_visible = orientation != NORTHUP;
}

void
GlueMapWindow::UpdateMapScale()
{
  /* not using MapWindowBlackboard here because these methods are
     called by the main thread */
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const SETTINGS_MAP &settings = CommonInterface::SettingsMap();

  if (GetDisplayMode() == DM_CIRCLING && SettingsMap().CircleZoom)
    return;

  if (!IsNearSelf())
    return;

  fixed wpd = calculated.auto_zoom_distance;
  if (settings.AutoZoom && positive(wpd)) {
    // Calculate distance percentage between plane symbol and map edge
    // 50: centered  100: at edge of map
    int AutoZoomFactor = (GetDisplayMode() == DM_CIRCLING) ?
                                 50 : 100 - SettingsMap().GliderScreenPosition;
    // Leave 5% of full distance for target display
    AutoZoomFactor -= 5;
    // Adjust to account for map scale units
    AutoZoomFactor *= 8;
    wpd = wpd / ((fixed) AutoZoomFactor / fixed_int_constant(100));
    // Clip map auto zoom range to reasonable values
    wpd = max(fixed_int_constant(525),
              min(SettingsMap().MaxAutoZoomDistance / fixed_int_constant(10), wpd));
    visible_projection.SetFreeMapScale(wpd);
  }
}

void
GlueMapWindow::UpdateProjection()
{
  const PixelRect rc = get_client_rect();

  /* not using MapWindowBlackboard here because these methods are
     called by the main thread */
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const SETTINGS_MAP &settings_map = CommonInterface::SettingsMap();

  RasterPoint center;
  center.x = (rc.left + rc.right) / 2;
  center.y = (rc.top + rc.bottom) / 2;

  if (GetDisplayMode() == DM_CIRCLING || !IsNearSelf())
    visible_projection.SetScreenOrigin(center.x, center.y);
  else if (settings_map.OrientationCruise == NORTHUP) {
    RasterPoint offset = OffsetHistory::zeroPoint;
    if (settings_map.GliderScreenPosition != 50 &&
        settings_map.MapShiftBias != MAP_SHIFT_BIAS_NONE) {
      fixed x = fixed_zero;
      fixed y = fixed_zero;
      if (settings_map.MapShiftBias == MAP_SHIFT_BIAS_TRACK) {
        if (basic.track_available &&
            basic.ground_speed_available &&
            basic.ground_speed > fixed_int_constant(8)) /* 8 m/s ~ 30 km/h */
          basic.track.Reciprocal().sin_cos(x, y);
      } else if (settings_map.MapShiftBias == MAP_SHIFT_BIAS_TARGET) {
        if (calculated.task_stats.current_leg.solution_remaining.IsDefined())
          calculated.task_stats.current_leg.solution_remaining
                      .vector.Bearing.Reciprocal().sin_cos(x, y);
      }
      fixed gspFactor = (fixed) (50 - settings_map.GliderScreenPosition) / 100;
      offset.x = x * (rc.right - rc.left) * gspFactor;
      offset.y = y * (rc.top - rc.bottom) * gspFactor;
      offsetHistory.add(offset);
      offset = offsetHistory.average();
    }
    visible_projection.SetScreenOrigin(center.x + offset.x, center.y + offset.y);
  } else
    visible_projection.SetScreenOrigin(center.x,
        ((rc.top - rc.bottom) * settings_map.GliderScreenPosition / 100) + rc.bottom);

  if (!IsNearSelf()) {
    /* no-op - the Projection's location is updated manually */
  } else if (GetDisplayMode() == DM_CIRCLING &&
           calculated.thermal_locator.estimate_valid) {
    const fixed d_t = calculated.thermal_locator.estimate_location.distance(basic.location);
    if (!positive(d_t)) {
      SetLocation(basic.location);
    } else {
      const fixed d_max = visible_projection.GetMapScale() * fixed_two;
      const fixed t = std::min(d_t, d_max)/d_t;
      SetLocation(basic.location.interpolate(calculated.thermal_locator.estimate_location,
                                               t));
    }
  } else
    // Pan is off
    SetLocation(basic.location);

  visible_projection.UpdateScreenBounds();
}
