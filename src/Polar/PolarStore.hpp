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

#ifndef XCSOAR_POLAR_BUILTIN_HPP
#define XCSOAR_POLAR_BUILTIN_HPP

#include <tchar.h>

struct PolarShape;
struct PolarInfo;

namespace PolarStore
{
  struct Item
  {
    const TCHAR* name;   /**< Name of the glider type */

    // Using doubles here to simplify the code in PolarStore.cpp

    /** Reference mass of the polar (kg) */
    double reference_mass;

    /** Max water ballast (l) */
    double max_ballast;

    /** Speed (kph) of point 1 */
    double v1;
    /** Sink rate (negative, m/s) of point 1  */
    double w1;
    /** Speed (kph) of point 2 */
    double v2;
    /** Sink rate (negative, m/s) of point 2  */
    double w2;
    /** Speed (kph) of point 3 */
    double v3;
    /** Sink rate (negative, m/s) of point 3  */
    double w3;

    /** Reference wing area (m^2), 0.0 if unknown */
    double wing_area;

    /** Maximum speed for normal operations (m/s), 0.0 if unknown */
    double v_no;

    /** Contest handicap, 0 if unknown */
    unsigned contest_handicap;

    PolarShape ToPolarShape() const;
    PolarInfo ToPolarInfo() const;
  };

  const Item &GetItem(unsigned i);
  unsigned Count();
}

#endif
