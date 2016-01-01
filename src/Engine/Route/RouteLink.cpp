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

#include "RouteLink.hpp"
#include "RoutePolar.hpp"
#include "Geo/Flat/FlatProjection.hpp"

#include <assert.h>
#include <stdlib.h>

gcc_const
static unsigned
AngleToIndex(Angle a)
{
  auto i = ROUTEPOLAR_POINTS * (1.25
                                - a.AsBearing() / Angle::FullCircle());
  assert(i > 0);
  return uround(i) % ROUTEPOLAR_POINTS;
}

gcc_const
static unsigned
XYToIndex(double x, double y)
{
  return AngleToIndex(Angle::FromXY(y, x));
}

RouteLink::RouteLink (const RouteLinkBase& _link, const FlatProjection &proj)
  :RouteLinkBase(_link)
{
  CalcSpeedups(proj);
}

RouteLink::RouteLink (const RoutePoint& _destination, const RoutePoint& _origin,
                      const FlatProjection &proj)
  :RouteLinkBase(_destination, _origin)
{
  CalcSpeedups(proj);
}

void
RouteLink::CalcSpeedups(const FlatProjection &proj)
{
  const auto scale = proj.GetApproximateScale();
  const auto dx = first.x - second.x;
  const auto dy = first.y - second.y;
  if (dx == 0 && dy == 0) {
    d = 0;
    inv_d = 0;
    polar_index = 0;
    return;
  }

  polar_index = XYToIndex(dx, dy);
  d = hypot(dx, dy) * scale;
  inv_d = 1. / d;
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
  return abs(first.x - second.x) < ROUTE_MIN_STEP &&
         abs(first.y - second.y) < ROUTE_MIN_STEP;
}
