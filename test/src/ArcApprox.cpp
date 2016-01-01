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

#include "OS/Args.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/Math.hpp"

#include <vector>

#include <stdio.h>

static void
AppendArc(std::vector<GeoPoint> &points, GeoPoint center,
          double radius, double _step)
{
  Angle start = Angle::Zero();
  Angle end = Angle::HalfCircle();

  // 5 or -5, depending on direction
  const Angle step = Angle::Degrees(_step);

  // Add first polygon point
  points.push_back(FindLatitudeLongitude(center, start, radius));

  // Add intermediate polygon points
  while ((end - start).AbsoluteDegrees() > _step * 3 / 2) {
    start = (start + step).AsBearing();
    points.push_back(FindLatitudeLongitude(center, start, radius));
  }

  // Add last polygon point
  points.push_back(FindLatitudeLongitude(center, end, radius));
}

static void
TestApproximation(double radius, double step)
{
  GeoPoint center(Angle::Degrees(7), Angle::Degrees(51));

  std::vector<GeoPoint> points;
  AppendArc(points, center, radius, step);

  printf("Number of points: %u\n\n", (unsigned)points.size());

  double max_error = 0;

  for (auto it = points.begin(), it_last = it++, it_end = points.end();
      it != it_end; it_last = it++) {
    for (double x = 0; x < 1; x += 0.1) {
      GeoPoint test_point = (*it_last).Interpolate(*it, x);
      auto distance = center.Distance(test_point);
      auto error = fabs(radius - distance);
      if (error > max_error)
        max_error = error;
    }
  }

  printf("Max. Error: %f m\n", (double)max_error);
}

int main(int argc, char **argv)
{
  Args args(argc, argv, "RADIUS [DEGREE STEPWIDTH = 5]");

  double radius = args.ExpectNextInt();
  double step = args.IsEmpty() ? 5 : args.ExpectNextDouble();

  printf("Airspace Arc Approximation\n\nRadius: %.0f m\nDegree Stepwith: %f deg\n\n",
         (double)radius, (double)step);

  TestApproximation(radius, step);

  return 0;
}
