// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Blackboard/BlackboardListener.hpp"
#include "RateLimiter.hpp"
#include "WindMonitor.hpp"
#include "AirspaceWarningMonitor.hpp"
#include "TaskConstraintsMonitor.hpp"
#include "TaskAdvanceMonitor.hpp"
#include "MatTaskMonitor.hpp"

/**
 * A container that combines all monitor classes.
 */
class AllMonitors final : NullBlackboardListener, RateLimiter {
  WindMonitor wind;
  AirspaceWarningMonitor airspace_warnings;
  TaskConstraintsMonitor task_constraints;
  TaskAdvanceMonitor task_advance;
  MatTaskMonitor mat_task;

public:
  AllMonitors();
  ~AllMonitors();

  void Reset() {
    wind.Reset();
    airspace_warnings.Reset();
    task_constraints.Reset();
    task_advance.Reset();
    mat_task.Reset();
  }

  void Check() {
    wind.Check();
    airspace_warnings.Check();
    task_constraints.Check();
    task_advance.Check();
    mat_task.Check();
  }

private:
  void OnCalculatedUpdate([[maybe_unused]] const MoreData &basic,
                          [[maybe_unused]] const DerivedInfo &calculated) override {
    RateLimiter::Trigger();
  }

  void Run() override {
    Check();
  }
};
