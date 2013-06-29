/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Geo/Math.hpp"
#include "TestUtil.hpp"

static void
TestLinearDistance()
{
  const GeoPoint lon_start(Angle::Degrees(90),
                           Angle::Zero());
  for (unsigned i = 0; i < 180; i += 5) {
    const GeoPoint lon_end(lon_start.longitude + Angle::Degrees(i),
                           lon_start.latitude);
    fixed distance = Distance(lon_start, lon_end);

#ifdef USE_WGS84
    double min = 111300 * i;
    double max = 111340 * i;
#else
    double min = 111100 * i;
    double max = 111200 * i;
#endif

    ok1(between(distance, min, max));
  }

#ifndef USE_WGS84
  /* Unfortunately this test doesn't make sense
   * on the earth ellipsoid...
   */
  const GeoPoint lat_start(Angle::Zero(),
                           Angle::Zero());
  for (unsigned i = 0; i < 90; i += 5) {
    const GeoPoint lat_end(lat_start.longitude,
                           lat_start.latitude + Angle::Degrees(i));
    fixed distance = Distance(lat_start, lat_end);

    double min = 111100 * i;
    double max = 111200 * i;

    ok1(between(distance, min, max));
  }
#endif

}

int main(int argc, char **argv)
{
#ifdef USE_WGS84
  plan_tests(9 + 36);
#else
  plan_tests(9 + 36 + 18);
#endif

  const GeoPoint a(Angle::Degrees(7.7061111111111114),
                   Angle::Degrees(51.051944444444445));
  const GeoPoint b(Angle::Degrees(7.599444444444444),
                   Angle::Degrees(51.099444444444444));
  const GeoPoint c(Angle::Degrees(4.599444444444444),
                   Angle::Degrees(47.099444444444444));

  fixed distance = Distance(a, b);
#ifdef USE_WGS84
  ok1(distance > fixed(9150) && distance < fixed(9160));
#else
  ok1(distance > fixed(9130) && distance < fixed(9140));
#endif

  Angle bearing = Bearing(a, b);
  ok1(bearing.Degrees() > fixed(304));
  ok1(bearing.Degrees() < fixed(306));

  bearing = Bearing(b, a);
  ok1(bearing.Degrees() > fixed(124));
  ok1(bearing.Degrees() < fixed(126));

  distance = ProjectedDistance(a, b, a);
  ok1(is_zero(distance));
  distance = ProjectedDistance(a, b, b);

#ifdef USE_WGS84
  ok1(distance > fixed(9150) && distance < fixed(9180));
#else
  ok1(distance > fixed(9120) && distance < fixed(9140));
#endif

  const GeoPoint middle(a.longitude.Fraction(b.longitude, fixed(0.5)),
                        a.latitude.Fraction(b.latitude, fixed(0.5)));
  distance = ProjectedDistance(a, b, middle);
#ifdef USE_WGS84
  ok1(distance > fixed(9150/2) && distance < fixed(9180/2));
#else
  ok1(distance > fixed(9100/2) && distance < fixed(9140/2));
#endif

  fixed big_distance = Distance(a, c);
  ok1(big_distance > fixed(494000) && big_distance < fixed(495000));

  TestLinearDistance();

  return exit_status();
}
