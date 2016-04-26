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

#include "TeamCode.hpp"
#include "Geo/Math.hpp"
#include "Geo/GeoPoint.hpp"
#include "Math/Angle.hpp"
#include "Math/Util.hpp"

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

gcc_const
static unsigned
CountDigits(unsigned value)
{
  unsigned n = 1;
  while (value >= BASE) {
    value /= BASE;
    ++n;
  }

  return n;
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
  if (n_digits == 0)
    n_digits = CountDigits(value);

  TCHAR *p = code + n_digits;
  *p-- = _T('\0');

  do {
    unsigned digit_value = value % BASE;
    value /= BASE;

    *p = digit_value < 10
      ? TCHAR('0' + digit_value)
      : TCHAR('A' + digit_value - 10);
  } while (--p >= code);
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

double
TeamCode::GetRange() const
{
  // Get last three values from teamcode (3-5)
  unsigned value = GetValueFromTeamCode(code.begin() + 2, 3);
  return value * 100;
}

void
TeamCode::Update(Angle bearing, double range)
{
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
  auto bearing = GetBearing();
  auto distance = GetRange();

  return FindLatitudeLongitude(ref, bearing, distance);
}
