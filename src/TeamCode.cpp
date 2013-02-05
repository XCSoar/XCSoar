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
 * @param maxCount Maximum chars to decode
 * @return The decoded value
 */
static unsigned
GetValueFromTeamCode(const TCHAR *code, unsigned maxCount)
{
  unsigned val = 0;
  unsigned charPos = 0;

  while (code[charPos] != 0 && charPos < maxCount) {
    unsigned cifferVal = 0;

    if (code[charPos] >= '0' && code[charPos] <= '9') {
      cifferVal = code[charPos] - '0';
    } else if (code[charPos] >= 'A' && code[charPos] <= 'Z') {
      cifferVal = 10 + code[charPos] - 'A';
    }

    val = val * BASE;
    val += cifferVal;

    charPos++;
  }

  return val;
}

/**
 * Encodes a value to teamcode
 * @param value The value to encode
 * @param code The teamcode (pointer)
 * @param minCiffers Number of chars for the teamcode
 */
static void
NumberToTeamCode(unsigned value, TCHAR *code, unsigned minCiffers)
{
  unsigned maxCif = 0;
  int curCif = 0;

  if (minCiffers > 0)	{
    maxCif = minCiffers - 1;
    curCif = maxCif;
  }

  unsigned rest = value;
  while (rest > 0 || curCif >= 0) {
    unsigned cifVal = (unsigned)pow(BASE, curCif);
    unsigned partSize = (unsigned)(rest / cifVal);
    unsigned partVal(partSize * cifVal);
    unsigned txtPos = maxCif - curCif;

    if (partSize < 10) {
      rest -= partVal;
      code[txtPos] = (unsigned char)('0' + partSize);
      curCif--;
    } else if (partSize < BASE) {
      rest -= partVal;
      code[txtPos] = (unsigned char)('A' + partSize - 10);
      curCif--;
    } else {
      curCif++;
      maxCif = curCif;
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
  const unsigned bamValue = unsigned(bearing.AsBearing().Native()
                                     / ANGLE_FACTOR.Native());
  NumberToTeamCode(bamValue, code, 2);
}

Angle
TeamCode::GetBearing() const
{
  // Get the first two values from teamcode (1-2)
  unsigned val = GetValueFromTeamCode(code, 2);

  // Calculate bearing
  return (ANGLE_FACTOR * val).AsBearing();
}

fixed
TeamCode::GetRange() const
{
  // Get last three values from teamcode (3-5)
  unsigned val = GetValueFromTeamCode(code.begin() + 2, 3);
  return fixed(val * 100);
}

void
TeamCode::Update(Angle bearing, fixed range)
{
  // Clear teamcode
  std::fill(code.buffer(), code.buffer() + code.MAX_SIZE, _T('\0'));
  // Calculate bearing part of the teamcode
  ConvertBearingToTeamCode(bearing, code.buffer());
  // Calculate distance part of the teamcode
  NumberToTeamCode(unsigned(range / 100), code.buffer() + 2, 0);
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
