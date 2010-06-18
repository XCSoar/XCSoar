/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "MapWindowProjection.hpp"
#include "Math/FastRotation.hpp"
#include "Screen/Layout.hpp"
#include "SettingsComputer.hpp"
#include "Profile.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "Waypoint/Waypoint.hpp"

#include <stdlib.h>
#include <math.h>

// ratio of border size to trigger shape cache reload
static const fixed BORDERFACTOR(1.4);

MapWindowProjection::MapWindowProjection():
  Projection(),
  DisplayMode(dmCruise),
  DisplayAircraftAngle(),
  MapScale(fixed(5)),
  _RequestedMapScale(fixed(5)),
  _origin_centered(false),
  ScaleListCount (0),
  smart_range_active(fixed_one)
{
}


void
MapWindowProjection::InitialiseScaleList
(const SETTINGS_MAP &settings_map)
{
  ScaleListCount = Profile::GetScaleList(ScaleList,
                                         sizeof(ScaleList)/sizeof(ScaleList[0]));
  _RequestedMapScale = LimitMapScale(_RequestedMapScale, settings_map);
}

bool
MapWindowProjection::WaypointInScaleFilter(const Waypoint &way_point) const
{
  static const fixed fixed_10(10.0);
  static const fixed fixed_20(20.0);
  return way_point.is_landable()? 
    (MapScale <= fixed_20): 
    (MapScale <= fixed_10);
}

void
MapWindowProjection::CalculateOrientationNormal
(const NMEA_INFO &DrawInfo,
 const DERIVED_INFO &DerivedDrawInfo,
 const SETTINGS_MAP &settings)

{
  Angle trackbearing = DrawInfo.TrackBearing;

  if ((settings.DisplayOrientation == NORTHUP)
      || ((settings.DisplayOrientation == NORTHTRACK)
          && (DisplayMode != dmCircling))
      || (((settings.DisplayOrientation == NORTHCIRCLE)
           || (settings.DisplayOrientation == TRACKCIRCLE))
          && (DisplayMode == dmCircling))) {
    _origin_centered = true;

    if (settings.DisplayOrientation == TRACKCIRCLE) {
      DisplayAngle = DerivedDrawInfo.task_stats.current_leg.solution_remaining.Vector.Bearing;
      DisplayAircraftAngle = trackbearing - DisplayAngle.GetAngle();
    } else {
      DisplayAngle = Angle();
      DisplayAircraftAngle = trackbearing;
    }
  } else {
    // normal, glider forward
    _origin_centered = false;
    DisplayAngle = trackbearing;
    DisplayAircraftAngle = Angle();
  }

  DisplayAircraftAngle = DisplayAircraftAngle.as_bearing();
}

void
MapWindowProjection::CalculateOrientationTargetPan
(const NMEA_INFO &DrawInfo,
 const DERIVED_INFO &DerivedDrawInfo,
 const SETTINGS_MAP &settings)

{
  // Target pan mode, show track up when looking at current task point,
  // otherwise north up.  If circling, orient towards target.

#ifdef OLD_TASK // target control

  _origin_centered = true;
  if (((int)task.getActiveIndex()==settings.TargetPanIndex)
      &&(settings.DisplayOrientation != NORTHUP)
      &&(settings.DisplayOrientation != NORTHTRACK)
      )    {
    if (DisplayMode == dmCircling) {
      // target-up
      DisplayAngle = DerivedDrawInfo.WaypointBearing;
      DisplayAircraftAngle =
        DrawInfo.TrackBearing-DisplayAngle;
    } else {
      // track up
      DisplayAngle = DrawInfo.TrackBearing;
      DisplayAircraftAngle = 0.0;
    }
  } else {
    // North up
    DisplayAngle = 0.0;
    DisplayAircraftAngle = DrawInfo.TrackBearing;
  }
#else
  DisplayAngle.SetAngle(Angle());
  DisplayAircraftAngle = DrawInfo.TrackBearing;
#endif
}

void
MapWindowProjection::CalculateOrigin(const RECT rc, const NMEA_INFO &DrawInfo,
    const DERIVED_INFO &DerivedDrawInfo,
    const SETTINGS_COMPUTER &settings_computer,
    const SETTINGS_MAP &settings_map)
{

  if (settings_map.TargetPan)
    CalculateOrientationTargetPan(DrawInfo, DerivedDrawInfo, settings_map);
  else
    CalculateOrientationNormal(DrawInfo, DerivedDrawInfo, settings_map);

  if (_origin_centered || settings_map.EnablePan) {
    Orig_Screen.x = (rc.left + rc.right)/2;
    Orig_Screen.y = (rc.bottom + rc.top)/2;
  } else {
    Orig_Screen.x = (rc.left + rc.right)/2;
    Orig_Screen.y = ((rc.top - rc.bottom )*
		     settings_map.GliderScreenPosition/100)+rc.bottom;
  }

  if (settings_map.EnablePan)
    PanLocation = settings_map.PanLocation;
  else
    // Pan is off
    PanLocation = DrawInfo.Location;

  LonLat2Screen(DrawInfo.Location, Orig_Aircraft);

  UpdateScreenBounds();
}

fixed
MapWindowProjection::GetScreenDistanceMeters() const
{
  return DistancePixelsToMeters(max_dimension(MapRectBig));
}

fixed
MapWindowProjection::LimitMapScale(fixed value,
    const SETTINGS_MAP& settings_map)
{

  fixed minreasonable;

  minreasonable = 0.05;

  if (settings_map.AutoZoom && DisplayMode != dmCircling) {
#ifdef OLD_TASK // auto zoom 
    if (Calculated().common_stats.active_has_previous &&
        Calculated().common_stats.ordered_has_targets) {
      minreasonable = 0.88;
    } else {
      minreasonable = 0.44;
    }
#else
    minreasonable = 0.44;
#endif
  }

  value = max(minreasonable, min(fixed(160.0), value));
  if (ScaleListCount > 0)
    value = FindMapScale(value);

  return value;
}

fixed
MapWindowProjection::StepMapScale(int Step)
{
  if (abs(Step) >= 4)
    ScaleCurrent += Step / 4;
  else
    ScaleCurrent += Step;

  ScaleCurrent = max(0, min(ScaleListCount - 1, ScaleCurrent));
  return ((ScaleList[ScaleCurrent] * GetMapResolutionFactor())
         / (IBLSCALE(/*Appearance.DefaultMapWidth*/MapRect.right)));
}

fixed
MapWindowProjection::FindMapScale(const fixed Value)
{

  int i;
  fixed BestFit(99999);
  int BestFitIdx = -1;
  fixed DesiredScale = (Value * IBLSCALE(MapRect.right))
                     / GetMapResolutionFactor();

  for (i = 0; i < ScaleListCount; i++) {
    fixed err = fabs(DesiredScale - ScaleList[i]) / DesiredScale;
    if (err < BestFit) {
      BestFit = err;
      BestFitIdx = i;
    }
  }

  if (BestFitIdx != -1) {
    ScaleCurrent = BestFitIdx;
    return ((ScaleList[ScaleCurrent] * GetMapResolutionFactor())
           / IBLSCALE(MapRect.right-MapRect.left));
  }

  return Value;
}

void
MapWindowProjection::ModifyMapScale(const SETTINGS_MAP &settings_map)
{
  // limit zoomed in so doesn't reach silly levels
  _RequestedMapScale = LimitMapScale(_RequestedMapScale, settings_map);
  MapScale = _RequestedMapScale;

  SetScaleMetersToScreen(Units::ToUserUnit(fixed(GetMapResolutionFactor())
      / MapScale, Units::DistanceUnit));
}

void
MapWindowProjection::UpdateMapScale(const DERIVED_INFO &DerivedDrawInfo,
                                    const SETTINGS_MAP &settings_map)
{
  static int AutoMapScaleWaypointIndex = -1;
  static fixed StartingAutoMapScale(fixed_zero);
  fixed AutoZoomFactor;
  static DisplayMode_t DisplayModeLast = DisplayMode;

  // if there is user intervention in the scale
  if (settings_map.MapScale > 0) {
    fixed ext_mapscale = LimitMapScale(fixed(settings_map.MapScale),
        settings_map);
    if ((fabs(_RequestedMapScale - ext_mapscale) > fixed(0.05))
        && positive(ext_mapscale) && (DisplayMode == DisplayModeLast))
      _RequestedMapScale = ext_mapscale;
  }

  if(MapScale != _RequestedMapScale)
    ModifyMapScale(settings_map);

  DisplayModeLast = DisplayMode;

  fixed wpd;
  if (settings_map.TargetPan)
    wpd = settings_map.TargetZoomDistance;
  else
    wpd = DerivedDrawInfo.ZoomDistance;

  if (settings_map.TargetPan) {
    // set scale exactly so that waypoint distance is the zoom factor
    // across the screen
    _RequestedMapScale = LimitMapScale((fixed)Units::ToUserUnit(wpd / 4,
        Units::DistanceUnit), settings_map);
    ModifyMapScale(settings_map);
    return;
  }

  if (settings_map.AutoZoom && positive(wpd)) {
    if ((((settings_map.DisplayOrientation == NORTHTRACK)
          && (DisplayMode != dmCircling))
         || (settings_map.DisplayOrientation == NORTHUP)
         || (((settings_map.DisplayOrientation == NORTHCIRCLE)
             || (settings_map.DisplayOrientation == TRACKCIRCLE))
            && (DisplayMode == dmCircling)))
        && !settings_map.TargetPan) {
      AutoZoomFactor = 2.5;
    } else {
      AutoZoomFactor = 4;
    }

    if ((wpd < Units::ToSysUnit(AutoZoomFactor * MapScale, Units::DistanceUnit))
        || (StartingAutoMapScale == fixed_zero)) {
      // waypoint is too close, so zoom in
      // OR just turned waypoint

      // this is the first time this waypoint has gotten close,
      // so save original map scale

      if (StartingAutoMapScale == fixed_zero)
        StartingAutoMapScale = MapScale;

      // set scale exactly so that waypoint distance is the zoom factor
      // across the screen
      _RequestedMapScale = LimitMapScale((fixed)Units::ToUserUnit(wpd
          / AutoZoomFactor, Units::DistanceUnit), settings_map);
      ModifyMapScale(settings_map);
    }
  }

  // if there is an active waypoint
#ifdef OLD_TASK // auto zoom
  if (task.Valid()) {
    int task_index = task.getWaypointIndex();

    // if the current zoom focused waypoint has changed...
    if (AutoMapScaleWaypointIndex != task_index) {
      AutoMapScaleWaypointIndex = task_index;

      // zoom back out to where we were before
      if (StartingAutoMapScale> 0.0) {
	_RequestedMapScale = StartingAutoMapScale;
	ModifyMapScale(settings_map);
      }

      // reset search for new starting zoom level
      StartingAutoMapScale = 0.0;
    }
  } else {
    AutoMapScaleWaypointIndex = -1;
  }
#else
    AutoMapScaleWaypointIndex = -1;
#endif
}

void
MapWindowProjection::ExchangeBlackboard(const DERIVED_INFO &derived_info,
                                        const SETTINGS_MAP &settings_map)
{
  UpdateMapScale(derived_info, settings_map);
  // done here to avoid double latency due to locks
}

static const fixed MINRANGE(0.2);

static bool
RectangleIsInside(rectObj r_exterior, rectObj r_interior)
{
  if ((r_interior.minx >= r_exterior.minx) &&
      (r_interior.maxx <= r_exterior.maxx) &&
      (r_interior.miny >= r_exterior.miny) &&
      (r_interior.maxy <= r_exterior.maxy))
    return true;
  else
    return false;
}

bool
MapWindowProjection::SmartBounds(const bool force)
{
  const rectObj bounds_screen = CalculateScreenBounds(fixed_one);
  bool recompute = false;

  // only recalculate which shapes when bounds change significantly
  // need to have some trigger for this..

  // trigger if the border goes outside the stored area
  if (!RectangleIsInside(smart_bounds_active, bounds_screen))
    recompute = true;

  // also trigger if the scale has changed heaps
  const fixed range_real = fixed(max((bounds_screen.maxx - bounds_screen.minx),
                                     (bounds_screen.maxy - bounds_screen.miny)));
  const fixed range = max(fixed(MINRANGE), range_real);

  fixed scale = range / smart_range_active;
  if (max(scale, fixed_one / scale) > fixed(4))
    recompute = true;

  if (recompute || force) {
    // make bounds bigger than screen
    if (range_real < MINRANGE)
      scale = BORDERFACTOR * MINRANGE / range_real;
    else
      scale = BORDERFACTOR;

    smart_bounds_active = CalculateScreenBounds(scale);

    smart_range_active = max((smart_bounds_active.maxx
        - smart_bounds_active.minx), (smart_bounds_active.maxy
        - smart_bounds_active.miny));

    // now update visibility of objects in the map window
    return true;
  }

  return false;
}

fixed 
MapWindowProjection::GetMapScaleUser() const 
{
  return MapScale;
}
