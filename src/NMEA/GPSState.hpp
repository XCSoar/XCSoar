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

#ifndef XCSOAR_GPS_STATE_HPP
#define XCSOAR_GPS_STATE_HPP

#include "NMEA/Validity.hpp"

#include <stdint.h>

enum class FixQuality: uint8_t {
  NO_FIX,
  GPS,
  DGPS,
  PPS,
  REAL_TIME_KINEMATIC,
  FLOAT_RTK,
  ESTIMATION,
  MANUAL_INPUT,
  SIMULATION,
};

/**
 * State of GPS fix
 */
struct GPSState
{
  static constexpr unsigned MAXSATELLITES = 12;

  //############
  //   Status
  //############

  /**
   * Fix quality
   */
  FixQuality fix_quality;
  Validity fix_quality_available;

  /**
   * Number of satellites used for gps fix.  -1 means "unknown".
   */
  int satellites_used;
  Validity satellites_used_available;

  /** GPS Satellite ids */
  int satellite_ids[MAXSATELLITES];
  Validity satellite_ids_available;

  /** Horizontal dilution of precision */
  double hdop;

  /**
   * Is the fix real? (no replay, no simulator)
   */
  bool real;

  /** Is XCSoar in replay mode? */
  bool replay;

  /**
   * Did the simulator provide the GPS position?
   */
  bool simulator;

#if defined(ANDROID) || defined(__APPLE__)
  /**
   * Was this fix obtained from an internal GPS device for which
   * link timeout detection must be disabled? This is the case on
   * Android, iOS and OS X. On these platforms we get notifications
   * from the OS when the GPS gets disconnected.
   */
  bool nonexpiring_internal_gps;
#endif

  void Reset();
  void Expire(double now);
};

#endif
