/* Copyright_License {

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

#include "Math/Earth.hpp"
#include "TestUtil.hpp"

static void
TestLinearDistance()
{
  const GeoPoint lon_start(Angle::degrees(fixed(90)),
                           Angle::degrees(fixed_zero));
  for (unsigned i = 0; i < 180; i += 5) {
    const GeoPoint lon_end(lon_start.Longitude + Angle::degrees(fixed(i)),
                           lon_start.Latitude);
    fixed distance = Distance(lon_start, lon_end);

    double min = 111100 * i;
    double max = 111200 * i;

    ok1(between(distance, min, max));
  }

  const GeoPoint lat_start(Angle::degrees(fixed_zero),
                           Angle::degrees(fixed_zero));
  for (unsigned i = 0; i < 90; i += 5) {
    const GeoPoint lat_end(lat_start.Longitude,
                           lat_start.Latitude + Angle::degrees(fixed(i)));
    fixed distance = Distance(lat_start, lat_end);

    double min = 111100 * i;
    double max = 111200 * i;

    ok1(between(distance, min, max));
  }
}

int main(int argc, char **argv)
{
  plan_tests(9 + 36 + 18);

  const GeoPoint a(Angle::degrees(fixed(7.7061111111111114)),
                   Angle::degrees(fixed(51.051944444444445)));
  const GeoPoint b(Angle::degrees(fixed(7.599444444444444)),
                   Angle::degrees(fixed(51.099444444444444)));
  const GeoPoint c(Angle::degrees(fixed(4.599444444444444)),
                   Angle::degrees(fixed(47.099444444444444)));

  fixed distance = Distance(a, b);
  ok1(distance > fixed(9130) && distance < fixed(9140));

  Angle bearing = Bearing(a, b);
  ok1(bearing.value_degrees() > fixed(304));
  ok1(bearing.value_degrees() < fixed(306));

  bearing = Bearing(b, a);
  ok1(bearing.value_degrees() > fixed(124));
  ok1(bearing.value_degrees() < fixed(126));

  distance = ProjectedDistance(a, b, a);
  ok1(is_zero(distance));
  distance = ProjectedDistance(a, b, b);
  ok1(distance > fixed(9120) && distance < fixed(9140));

  const GeoPoint middle(a.Longitude.Fraction(b.Longitude, fixed_half),
                        a.Latitude.Fraction(b.Latitude, fixed_half));
  distance = ProjectedDistance(a, b, middle);
  ok1(distance > fixed(9100/2) && distance < fixed(9140/2));

  fixed big_distance = Distance(a, c);
  ok1(big_distance > fixed(494000) && big_distance < fixed(495000));

  TestLinearDistance();

  return exit_status();
}
