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

#ifndef XCSOAR_WIND_SETTINGS_HPP
#define XCSOAR_WIND_SETTINGS_HPP

#include "Geo/SpeedVector.hpp"
#include "NMEA/Validity.hpp"

#include <type_traits>

// control of calculations, these only changed by user interface
// but are used read-only by calculations

/** AutoWindMode (not in use) */
enum AutoWindModeBits
{
  /** 0: Manual */
  AUTOWIND_NONE = 0,
  /** 1: Circling */
  AUTOWIND_CIRCLING,
  /** 2: ZigZag */
  AUTOWIND_ZIGZAG,
  /** 3: Both */
};

/**
 * Wind calculator settings
 */
struct WindSettings {
  /**
   * Use the circling algorithm to calculate the wind?
   */
  bool circling_wind;

  /**
   * Use the EKF algorithm to calculate the wind? (formerly known as
   * "zig zag")
   */
  bool zig_zag_wind;

  bool external_wind;

  /**
   * This is the manual wind set by the pilot. Validity is set when
   * changeing manual wind but does not expire.
   */
  SpeedVector manual_wind;
  Validity manual_wind_available;

  void SetDefaults();

  bool IsAutoWindEnabled() const {
    return circling_wind || zig_zag_wind;
  }

  bool CirclingWindEnabled() const {
    return circling_wind;
  }

  bool ZigZagWindEnabled() const {
    return zig_zag_wind;
  }

  unsigned GetLegacyAutoWindMode() const {
    return (circling_wind ? 0x1 : 0x0) | (zig_zag_wind ? 0x2 : 0x0);
  }

  void SetLegacyAutoWindMode(unsigned mode) {
    circling_wind = (mode & 0x1) != 0;
    zig_zag_wind = (mode & 0x2) != 0;
  }
};

static_assert(std::is_trivial<WindSettings>::value, "type is not trivial");

#endif
