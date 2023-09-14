// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "time/DeltaTime.hpp"

class Airspaces;
struct ComputerSettings;
struct MoreData;
struct DerivedInfo;
struct AirspaceWarningsInfo;

/**
 * Manage airspace warnings.
 */
class WarningComputer {
  DeltaTime delta_time;

  Airspaces &airspaces;

  AirspaceWarningManager manager;
  ProtectedAirspaceWarningManager protected_manager;

  bool initialised;

public:
  WarningComputer(const AirspaceWarningConfig &_config,
                  Airspaces &_airspaces);

  ProtectedAirspaceWarningManager &GetManager() {
    return protected_manager;
  }

  const ProtectedAirspaceWarningManager &GetManager() const {
    return protected_manager;
  }

  void Reset() {
    delta_time.Reset();
    initialised = false;
  }

  void Update(const ComputerSettings &settings_computer,
              const MoreData &basic,
              const DerivedInfo &calculated,
              AirspaceWarningsInfo &result);
};
