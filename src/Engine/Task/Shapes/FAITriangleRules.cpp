/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "FAITriangleRules.hpp"

bool
FAITriangleRules::TestDistances(const fixed d1, const fixed d2, const fixed d3)
{
  const fixed d_wp = d1 + d2 + d3;
  const fixed d_wp28 = d_wp * fixed(0.28);

  /*
   * A triangle is a valid FAI-triangle, if no side is less than
   * 28% of the total length (total length less than 750 km), or no
   * side is less than 25% or larger than 45% of the total length
   * (totallength >= 750km).
   */

  if (d_wp < fixed(750000) &&
      d1 >= d_wp28 && d2 >= d_wp28 && d3 >= d_wp28)
    // small FAI
    return true;

  const fixed d_wp25 = d_wp / 4;
  const fixed d_wp45 = d_wp * fixed(0.45);

  if (d_wp >= fixed(750000) &&
      d1 > d_wp25 && d2 > d_wp25 && d3 > d_wp25 &&
      d1 <= d_wp45 && d2 <= d_wp45 && d3 <= d_wp45)
    // large FAI
    return true;

  return false;
}
