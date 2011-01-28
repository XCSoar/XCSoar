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
SimplePolar::ConvertToGlidePolar(GlidePolar &polar) const
{
  fixed d;
  fixed V1, V2, V3;
  fixed W1, W2, W3;

  V1 = fixed(v0 / 3.6);
  V2 = fixed(v1 / 3.6);
  V3 = fixed(v2 / 3.6);
  W1 = fixed(w0);
  W2 = fixed(w1);
  W3 = fixed(w2);

  d = V1 * V1 * (V2 - V3) + V2 * V2 * (V3 - V1) + V3 * V3 * (V1 - V2);
  if (d == fixed_zero)
    polar.ideal_polar_a = fixed_zero;
  else
    polar.ideal_polar_a = -((V2 - V3) * (W1 - W3) + (V3 - V1) * (W2 - W3)) / d;

  d = V2 - V3;
  if (d == fixed_zero)
    polar.ideal_polar_b = fixed_zero;
  else
    polar.ideal_polar_b =
        -(W2 - W3 + polar.ideal_polar_a * (V2 * V2 - V3 * V3)) / d;

  polar.ideal_polar_c =
      -(W3 + polar.ideal_polar_a * V3 * V3 + polar.ideal_polar_b * V3);

  // Glider empty weight
  polar.dry_mass = fixed(dry_mass);
  // Ballast weight
  polar.ballast_ratio = fixed(max_ballast) / polar.dry_mass;

  polar.wing_area = fixed(wing_area);

  polar.update();
}

bool
SimplePolar::ReadString(const TCHAR *line)
{
  // Example:
  // *LS-3  WinPilot POLAR file: MassDryGross[kg], MaxWaterBallast[liters], Speed1[km/h], Sink1[m/s], Speed2, Sink2, Speed3, Sink3
  // 403, 101, 115.03, -0.86, 174.04, -1.76, 212.72,  -3.4

  if (line[0] == _T('*'))
    /* a comment */
    return false;

  TCHAR *p;

  dry_mass = _tcstod(line, &p);
  if (*p != _T(','))
    return false;

  max_ballast = _tcstod(p + 1, &p);
  if (*p != _T(','))
    return false;

  v0 = _tcstod(p + 1, &p);
  if (*p != _T(','))
    return false;

  w0 = _tcstod(p + 1, &p);
  if (*p != _T(','))
    return false;

  v1 = _tcstod(p + 1, &p);
  if (*p != _T(','))
    return false;

  w1 = _tcstod(p + 1, &p);
  if (*p != _T(','))
    return false;

  v2 = _tcstod(p + 1, &p);
  if (*p != _T(','))
    return false;

  w2 = _tcstod(p + 1, &p);

  if (*p != _T(','))
    wing_area = 0.0;
  else
    wing_area = _tcstod(p + 1, &p);

  return true;
}

bool
SimplePolar::ReadFile(TLineReader &reader)
{
  const TCHAR *line;
  while ((line = reader.read()) != NULL)
    if (ReadString(line))
      return true;

  return false;
}

/**
 * Reads the WinPilor polar file specified in the registry
 * @return True if parsing was successful, False otherwise
 */
bool
SimplePolar::ReadFileFromProfile()
{
  TLineReader *reader = OpenConfiguredTextFile(szProfilePolarFile);
  if (reader == NULL)
    return false;

  bool success = ReadFile(*reader);
  delete reader;

  return success;
}
