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

#ifndef XCSOAR_FLARM_STATUS_HPP
#define XCSOAR_FLARM_STATUS_HPP

#include "FLARM/Traffic.hpp"
#include "NMEA/Validity.hpp"

#include <type_traits>

/**
 * The FLARM operation status read from the PFLAU sentence.
 */
struct FlarmStatus {
  enum class GPSStatus: uint8_t {
    NONE = 0,
    GPS_2D = 1,
    GPS_3D = 2,
  };

  /** Number of received FLARM devices */
  unsigned short rx;
  /** Transmit status */
  bool tx;

  /** GPS status */
  GPSStatus gps;

  /** Alarm level of FLARM (0-3) */
  FlarmTraffic::AlarmType alarm_level;

  /** Is FLARM information available? */
  Validity available;

  void Clear() {
    available.Clear();
  }

  void Complement(const FlarmStatus &add) {
    if (!available && add.available)
      *this = add;
  }

  void Expire(double clock) {
    available.Expire(clock, 10);
  }
};

static_assert(std::is_trivial<FlarmStatus>::value, "type is not trivial");

#endif
