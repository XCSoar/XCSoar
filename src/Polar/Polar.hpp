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

#include "Math/fixed.hpp"
#include <tchar.h>

class GlidePolar;
class TLineReader;

struct PolarCoefficients
{
  fixed a, b, c;
};

/**
 * Struct for internally stored WinPilot-like polars
 */
struct SimplePolar
{
  const TCHAR* name;   /**< Name of the glider type */
  double dry_mass;     /**< Mass dry gross (kg) */
  double max_ballast;  /**< Max water ballast (l) */
  double v1;           /**< Speed (kph) of point 1 */
  double w1;           /**< Sink rate (negative, m/s) of point 1  */
  double v2;           /**< Speed (kph) of point 2 */
  double w2;           /**< Sink rate (negative, m/s) of point 2  */
  double v3;           /**< Speed (kph) of point 3 */
  double w3;           /**< Sink rate (negative, m/s) of point 3  */
  double wing_area;    /**< Reference wing area (m^2) */
  double v_no;         /**< Maximum speed for normal operations */

  bool ReadString(const TCHAR* line);
  void GetString(TCHAR* line, size_t size) const;

  /**
   * Converts a WinPilot based SimplePolar to a XCSoar based GlidePolar
   * @param polar Reference to the GlidePolar to save the data to
   */
  bool CopyIntoGlidePolar(GlidePolar &polar) const;

  PolarCoefficients CalculateCoefficients() const;

  bool IsValid() const;
  static bool IsValid(const PolarCoefficients &coeff);

  void Init();
};

#endif
