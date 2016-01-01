/* Copyright_License {

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

#include "Computer/ThermalBase.hpp"
#include "TestUtil.hpp"
#include "Geo/SpeedVector.hpp"

int main(int argc, char **argv)
{
  plan_tests(3);

  GeoPoint location(Angle::Degrees(7), Angle::Degrees(45));
  double altitude(1300);
  double average(2.5);
  SpeedVector wind(Angle::Degrees(60), 20);

  GeoPoint ground_location(Angle::Zero(), Angle::Zero());
  double ground_alt;

  EstimateThermalBase(nullptr, location, altitude, average, wind,
                      ground_location, ground_alt);

  ok1(equals(ground_location.longitude.Degrees(), 7.114186));
  ok1(equals(ground_location.latitude.Degrees(), 45.046563));
  ok1(equals(ground_alt, 0));

  return exit_status();
}
