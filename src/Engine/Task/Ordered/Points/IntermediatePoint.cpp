// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "IntermediatePoint.hpp"
#include "Task/TaskBehaviour.hpp"

IntermediateTaskPoint::IntermediateTaskPoint(TaskPointType _type,
                                             std::unique_ptr<ObservationZonePoint> &&_oz,
                                             WaypointPtr &&wp,
                                             const TaskBehaviour &tb,
                                             const bool b_scored)
  :OrderedTaskPoint(_type, std::move(_oz), std::move(wp), b_scored),
   safety_height(tb.safety_height_arrival) {}

void
IntermediateTaskPoint::SetTaskBehaviour(const TaskBehaviour &tb) noexcept
{
  safety_height = tb.safety_height_arrival;
}

double
IntermediateTaskPoint::GetElevation() const noexcept
{
  return GetBaseElevation() + safety_height;
}
