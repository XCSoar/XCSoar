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

#include "Math/Earth.hpp"
#include "TestUtil.hpp"

static void
TestLinearDistance()
{
  const GeoPoint lon_start(Angle::Degrees(fixed(90)),
                           Angle::Degrees(fixed_zero));
  for (unsigned i = 0; i < 180; i += 5) {
    const GeoPoint lon_end(lon_start.longitude + Angle::Degrees(fixed(i)),
                           lon_start.latitude);
    fixed distance = Distance(lon_start, lon_end);

    double min = 111100 * i;
    double max = 111200 * i;

    ok1(between(distance, min, max));
  }

  const GeoPoint lat_start(Angle::Degrees(fixed_zero),
                           Angle::Degrees(fixed_zero));
  for (unsigned i = 0; i < 90; i += 5) {
    const GeoPoint lat_end(lat_start.longitude,
                           lat_start.latitude + Angle::Degrees(fixed(i)));
    fixed distance = Distance(lat_start, lat_end);

    double min = 111100 * i;
    double max = 111200 * i;

    ok1(between(distance, min, max));
  }
}

int main(int argc, char **argv)
{
  plan_tests(9 + 36 + 18);

  const GeoPoint a(Angle::Degrees(fixed(7.7061111111111114)),
                   Angle::Degrees(fixed(51.051944444444445)));
  const GeoPoint b(Angle::Degrees(fixed(7.599444444444444)),
                   Angle::Degrees(fixed(51.099444444444444)));
  const GeoPoint c(Angle::Degrees(fixed(4.599444444444444)),
                   Angle::Degrees(fixed(47.099444444444444)));

  fixed distance = Distance(a, b);
  ok1(distance > fixed(9130) && distance < fixed(9140));

  Angle bearing = Bearing(a, b);
  ok1(bearing.Degrees() > fixed(304));
  ok1(bearing.Degrees() < fixed(306));

  bearing = Bearing(b, a);
  ok1(bearing.Degrees() > fixed(124));
  ok1(bearing.Degrees() < fixed(126));

  distance = ProjectedDistance(a, b, a);
  ok1(is_zero(distance));
  distance = ProjectedDistance(a, b, b);
  ok1(distance > fixed(9120) && distance < fixed(9140));

  const GeoPoint middle(a.longitude.Fraction(b.longitude, fixed_half),
                        a.latitude.Fraction(b.latitude, fixed_half));
  distance = ProjectedDistance(a, b, middle);
  ok1(distance > fixed(9100/2) && distance < fixed(9140/2));

  fixed big_distance = Distance(a, c);
  ok1(big_distance > fixed(494000) && big_distance < fixed(495000));

  TestLinearDistance();

  return exit_status();
}
