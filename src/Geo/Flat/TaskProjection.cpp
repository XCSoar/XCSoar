// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskProjection.hpp"

#include <algorithm>
#include <cassert>

TaskProjection::TaskProjection(const GeoBounds &_bounds)
  :bounds(_bounds)
{
  SetCenter(bounds.GetCenter());
}

void
TaskProjection::Reset(const GeoPoint &ref)
{
  FlatProjection::SetInvalid();
  bounds = GeoBounds(ref);
}

bool
TaskProjection::Update()
{
  assert(bounds.IsValid());

  GeoPoint old_center = GetCenter();
  GeoPoint new_center = bounds.GetCenter();
  if (new_center == old_center)
    return false;

  SetCenter(new_center);
  return true;
}

double
TaskProjection::ApproxRadius() const
{
  assert(bounds.IsValid());

  return std::max(GetCenter().DistanceS(bounds.GetSouthWest()),
                  GetCenter().DistanceS(bounds.GetNorthEast()));
}
