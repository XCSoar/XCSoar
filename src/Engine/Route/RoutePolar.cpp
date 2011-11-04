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

#include "RoutePolar.hpp"
#include "GlideSolvers/GlideState.hpp"
#include "GlideSolvers/GlideResult.hpp"
#include "GlideSolvers/MacCready.hpp"
#include "Navigation/SpeedVector.hpp"
#include "Navigation/TaskProjection.hpp"
#include "Math/FastMath.h"
#include <assert.h>
#include <limits.h>

gcc_const
static Angle
IndexToAngle(unsigned i)
{
  assert(i < ROUTEPOLAR_POINTS);

  return Angle::Radians(fixed_half_pi - i * fixed_two_pi / ROUTEPOLAR_POINTS);
}

gcc_const
static unsigned
AngleToIndex(Angle a)
{
  fixed i = ROUTEPOLAR_POINTS * (fixed(1.25)
                                 - a.AsBearing().Radians() / fixed_two_pi);
  assert(positive(i));
  return uround(i) % ROUTEPOLAR_POINTS;
}

gcc_const
static Angle
XYToAngle(fixed x, fixed y)
{
  return Angle::Radians(atan2(x, y));
}

gcc_const
static unsigned
XYToIndex(fixed x, fixed y)
{
  return AngleToIndex(XYToAngle(x, y));
}

GlideResult
RoutePolar::solve_task(const GlidePolar& glide_polar,
                       const SpeedVector& wind,
                       const Angle theta, const bool glide) const
{
  fixed altitude = glide? fixed(1.0e5): fixed_zero;
  GlideState task(GeoVector(fixed(1.0), theta), fixed_zero, altitude, wind);
  return MacCready::solve(glide_polar, task);
}

void
RoutePolar::initialise(const GlidePolar& polar, const SpeedVector& wind,
                       const bool is_glide)
{
  for (unsigned i = 0; i < ROUTEPOLAR_POINTS; ++i) {
    const Angle ang = IndexToAngle(i);
    GlideResult res = solve_task(polar, wind, ang, is_glide);
    RoutePolarPoint point(res.time_elapsed, res.height_glide);
    if (res.validity != GlideResult::RESULT_OK)
      point.valid = false;
    points[i] = point;
  }
}

void
RoutePolar::index_to_dxdy(const int index, int& dx, int& dy)
{
  static gcc_constexpr_data int sx[ROUTEPOLAR_POINTS]= {
    128, 126, 123, 118, 111, 102, 91, 79, 66, 51, 36, 20, 4, -12, -28, -44,
    -59, -73, -86, -97, -107, -115, -121, -125, -127, -127, -125, -121, -115,
    -107, -97, -86, -73, -59, -44, -28, -12, 4, 20, 36, 51, 66, 79, 91, 102,
    111, 118, 123, 126,
  };
  static gcc_constexpr_data int sy[ROUTEPOLAR_POINTS]= {
    0, 16, 32, 48, 62, 76, 89, 100, 109, 117, 122, 126, 127, 127, 124, 120,
    113, 104, 94, 82, 69, 55, 40, 24, 8, -8, -24, -40, -55, -69, -82, -94,
    -104, -113, -120, -124, -127, -127, -126, -122, -117, -109, -100, -89,
    -76, -62, -48, -32, -16,
  };

  dx = sx[index];
  dy = sy[index];
}

RouteLink::RouteLink (const RouteLinkBase& _link, const TaskProjection &proj)
  :RouteLinkBase(_link)
{
  calc_speedups(proj);
}

RouteLink::RouteLink (const RoutePoint& _destination, const RoutePoint& _origin,
                      const TaskProjection &proj)
  :RouteLinkBase(_destination, _origin)
{
  calc_speedups(proj);
}

void
RouteLink::calc_speedups(const TaskProjection& proj)
{
  const fixed scale = proj.get_approx_scale();
  const fixed dx = fixed(first.Longitude - second.Longitude);
  const fixed dy = fixed(first.Latitude - second.Latitude);
  if (!positive(fabs(dx)) && !positive(fabs(dy))) {
    d = fixed_zero;
    inv_d = fixed_zero;
    polar_index = 0;
    return;
  }
  mag_rmag(dx, dy, d, inv_d);
  polar_index = XYToIndex(dx, dy);
  d *= scale;
  inv_d /= scale;
}

RouteLink
RouteLink::flat() const
{
  RouteLink copy(*this);
  copy.second.altitude = copy.first.altitude;
  return copy;
}

#define ROUTE_MIN_STEP 3

bool
RouteLinkBase::is_short() const
{
  return abs(first.Longitude - second.Longitude) < ROUTE_MIN_STEP &&
         abs(first.Latitude - second.Latitude) < ROUTE_MIN_STEP;
}
