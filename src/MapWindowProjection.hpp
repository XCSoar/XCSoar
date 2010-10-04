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
#include "SettingsMap.hpp"
#include "Sizes.h"
#include "Math/fixed.hpp"
#include "Compiler.h"

struct NMEA_INFO;
struct DERIVED_INFO;
struct SETTINGS_COMPUTER;
class Waypoint;
class Canvas;


class MapWindowProjection: public Projection
{
 public:
  MapWindowProjection();

  gcc_pure
  fixed  GetScreenDistanceMeters(void) const;

  gcc_pure
  virtual fixed GetMapScaleUser() const;

  gcc_pure
  fixed GetMapScaleKM() const {
    return (fixed)Units::ToSysDistance(MapScale * fixed(0.001));
  }

  const POINT &GetOrigAircraft(void) const {
    return Orig_Aircraft;
  }

  // called on receipt of new data, to trigger projection/scale change functions
  void ExchangeBlackboard(const DERIVED_INFO &derived_info,
                          const SETTINGS_MAP &settings_map);

  DisplayMode_t GetDisplayMode() const {
    return DisplayMode;
  }

  void SetDisplayMode(DisplayMode_t _DisplayMode) {
    DisplayMode = _DisplayMode;
  }

  Angle GetDisplayAircraftAngle() const {
    return DisplayAircraftAngle;
  }

 protected:
  DisplayMode_t DisplayMode;

  Angle DisplayAircraftAngle;

public:
  // scale/display stuff
  void CalculateOrigin(const RECT rc,
                       const NMEA_INFO &nmea_info,
                       const DERIVED_INFO &derived_info,
                       const SETTINGS_COMPUTER &settings_computer,
                       const SETTINGS_MAP &settings_map);

  void InitialiseScaleList(const SETTINGS_MAP &settings, const RECT &rc);

  // 4 = x*30/1000
  gcc_pure
  fixed DistancePixelsToMeters(const int x) const {
    return x * MapScale / Units::ToUserUnit(fixed(GetMapResolutionFactor()),
                                            Units::DistanceUnit);
  }

  //
  gcc_pure
  fixed RequestDistancePixelsToMeters(const int x) const {
    return x * _RequestedMapScale / Units::ToUserUnit(fixed(GetMapResolutionFactor()),
                                                      Units::DistanceUnit);
  }

  gcc_pure
  fixed DistanceScreenToUser(const int x) const {
    return x * MapScale / GetMapResolutionFactor();
  }
  fixed RequestMapScale(fixed x, const SETTINGS_MAP &settings_map) {
    _RequestedMapScale = LimitMapScale(x, settings_map);
    return _RequestedMapScale;
  }

protected:
  fixed GetRequestedMapScale() const {
    return _RequestedMapScale;
  }
  bool IsOriginCentered() const {
    return _origin_centered;
  }

  fixed StepMapScale(int Step) const;

public:
  bool HaveScaleList() const {
    return ScaleListCount > 0;
  }

  /**
   * Calculates a scale index.
   */
  gcc_pure
  fixed CalculateMapScale(int scale) const;

  gcc_pure
  fixed StepMapScale(fixed scale, int Step) const;

  gcc_pure
  bool WaypointInScaleFilter(const Waypoint &way_point) const;

  static bool WaypointInScaleFilter(const fixed scale,
                                    const Waypoint &way_point);

private:
  POINT Orig_Aircraft;

  fixed MapScale;
  fixed _RequestedMapScale;

  /**
   * sets the MapScale based on the validated value
   * of _RequestedMapScale
   * @param settings_map
   */
  void ModifyMapScale(const SETTINGS_MAP &settings_map);

  /**
   * sets the map scale based on three possible inputs:
   * If TerrainPan is enabled, then uses that parameter
   * Else If settings_map.MapScale > 0, uses that value (which
   * will be reset back to zero during the blackboard copy)
   * Else If AutoZoom is enabled, scales to DerivedDrawInfo.ZoomDistance
   * @param derived_info
   * @param settings_map
   */
  void UpdateMapScale(const DERIVED_INFO &derived_info,
                      const SETTINGS_MAP &settings_map);
  /**
   * sets orientation for dlgTarget
   * if he next tp is the one being viewed by dlgTarget
   * then use normal orientation, else use NorthUp
   * @param nmea_info
   * @param derived_info
   * @param settings
   */
  void CalculateOrientationTargetPan(const NMEA_INFO &nmea_info,
                                     const DERIVED_INFO &derived_info,
                                     const SETTINGS_MAP &settings);
  void CalculateOrientationNormal(const NMEA_INFO &nmea_info,
                                  const DERIVED_INFO &derived_info,
                                  const SETTINGS_MAP &settings);

  bool _origin_centered;
  fixed LimitMapScale(fixed value, const SETTINGS_MAP &settings);

  gcc_pure
  int FindMapScale(const fixed Value) const;

  fixed ScaleList[SCALELISTSIZE];
  int ScaleListCount;
};

#endif
