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

#ifndef XCSOAR_POLAR_WINPILOT_HPP
#define XCSOAR_POLAR_WINPILOT_HPP

#include <tchar.h>

class GlidePolar;

/**
 * Struct for internally stored WinPilot-like polars
 */
struct SimplePolar
{
  const TCHAR *name;   /**< Name of the glider type */
  double ww0;          /**< Mass dry gross (kg) */
  double ww1;          /**< Max water ballast (l) */
  double v0;           /**< Speed (kph) of point 1 */
  double w0;           /**< Sink rate (negative, m/s) of point 1  */
  double v1;           /**< Speed (kph) of point 2 */
  double w1;           /**< Sink rate (negative, m/s) of point 2  */
  double v2;           /**< Speed (kph) of point 3 */
  double w2;           /**< Sink rate (negative, m/s) of point 3  */
  double wing_area;    /**< Reference wing area (m^2) */
};

void
ConvertWinPilotPolar(GlidePolar &polar, const SimplePolar &_polar);

bool
ReadWinPilotPolar(SimplePolar &polar);

#endif
