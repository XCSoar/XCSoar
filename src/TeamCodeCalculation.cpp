/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "TeamCodeCalculation.h"
#include "Math/Constants.h"
#include "Engine/Math/Earth.hpp"

#include <math.h>
#include <string.h>

#define TEAMCODE_COMBINATIONS 1296

/**
 * Decodes the TeamCode
 * @param code The teamcode (or part of it)
 * @param maxCount Maximum chars to decode
 * @return The decoded value
 */
static int
GetValueFromTeamCode(const TCHAR *code, int maxCount)
{
  int val = 0;
  int charPos = 0;

  while (code[charPos] != 0 && charPos < maxCount) {
    int cifferVal = 0;

    if (code[charPos] >= '0' && code[charPos] <= '9') {
      cifferVal = (int)(code[charPos] - '0');
    } else if (code[charPos] >= 'A' && code[charPos] <= 'Z') {
      cifferVal = (int)(code[charPos] + -'A') + 10;
    }

    val = val * 36;
    val += (int)cifferVal;

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
NumberToTeamCode(double value, TCHAR *code, int minCiffers)
{
	int maxCif = 0;
	int curCif = 0;

	if (minCiffers > 0)	{
		maxCif = minCiffers - 1;
		curCif = maxCif;
	}

	double rest = value;
	while (rest > 0 || curCif >= 0) {
		int cifVal = (int)pow(36.0, curCif);
		int partSize = (int)(rest / cifVal);
		int partVal = partSize * cifVal;
		int txtPos = maxCif - curCif;

		if (partSize < 10) {
      rest -= partVal;
      code[txtPos] = (unsigned char)('0' + partSize);
      curCif--;
    } else if (partSize < 36) {
      rest -= partVal;
      code[txtPos] = (unsigned char)('A' + partSize - 10);
      curCif--;
    } else {
      curCif++;
      maxCif = curCif;
    }

		if (rest < 1)
      rest = 0;
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
  const double bamValue = bearing.as_bearing().value_degrees() / 360
                          * TEAMCODE_COMBINATIONS;
  NumberToTeamCode(bamValue, code, 2);
}

/**
 * Calculates the bearing from the given teamcode
 * @param code The teamcode
 * @return Bearing to the reference waypoint
 */
static Angle
GetBearing(const TCHAR *code)
{
  // Get the first two values from teamcode (1-2)
	int val = GetValueFromTeamCode(code, 2);

	// Calculate bearing
	return Angle::degrees(fixed(val * 360.0 / TEAMCODE_COMBINATIONS)).as_bearing();
}

/**
 * Calculates the distance from the given teamcode
 * @param code The teamcode
 * @return Distance to the reference waypoint
 */
static fixed
GetRange(const TCHAR *code)
{
  // Get last three values from teamcode (3-5)
	int val = GetValueFromTeamCode(&code[2], 3);
	return fixed(val * 100);
}

/**
 * Calculates distance and bearing to the teammate by decoding the given
 * teamcode and comparing the value with own bearing and distance to
 * the reference waypoint
 * @param ownBear Own bearing to the reference waypoint
 * @param ownDist Own distance to the reference waypoint
 * @param TeamMateCode The teamcode
 * @param bearToMate Bearing to the teammate (pointer)
 * @param distToMate Distance to the teammate (pointer)
 */
GeoPoint
GetTeamCodePosition(GeoPoint wpPos, const TCHAR *TeamCode)
{
  Angle bearing = GetBearing(TeamCode);
	fixed distance = GetRange(TeamCode);

	GeoPoint position;
  FindLatitudeLongitude(wpPos, bearing, distance, &position);
  return position;
}

/**
 * Calculates the teamcode of the given bearing and distance
 * @param code The teamcode (pointer)
 * @param bearing Bearing to the reference waypoint
 * @param range Distance to the reference waypoint
 */
void
GetTeamCode(TCHAR *code, Angle bearing, fixed range)
{
  // Clear teamcode
  memset(code, 0, sizeof(TCHAR) * 10);
  // Calculate bearing part of the teamcode
  ConvertBearingToTeamCode(bearing, code);
  // Calculate distance part of the teamcode
  NumberToTeamCode(range / 100, &code[2], 0);
}

void
TeamCode::Update(Angle bearing, fixed range)
{
  GetTeamCode(code, bearing, range);
}

void
TeamCode::Update(const TCHAR* _code)
{
  _tcsncpy(code, _code, 9);
  code[9] = 0;
}

const TCHAR*
TeamCode::GetCode() const
{
  return code;
}
