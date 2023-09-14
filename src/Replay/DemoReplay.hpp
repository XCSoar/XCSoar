// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TaskAutoPilot.hpp"
#include "AircraftSim.hpp"
#include "time/FloatDuration.hxx"

class DemoReplay
{
public:
  AutopilotParameters parms;
  TaskAutoPilot autopilot{parms};
  AircraftSim aircraft;

protected:
  void Start(const TaskAccessor& task, const GeoPoint& default_location);
  bool Update(FloatDuration time_scale,
              TaskAccessor &task) noexcept;
};
