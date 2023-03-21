// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UnorderedTaskPoint.hpp"
#include "Task/TaskBehaviour.hpp"
#include "Geo/GeoVector.hpp"

UnorderedTaskPoint::UnorderedTaskPoint(WaypointPtr wp,
                                       const TaskBehaviour &tb)
  :TaskWaypoint(TaskPointType::UNORDERED, std::move(wp)),
   safety_height_arrival(tb.safety_height_arrival) {}

void
UnorderedTaskPoint::SetTaskBehaviour(const TaskBehaviour &tb)
{
  safety_height_arrival = tb.safety_height_arrival;
}

GeoVector
UnorderedTaskPoint::GetVectorRemaining(const GeoPoint &reference) const noexcept
{
  if (!reference.IsValid())
    return GeoVector::Invalid();

  return GeoVector(reference, GetLocationRemaining());
}

double
UnorderedTaskPoint::GetElevation() const noexcept
{
  return GetBaseElevation() + safety_height_arrival;
}
