// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RouteLink.hpp"
#include "RoutePolar.hpp"
#include "Geo/Flat/FlatProjection.hpp"

#include <cassert>
#include <stdlib.h>

[[gnu::const]]
static unsigned
AngleToIndex(Angle a) noexcept
{
  auto i = ROUTEPOLAR_POINTS * (1.25
                                - a.AsBearing() / Angle::FullCircle());
  assert(i > 0);
  return uround(i) % ROUTEPOLAR_POINTS;
}

[[gnu::const]]
static unsigned
XYToIndex(double x, double y) noexcept
{
  return AngleToIndex(Angle::FromXY(y, x));
}

RouteLink::RouteLink(const RouteLinkBase& _link,
                     const FlatProjection &proj) noexcept
  :RouteLinkBase(_link)
{
  CalcSpeedups(proj);
}

RouteLink::RouteLink(const RoutePoint& _destination, const RoutePoint& _origin,
                     const FlatProjection &proj) noexcept
  :RouteLinkBase(_destination, _origin)
{
  CalcSpeedups(proj);
}

void
RouteLink::CalcSpeedups(const FlatProjection &proj) noexcept
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

bool
RouteLinkBase::IsShort() const noexcept
{
  constexpr unsigned MIN_STEP = 3;
  const auto delta = GetDelta();
  return (unsigned)abs(delta.x) < MIN_STEP && (unsigned)abs(delta.y) < MIN_STEP;
}
