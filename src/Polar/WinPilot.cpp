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

#include "Polar/WinPilot.hpp"
#include "Polar/Polar.hpp"
#include "IO/ConfiguredFile.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Sizes.h"

#include <math.h>
#include <stdlib.h>

/**
 * Converts a WinPilot based to a XCSoar based polar
 * @param polar The polar structure to convert
 * @param POLARV Speed1, Speed2 and Speed3
 * @param POLARW Sinkrate1, Sinkrate2 and Sinkrate3
 * @param ww dry mass, maximum takeoff weight
 */
void
PolarWinPilot2XCSoar(class Polar &polar,
                     double POLARV[3], double POLARW[3], double ww[2])
{
  double d;
  double v1,v2,v3;
  double w1,w2,w3;

  v1 = POLARV[0]/3.6; v2 = POLARV[1]/3.6; v3 = POLARV[2]/3.6;
  //	w1 = -POLARV[0]/POLARLD[0];
  //    w2 = -POLARV[1]/POLARLD[1];
  //    w3 = -POLARV[2]/POLARLD[2];
  w1 = POLARW[0]; w2 = POLARW[1]; w3 = POLARW[2];

  d = v1*v1*(v2-v3)+v2*v2*(v3-v1)+v3*v3*(v1-v2);
  if (d == 0.0)
    polar.POLAR[0] = 0;
  else
    polar.POLAR[0] = ((v2 - v3) * (w1 - w3) + (v3 - v1) * (w2 - w3)) / d;

  d = v2-v3;
  if (d == 0.0)
    polar.POLAR[1] = 0;
  else
    polar.POLAR[1] = (w2 - w3 - polar.POLAR[0] * (v2 * v2 - v3 * v3)) / d;

  polar.WEIGHTS[0] = 70; // Pilot weight
  polar.WEIGHTS[1] = ww[0] - polar.WEIGHTS[0]; // Glider empty weight
  polar.WEIGHTS[2] = ww[1]; // Ballast weight

  polar.POLAR[2] = w3 - polar.POLAR[0] * v3 * v3 - polar.POLAR[1] * v3;

  // now scale off weight
  polar.POLAR[0] = polar.POLAR[0] * sqrt(polar.WEIGHTS[0] + polar.WEIGHTS[1]);
  polar.POLAR[2] = polar.POLAR[2] / sqrt(polar.WEIGHTS[0] + polar.WEIGHTS[1]);

  polar.WingArea = 0.0;
}

static bool
ReadWinPilotPolar(Polar &polar, const TCHAR *line)
{
  if (line[0] == _T('*'))
    /* a comment */
    return false;

  TCHAR *p;
  double POLARV[3];
  double POLARW[3];
  double ww[2];

  ww[0] = _tcstod(line, &p);
  if (*p != _T(','))
    return false;

  ww[1] = _tcstod(p + 1, &p);
  if (*p != _T(','))
    return false;

  POLARV[0] = _tcstod(p + 1, &p);
  if (*p != _T(','))
    return false;

  POLARW[0] = _tcstod(p + 1, &p);
  if (*p != _T(','))
    return false;

  POLARV[1] = _tcstod(p + 1, &p);
  if (*p != _T(','))
    return false;

  POLARW[1] = _tcstod(p + 1, &p);
  if (*p != _T(','))
    return false;

  POLARV[2] = _tcstod(p + 1, &p);
  if (*p != _T(','))
    return false;

  POLARW[2] = _tcstod(p + 1, NULL);
  if (*p != _T(','))
    return false;

  PolarWinPilot2XCSoar(polar, POLARV, POLARW, ww);
  return true;
}

static bool
ReadWinPilotPolar(Polar &polar, TLineReader &reader)
{
  const TCHAR *line;
  while ((line = reader.read()) != NULL)
    if (ReadWinPilotPolar(polar, line))
      return true;

  return false;
}

// Example:
// *LS-3  WinPilot POLAR file: MassDryGross[kg], MaxWaterBallast[liters], Speed1[km/h], Sink1[m/s], Speed2, Sink2, Speed3, Sink3
// 403, 101, 115.03, -0.86, 174.04, -1.76, 212.72,  -3.4
/**
 * Reads the WinPilor polar file specified in the registry
 * @return True if parsing was successful, False otherwise
 */
bool
ReadWinPilotPolar(Polar &polar)
{
  TLineReader *reader = OpenConfiguredTextFile(szProfilePolarFile);
  if (reader == NULL)
    return false;

  bool success = ReadWinPilotPolar(polar, *reader);
  delete reader;

  return success;
}
