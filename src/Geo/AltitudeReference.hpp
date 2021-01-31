/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_ALTITUDE_REFERENCE_HPP
#define XCSOAR_ALTITUDE_REFERENCE_HPP

#include <cstdint>

/**
 * This enum specifies the reference for altitude specifications.
 */
enum class AltitudeReference : int8_t {
  /**
   * No reference set, the altitude value is invalid.
   */
  NONE = -1,

  /**
   * Altitude is measured above ground level (AGL).
   *
   * Note: the integer value is important because it is stored in the
   * profile.
   */
  AGL = 0,

  /**
   * Altitude is measured above mean sea level (MSL).
   *
   * Note: the integer value is important because it is stored in the
   * profile.
   */
  MSL = 1,

  /**
   * Altitude is measured above the standard pressure (1013.25 hPa).
   * This is used for flight levels (FL).
   */
  STD,
};

#endif
