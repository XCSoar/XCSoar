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

#include "Geo/UTM.hpp"
#include "Geo/GeoPoint.hpp"

#include "TestUtil.hpp"

#include <cstdio>

int main(int argc, char **argv)
{
  plan_tests(5);

  GeoPoint p(Angle::Degrees(7), Angle::Degrees(50));
  UTM u = UTM::FromGeoPoint(p);

  ok1(u.zone_number == 32);
  ok1(u.zone_letter == 'U');
  ok1(equals(u.easting, 356670.9));
  ok1(equals(u.northing, 5540547.9));

  printf("%d%c E: %7f N: %7f\n", u.zone_number, u.zone_letter,
         (double)u.easting, (double)u.northing);

  p = u.ToGeoPoint();

  ok1(equals(p, 50, 7));

  printf("lon: %f - lat: %f\n", (double)p.longitude.Degrees(),
         (double)p.latitude.Degrees());

  return exit_status();
}
