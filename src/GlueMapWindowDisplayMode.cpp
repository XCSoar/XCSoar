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

#include "Profile/Profile.hpp"
#include "Screen/Layout.hpp"

ZoomClimb_t::ZoomClimb_t():
  CruiseScale(fixed_one / 60),
  ClimbScale(fixed_one / 2),
  last_isclimb(false),
  last_targetpan(false) {}


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
GlueMapWindow::SetMapScale(const fixed x)
{
  MapWindow::SetMapScale(x);

  if (!SettingsMap().TargetPan) {
    if (GetDisplayMode() == dmCircling && SettingsMap().CircleZoom)
      // save cruise scale
      zoomclimb.ClimbScale = visible_projection.GetScale();
    else
      zoomclimb.CruiseScale = visible_projection.GetScale();

    SaveDisplayModeScales();
  }
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
  bool isclimb = (GetDisplayMode() == dmCircling);

  bool my_target_pan = SettingsMap().TargetPan;

  if (my_target_pan != zoomclimb.last_targetpan) {
    if (my_target_pan) {
      // save starting values
      if (isclimb)
        zoomclimb.ClimbScale = visible_projection.GetScale();
      else
        zoomclimb.CruiseScale = visible_projection.GetScale();
    } else {
      // restore scales
      if (isclimb)
        visible_projection.SetScale(zoomclimb.ClimbScale);
      else
        visible_projection.SetScale(zoomclimb.CruiseScale);
    }
    zoomclimb.last_targetpan = my_target_pan;
    return;
  }

  if (!my_target_pan && SettingsMap().CircleZoom) {
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

static DisplayMode_t
GetNewDisplayMode(const SETTINGS_MAP &settings_map,
                  const DERIVED_INFO &derived_info)
{
  if (settings_map.UserForceDisplayMode != dmNone)
    return settings_map.UserForceDisplayMode;
  else if (derived_info.Circling)
    return dmCircling;
  else if (derived_info.task_stats.flight_mode_final_glide)
    return dmFinalGlide;
  else
    return dmCruise;
}

void
GlueMapWindow::UpdateDisplayMode()
{
  DisplayMode_t new_mode = GetNewDisplayMode(SettingsMap(), Calculated());

  if (DisplayMode != new_mode && new_mode == dmCircling)
    offsetHistory.reset();

  DisplayMode = new_mode;
  SwitchZoomClimb();
}

void
GlueMapWindow::UpdateScreenAngle()
{
  const SETTINGS_MAP &settings = SettingsMap();

  if (settings.TargetPan &&
      Calculated().common_stats.active_taskpoint_index !=
          settings.TargetPanIndex) {
    visible_projection.SetScreenAngle(Angle::native(fixed_zero));
    return;
  }

  DisplayOrientation_t orientation =
      (GetDisplayMode() == dmCircling) ?
          settings.OrientationCircling : settings.OrientationCruise;

  if (orientation == TARGETUP &&
      Calculated().task_stats.current_leg.solution_remaining.defined())
    visible_projection.SetScreenAngle(Calculated().task_stats.current_leg.
                                      solution_remaining.Vector.Bearing);
  else if (orientation == NORTHUP)
    visible_projection.SetScreenAngle(Angle::native(fixed_zero));
  else
    // normal, glider forward
    visible_projection.SetScreenAngle(Basic().TrackBearing);

  settings_map.NorthArrow = (orientation != NORTHUP);
}

void
GlueMapWindow::UpdateMapScale()
{
  if (SettingsMap().TargetPan) {
    // set scale exactly so that waypoint distance is the zoom factor
    // across the screen
    fixed wpd = SettingsMap().TargetZoomDistance;
    wpd = max(fixed_int_constant(50), min(fixed_int_constant(160000), wpd / 4));
    visible_projection.SetFreeMapScale(wpd);
    return;
  }

  if (GetDisplayMode() == dmCircling && SettingsMap().CircleZoom)
    return;

  if (SettingsMap().EnablePan)
    return;

  fixed wpd = Calculated().AutoZoomDistance;
  if (SettingsMap().AutoZoom && positive(wpd)) {
    // Calculate distance percentage between plane symbol and map edge
    // 50: centered  100: at edge of map
    int AutoZoomFactor = (GetDisplayMode() == dmCircling) ?
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
  const SETTINGS_MAP &settings_map = SettingsMap();

  RasterPoint center;
  center.x = (rc.left + rc.right) / 2;
  center.y = (rc.top + rc.bottom) / 2;

  if (GetDisplayMode() == dmCircling || settings_map.EnablePan)
    visible_projection.SetScreenOrigin(center.x, center.y);
  else if (settings_map.OrientationCruise == NORTHUP) {
    RasterPoint offset = OffsetHistory::zeroPoint;
    if (settings_map.GliderScreenPosition != 50 &&
        settings_map.MapShiftBias != MAP_SHIFT_BIAS_NONE) {
      fixed x = fixed_zero;
      fixed y = fixed_zero;
      if (settings_map.MapShiftBias == MAP_SHIFT_BIAS_TRACK) {
        if (Basic().GroundSpeed > fixed_int_constant(8)) /* 8 m/s ~ 30 km/h */
          Basic().TrackBearing.Reciprocal().sin_cos(x, y);
      } else if (settings_map.MapShiftBias == MAP_SHIFT_BIAS_TARGET) {
        if (Calculated().task_stats.current_leg.solution_remaining.defined())
          Calculated().task_stats.current_leg.solution_remaining
                      .Vector.Bearing.Reciprocal().sin_cos(x, y);
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

  if (settings_map.EnablePan)
    SetLocation(settings_map.PanLocation);
  else if (GetDisplayMode() == dmCircling &&
           Calculated().thermal_locator.estimate_valid &&
           Calculated().thermal_locator.estimate_location.distance(Basic().Location)
                  < visible_projection.GetMapScale() * fixed_two)
    SetLocation(Calculated().thermal_locator.estimate_location);
  else
    // Pan is off
    SetLocation(Basic().Location);

  visible_projection.UpdateScreenBounds();
}
