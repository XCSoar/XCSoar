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

#include "ThermalBase.hpp"
#include "TestUtil.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Engine/Navigation/SpeedVector.hpp"

RasterTerrain *terrain;

int main(int argc, char **argv)
{
  plan_tests(3);

  GeoPoint location(Angle::degrees(fixed(7)), Angle::degrees(fixed(45)));
  fixed altitude(1300);
  fixed average(2.5);
  SpeedVector wind(Angle::degrees(fixed(60)), fixed(20));

  GeoPoint ground_location(Angle::zero(), Angle::zero());
  fixed ground_alt;

  EstimateThermalBase(location, altitude, average, wind,
                      ground_location, ground_alt);

  ok1(equals(ground_location.Longitude.value_degrees(), 7.114186));
  ok1(equals(ground_location.Latitude.value_degrees(), 45.046563));
  ok1(equals(ground_alt, 0));

  return exit_status();
}
