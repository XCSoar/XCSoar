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

#ifndef XCSOAR_UNITS_UNIT_HPP
#define XCSOAR_UNITS_UNIT_HPP

#include <stdint.h>

enum class Unit: uint8_t {
  UNDEFINED,
  KILOMETER,
  NAUTICAL_MILES,
  STATUTE_MILES,
  KILOMETER_PER_HOUR,
  KNOTS,
  STATUTE_MILES_PER_HOUR,
  METER_PER_SECOND,
  FEET_PER_MINUTE,
  METER,
  FEET,
  FLIGHT_LEVEL,
  KELVIN,
  DEGREES_CELCIUS, // K = C° + 273,15
  DEGREES_FAHRENHEIT, // K = (°F + 459,67) / 1,8
  HECTOPASCAL,
  MILLIBAR,
  TORR,
  INCH_MERCURY,
  KG_PER_M2,
  LB_PER_FT2,
  KG,
  LB,
  PERCENT,
  GRADIENT,
  VOLT,
  /**
   * The sentinel: the number of units in this enum.
   */
  COUNT
};

#endif
