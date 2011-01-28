/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Polar/WinPilot.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "IO/ConfiguredFile.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Math/fixed.hpp"

#include <stdlib.h>

/**
 * Converts a WinPilot based to a XCSoar based polar
 * @param polar The polar structure to convert
 * @param POLARV Speed1, Speed2 and Speed3
 * @param POLARW Sinkrate1, Sinkrate2 and Sinkrate3
 * @param ww dry mass, maximum takeoff weight
 */
void
ConvertWinPilotPolar(GlidePolar &polar, const SimplePolar &_polar)
{
  fixed d;
  fixed v1, v2, v3;
  fixed w1, w2, w3;

  v1 = fixed(_polar.v0 / 3.6);
  v2 = fixed(_polar.v1 / 3.6);
  v3 = fixed(_polar.v2 / 3.6);
  w1 = fixed(_polar.w0);
  w2 = fixed(_polar.w1);
  w3 = fixed(_polar.w2);

  d = v1 * v1 * (v2 - v3) + v2 * v2 * (v3 - v1) + v3 * v3 * (v1 - v2);
  if (d == fixed_zero)
    polar.ideal_polar_a = fixed_zero;
  else
    polar.ideal_polar_a = -((v2 - v3) * (w1 - w3) + (v3 - v1) * (w2 - w3)) / d;

  d = v2 - v3;
  if (d == fixed_zero)
    polar.ideal_polar_b = fixed_zero;
  else
    polar.ideal_polar_b =
        -(w2 - w3 + polar.ideal_polar_a * (v2 * v2 - v3 * v3)) / d;

  polar.ideal_polar_c =
      -(w3 + polar.ideal_polar_a * v3 * v3 + polar.ideal_polar_b * v3);

  // Glider empty weight
  polar.empty_mass = fixed(_polar.ww0);
  // Ballast weight
  polar.ballast_ratio = fixed(_polar.ww1) / polar.empty_mass;

  polar.wing_area = fixed(_polar.wing_area);

  polar.update();
}

static bool
ReadWinPilotPolarFileLine(GlidePolar &polar, const TCHAR *line)
{
  SimplePolar wp_polar;

  // Example:
  // *LS-3  WinPilot POLAR file: MassDryGross[kg], MaxWaterBallast[liters], Speed1[km/h], Sink1[m/s], Speed2, Sink2, Speed3, Sink3
  // 403, 101, 115.03, -0.86, 174.04, -1.76, 212.72,  -3.4

  if (line[0] == _T('*'))
    /* a comment */
    return false;

  TCHAR *p;

  wp_polar.ww0 = _tcstod(line, &p);
  if (*p != _T(','))
    return false;

  wp_polar.ww1 = _tcstod(p + 1, &p);
  if (*p != _T(','))
    return false;

  wp_polar.v0 = _tcstod(p + 1, &p);
  if (*p != _T(','))
    return false;

  wp_polar.w0 = _tcstod(p + 1, &p);
  if (*p != _T(','))
    return false;

  wp_polar.v1 = _tcstod(p + 1, &p);
  if (*p != _T(','))
    return false;

  wp_polar.w1 = _tcstod(p + 1, &p);
  if (*p != _T(','))
    return false;

  wp_polar.v2 = _tcstod(p + 1, &p);
  if (*p != _T(','))
    return false;

  wp_polar.w2 = _tcstod(p + 1, &p);

  if (*p != _T(','))
    wp_polar.wing_area = 0.0;
  else
    wp_polar.wing_area = _tcstod(p + 1, &p);

  ConvertWinPilotPolar(polar, wp_polar);
  return true;
}

static bool
ReadWinPilotPolarFile(GlidePolar &polar, TLineReader &reader)
{
  const TCHAR *line;
  while ((line = reader.read()) != NULL)
    if (ReadWinPilotPolarFileLine(polar, line))
      return true;

  return false;
}

/**
 * Reads the WinPilor polar file specified in the registry
 * @return True if parsing was successful, False otherwise
 */
bool
ReadWinPilotPolar(GlidePolar &polar)
{
  TLineReader *reader = OpenConfiguredTextFile(szProfilePolarFile);
  if (reader == NULL)
    return false;

  bool success = ReadWinPilotPolarFile(polar, *reader);
  delete reader;

  return success;
}
