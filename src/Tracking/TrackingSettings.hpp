/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_TRACKING_SETTINGS_HPP
#define XCSOAR_TRACKING_SETTINGS_HPP

#include "Tracking/Features.hpp"

#ifdef HAVE_TRACKING

#include "Tracking/SkyLines/Features.hpp"
#include "Tracking/SkyLines/Settings.hpp"

#include "Util/StaticString.hxx"

#ifdef HAVE_LIVETRACK24

struct LiveTrack24Settings {
  bool enabled;
  StaticString<64> server;
  StaticString<64> username;
  StaticString<64> password;

  void SetDefaults();
};

#endif

struct TrackingSettings {
#ifdef HAVE_LIVETRACK24
  enum class VehicleType {
    GLIDER = 0,
    PARAGLIDER = 1,
    POWERED_AIRCRAFT = 2,
    HOT_AIR_BALLOON = 3,
    HANGGLIDER_FLEX = 4,
    HANGGLIDER_RIGID = 5,
  };

  /** Minimum time between two position updates (in seconds) */
  unsigned interval;
  VehicleType vehicleType;
  StaticString<64> vehicle_name;
#endif

#ifdef HAVE_SKYLINES_TRACKING
  SkyLinesTracking::Settings skylines;
#endif

#ifdef HAVE_LIVETRACK24
  LiveTrack24Settings livetrack24;
#endif

  void SetDefaults();
};

#endif /* HAVE_TRACKING */
#endif
