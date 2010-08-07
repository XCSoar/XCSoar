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
#if !defined(XCSOAR_MAPWINDOW_PROJECTION_HPP)
#define XCSOAR_MAPWINDOW_PROJECTION_HPP

#include "Projection.hpp"
#include "Units.hpp"
#include "SettingsUser.hpp"
#include "Sizes.h"
#include "Math/fixed.hpp"

struct NMEA_INFO;
struct DERIVED_INFO;
struct SETTINGS_COMPUTER;
class Waypoint;
class Canvas;


class MapWindowProjection: public Projection
{
 public:
  MapWindowProjection();

  fixed  GetScreenDistanceMeters(void) const;

  virtual fixed GetMapScaleUser() const;

  fixed GetMapScaleKM() const {
    return (fixed)Units::ToSysUnit(MapScale * fixed(0.001), Units::DistanceUnit);
  }
  POINT GetOrigAircraft(void) const {
    return Orig_Aircraft;
  }

  rectObj* getSmartBounds() {
    return &smart_bounds_active;
  }

  // called on receipt of new data, to trigger projection/scale change functions
  void ExchangeBlackboard(const DERIVED_INFO &derived_info,
                          const SETTINGS_MAP &settings_map);

  DisplayMode_t GetDisplayMode() const {
    return DisplayMode;
  }

 protected:
  DisplayMode_t DisplayMode;

  Angle DisplayAircraftAngle;

  // scale/display stuff
  void CalculateOrigin(const RECT rc,
                       const NMEA_INFO &nmea_info,
                       const DERIVED_INFO &derived_info,
                       const SETTINGS_COMPUTER &settings_computer,
                       const SETTINGS_MAP &settings_map);

  void InitialiseScaleList(const SETTINGS_MAP &settings);

  // 4 = x*30/1000
  fixed DistancePixelsToMeters(const int x) const {
    return x * MapScale / Units::ToUserUnit(fixed(GetMapResolutionFactor()),
                                            Units::DistanceUnit);
  }
  //
  fixed RequestDistancePixelsToMeters(const int x) const {
    return x * _RequestedMapScale / Units::ToUserUnit(fixed(GetMapResolutionFactor()),
                                                      Units::DistanceUnit);
  }
  fixed DistanceScreenToUser(const int x) const {
    return x * MapScale / GetMapResolutionFactor();
  }
  fixed RequestMapScale(fixed x, const SETTINGS_MAP &settings_map) {
    _RequestedMapScale = LimitMapScale(x, settings_map);
    return _RequestedMapScale;
  }
  fixed GetRequestedMapScale() const {
    return _RequestedMapScale;
  }
  bool IsOriginCentered() const {
    return _origin_centered;
  }

  bool SmartBounds(const bool force);
  fixed StepMapScale(int Step) const;

public:
  bool HaveScaleList() const {
    return ScaleListCount > 0;
  }
  fixed StepMapScale(int Step);

  bool WaypointInScaleFilter(const Waypoint &way_point) const;
  static bool WaypointInScaleFilter(const fixed scale,
                                    const Waypoint &way_point);

private:
  POINT Orig_Aircraft;

  fixed MapScale;
  fixed _RequestedMapScale;

  void ModifyMapScale(const SETTINGS_MAP &settings_map);

  void UpdateMapScale(const DERIVED_INFO &derived_info,
                      const SETTINGS_MAP &settings_map);

  void CalculateOrientationTargetPan(const NMEA_INFO &nmea_info,
                                     const DERIVED_INFO &derived_info,
                                     const SETTINGS_MAP &settings);
  void CalculateOrientationNormal(const NMEA_INFO &nmea_info,
                                  const DERIVED_INFO &derived_info,
                                  const SETTINGS_MAP &settings);

  bool _origin_centered;
  fixed LimitMapScale(fixed value, const SETTINGS_MAP &settings);
  fixed FindMapScale(const fixed Value);
  int ScaleCurrent;
  fixed ScaleList[SCALELISTSIZE];
  int ScaleListCount;

  rectObj smart_bounds_active;
  Angle smart_range_active;
};

#endif
