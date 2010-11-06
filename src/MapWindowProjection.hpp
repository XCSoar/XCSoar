/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#ifndef XCSOAR_MAPWINDOW_PROJECTION_HPP
#define XCSOAR_MAPWINDOW_PROJECTION_HPP

#include "WindowProjection.hpp"
#include "SettingsMap.hpp"
#include "Sizes.h"
#include "Math/fixed.hpp"
#include "Compiler.h"

struct NMEA_INFO;
struct DERIVED_INFO;
class Waypoint;

class MapWindowProjection:
  public WindowProjection
{
public:
  MapWindowProjection();

  DisplayMode_t GetDisplayMode() const {
    return DisplayMode;
  }

  void SetDisplayMode(DisplayMode_t _DisplayMode) {
    DisplayMode = _DisplayMode;
  }

protected:
  DisplayMode_t DisplayMode;

public:
  // scale/display stuff
  void Update(const RECT rc, const NMEA_INFO &basic,
              const SETTINGS_MAP &settings_map);

  void Initialize(const SETTINGS_MAP &settings, const RECT &rc);

  void RequestMapScaleUser(fixed x, const SETTINGS_MAP &setting_map);
  void RequestMapScale(fixed x, const SETTINGS_MAP &setting_map);

  bool IsOriginCentered(const DisplayOrientation_t orientation);

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
  fixed CalculateMapScaleUser(int scale) const;

  gcc_pure
  fixed StepMapScale(fixed scale, int Step) const;

  gcc_pure
  fixed StepMapScaleUser(fixed scale, int Step) const;

  gcc_pure
  bool WaypointInScaleFilter(const Waypoint &way_point) const;

private:
  fixed MapScale;

  fixed LimitMapScaleUser(fixed value, const SETTINGS_MAP &settings);

  fixed LimitMapScale(fixed value, const SETTINGS_MAP &settings);

  gcc_pure
  int FindMapScale(const fixed Value) const;

  gcc_pure
  int FindMapScaleUser(const fixed Value) const;

  fixed ScaleList[11];
  int ScaleListCount;
};

#endif
