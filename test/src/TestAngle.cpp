/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Math/Angle.hpp"
#include "TestUtil.hpp"

#include <stdio.h>

int main(int argc, char **argv)
{
  plan_tests(38);

  // Test magnitude_degrees()
  ok1(equals(Angle::degrees(fixed_90).magnitude_degrees(), 90));
  ok1(equals(Angle::degrees(-fixed_90).magnitude_degrees(), 90));

  // Test Reciprocal()
  ok1(equals(Angle::degrees(fixed_90).Reciprocal(), 270));
  ok1(equals(Angle::degrees(fixed_270).Reciprocal(), 90));

  // Test as_bearing()
  ok1(equals(Angle::degrees(fixed(361)).as_bearing(), 1));
  ok1(equals(Angle::degrees(fixed(180)).as_bearing(), 180));
  ok1(equals(Angle::degrees(fixed(-180)).as_bearing(), 180));
  ok1(equals(Angle::degrees(fixed(-270)).as_bearing(), 90));

  // Test as_delta()
  ok1(equals(Angle::degrees(fixed_90).as_delta(), 90));
  ok1(equals(Angle::degrees(fixed(179)).as_delta(), 179));
  ok1(equals(Angle::degrees(fixed(-179)).as_delta(), -179));
  ok1(equals(Angle::degrees(fixed_270).as_delta(), -90));

  // Test between()
  ok1(Angle::degrees(fixed_90).between(Angle::degrees(fixed_zero),
                                       Angle::degrees(fixed_180)));

  ok1(!Angle::degrees(fixed_90).between(Angle::degrees(fixed_180),
                                        Angle::degrees(fixed_zero)));

  ok1(Angle::degrees(fixed_zero).between(Angle::degrees(fixed_270),
                                         Angle::degrees(fixed_90)));

  // Test sin()
  ok1(equals(Angle::degrees(fixed_zero).sin(), 0));
  ok1(equals(Angle::degrees(fixed_90).sin(), 1));
  ok1(equals(Angle::degrees(fixed_180).sin(), 0));
  ok1(equals(Angle::degrees(fixed_270).sin(), -1));
  ok1(equals(Angle::degrees(fixed_360).sin(), 0));

  // Test cos()
  ok1(equals(Angle::degrees(fixed_zero).cos(), 1));
  ok1(equals(Angle::degrees(fixed_90).cos(), 0));
  ok1(equals(Angle::degrees(fixed_180).cos(), -1));
  ok1(equals(Angle::degrees(fixed_270).cos(), 0));
  ok1(equals(Angle::degrees(fixed_360).cos(), 1));

  // Test fastsine()
  ok1(equals(Angle::degrees(fixed_zero).fastsine(), 0));
  ok1(equals(Angle::degrees(fixed_90).fastsine(), 1));
  ok1(equals(Angle::degrees(fixed_180).fastsine(), 0));
  ok1(equals(Angle::degrees(fixed_270).fastsine(), -1));
  ok1(equals(Angle::degrees(fixed_360).fastsine(), 0));

  // Test fastcosine()
  ok1(equals(Angle::degrees(fixed_zero).fastcosine(), 1));
  ok1(equals(Angle::degrees(fixed_90).fastcosine(), 0));
  ok1(equals(Angle::degrees(fixed_180).fastcosine(), -1));
  ok1(equals(Angle::degrees(fixed_270).fastcosine(), 0));
  ok1(equals(Angle::degrees(fixed_360).fastcosine(), 1));

  // Test BiSector()
  ok1(equals(Angle::degrees(fixed_90).BiSector(Angle::degrees(fixed_180)), 45));
  ok1(equals(Angle::degrees(fixed_270).BiSector(Angle::degrees(fixed_zero)), 225));
  ok1(equals(Angle::degrees(fixed_270).BiSector(Angle::degrees(fixed_180)), 315));

  return exit_status();
}
