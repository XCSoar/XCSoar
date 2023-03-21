// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AirspaceEnterMonitor.hpp"

/**
 * A composite of "more" #ConditionMonitor implementations to be
 * called by GlideComputer::ProcessIdle().
 *
 * @see ConditionMonitors
 */
class MoreConditionMonitors {
  AirspaceEnterMonitor airspace_enter;

public:
  explicit MoreConditionMonitors(const ProtectedAirspaceWarningManager &warnings) noexcept
    :airspace_enter(warnings) {}

  void Update(const NMEAInfo &basic, const DerivedInfo &calculated,
              const ComputerSettings &settings) noexcept;
};
