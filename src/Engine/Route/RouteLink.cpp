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

#include "RouteLink.hpp"
#include "RoutePolar.hpp"
#include "GlideSolvers/GlideState.hpp"
#include "GlideSolvers/GlideResult.hpp"
#include "GlideSolvers/MacCready.hpp"
#include "Geo/SpeedVector.hpp"
#include "Geo/Flat/TaskProjection.hpp"
#include "Math/FastMath.h"

#include <assert.h>
#include <limits.h>

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
static unsigned
XYToIndex(fixed x, fixed y)
{
  return AngleToIndex(Angle::FromXY(y, x));
}

RouteLink::RouteLink (const RouteLinkBase& _link, const TaskProjection &proj)
  :RouteLinkBase(_link)
{
  CalcSpeedups(proj);
}

RouteLink::RouteLink (const RoutePoint& _destination, const RoutePoint& _origin,
                      const TaskProjection &proj)
  :RouteLinkBase(_destination, _origin)
{
  CalcSpeedups(proj);
}

void
RouteLink::CalcSpeedups(const TaskProjection& proj)
{
  const fixed scale = proj.GetApproximateScale();
  const fixed dx = fixed(first.longitude - second.longitude);
  const fixed dy = fixed(first.latitude - second.latitude);
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
RouteLink::Flat() const
{
  RouteLink copy(*this);
  copy.second.altitude = copy.first.altitude;
  return copy;
}

#define ROUTE_MIN_STEP 3

bool
RouteLinkBase::IsShort() const
{
  return abs(first.longitude - second.longitude) < ROUTE_MIN_STEP &&
         abs(first.latitude - second.latitude) < ROUTE_MIN_STEP;
}
