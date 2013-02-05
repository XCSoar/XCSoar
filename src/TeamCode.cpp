/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "TeamCode.hpp"
#include "Geo/Math.hpp"
#include "Geo/GeoPoint.hpp"
#include "Math/Angle.hpp"
#include "Util/StringUtil.hpp"
#include "Util/Macros.hpp"

#include <algorithm>

#include <string.h>

static constexpr unsigned BASE = 36;
static constexpr unsigned TEAMCODE_COMBINATIONS = BASE * BASE;

static constexpr Angle ANGLE_FACTOR =
  Angle::FullCircle() / TEAMCODE_COMBINATIONS;

/**
 * Decodes the TeamCode
 * @param code The teamcode (or part of it)
 * @param length Maximum chars to decode
 * @return The decoded value
 */
static unsigned
GetValueFromTeamCode(const TCHAR *code, unsigned length)
{
  unsigned val = 0;
  unsigned position = 0;

  while (code[position] != 0 && position < length) {
    unsigned cifferVal = 0;

    if (code[position] >= '0' && code[position] <= '9') {
      cifferVal = code[position] - '0';
    } else if (code[position] >= 'A' && code[position] <= 'Z') {
      cifferVal = 10 + code[position] - 'A';
    }

    val = val * BASE;
    val += cifferVal;

    ++position;
  }

  return val;
}

/**
 * Encodes a value to teamcode
 * @param value The value to encode
 * @param code The teamcode (pointer)
 * @param n_digits Number of chars for the teamcode
 */
static void
NumberToTeamCode(unsigned value, TCHAR *code, unsigned n_digits)
{
  unsigned max_digits = 0;
  int current_digit = 0;

  if (n_digits > 0) {
    max_digits = n_digits - 1;
    current_digit = max_digits;
  }

  unsigned rest = value;
  while (rest > 0 || current_digit >= 0) {
    unsigned digit_value = (unsigned)pow(BASE, current_digit);
    unsigned part_size = (unsigned)(rest / digit_value);
    unsigned part_value(part_size * digit_value);
    unsigned position = max_digits - current_digit;

    if (part_size < 10) {
      rest -= part_value;
      code[position] = (unsigned char)('0' + part_size);
      --current_digit;
    } else if (part_size < BASE) {
      rest -= part_value;
      code[position] = (unsigned char)('A' + part_size - 10);
      --current_digit;
    } else {
      ++current_digit;
      max_digits = current_digit;
    }
  }
}

/**
 * Converts a given bearing to the bearing part of the teamcode
 * @param bearing Bearing to the reference waypoint
 * @param code The teamcode (pointer)
 */
static void
ConvertBearingToTeamCode(const Angle bearing, TCHAR *code)
{
  const unsigned value = uround(bearing.AsBearing().Native()
                                / ANGLE_FACTOR.Native());
  NumberToTeamCode(value, code, 2);
}

Angle
TeamCode::GetBearing() const
{
  // Get the first two values from teamcode (1-2)
  unsigned value = GetValueFromTeamCode(code, 2);

  // Calculate bearing
  return (ANGLE_FACTOR * value).AsBearing();
}

fixed
TeamCode::GetRange() const
{
  // Get last three values from teamcode (3-5)
  unsigned value = GetValueFromTeamCode(code.begin() + 2, 3);
  return fixed(value * 100);
}

void
TeamCode::Update(Angle bearing, fixed range)
{
  // Clear teamcode
  std::fill(code.buffer(), code.buffer() + code.MAX_SIZE, _T('\0'));
  // Calculate bearing part of the teamcode
  ConvertBearingToTeamCode(bearing, code.buffer());
  // Calculate distance part of the teamcode
  NumberToTeamCode(uround(range / 100), code.buffer() + 2, 0);
}

void
TeamCode::Update(const TCHAR* _code)
{
  code = _code;
}

GeoPoint
TeamCode::GetLocation(const GeoPoint ref) const
{
  Angle bearing = GetBearing();
  fixed distance = GetRange();

  return FindLatitudeLongitude(ref, bearing, distance);
}
