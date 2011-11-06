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

#ifndef XCSOAR_MAP_LOOK_HPP
#define XCSOAR_MAP_LOOK_HPP

#include "WaypointLook.hpp"
#include "AirspaceLook.hpp"
#include "AircraftLook.hpp"
#include "TaskLook.hpp"
#include "MarkerLook.hpp"
#include "TrailLook.hpp"
#include "Screen/Icon.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Pen.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Features.hpp"

struct SETTINGS_MAP;

struct MapLook {
  WaypointLook waypoint;
  AirspaceLook airspace;
  AircraftLook aircraft;
  TaskLook task;
  MarkerLook marker;
  TrailLook trail;

#ifdef HAVE_HATCHED_BRUSH
  Bitmap hAboveTerrainBitmap;
  Brush hAboveTerrainBrush;
#endif

  MaskedIcon hTerrainWarning;

  Pen hpWind, hpWindTail;
  Brush hbWind;

  Pen hpCompass;
  Brush hbCompass;

  Pen hpTerrainLine;
  Pen hpTerrainLineThick;

  Pen hpTrackBearingLine;

  Pen ContestPen[3];

  MaskedIcon hBmpThermalSource;

  MaskedIcon hBmpTrafficSafe;
  MaskedIcon hBmpTrafficWarning;
  MaskedIcon hBmpTrafficAlarm;

  MaskedIcon hBmpMapScaleLeft;
  MaskedIcon hBmpMapScaleRight;

  void Initialise(const struct SETTINGS_MAP &settings);
};

#endif
