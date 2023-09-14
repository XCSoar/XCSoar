// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DemoReplay.hpp"

void
DemoReplay::Start(const TaskAccessor& task, const GeoPoint& default_location)
{
  autopilot.SetDefaultLocation(default_location);
  autopilot.Start(task);
  aircraft.Start(autopilot.location_start, autopilot.location_previous,
                 parms.start_alt);
}

bool
DemoReplay::Update(FloatDuration time_scale,
                   TaskAccessor &task) noexcept
{
  autopilot.UpdateState(task, aircraft.GetState(), time_scale);
  aircraft.Update(autopilot.heading, time_scale);
  return autopilot.UpdateAutopilot(task, aircraft.GetState());
}
