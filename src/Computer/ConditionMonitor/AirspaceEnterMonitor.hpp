// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Airspace/Ptr.hpp"
#include "util/Serial.hpp"

#include <set>

struct NMEAInfo;
struct DerivedInfo;
struct ComputerSettings;
class ProtectedAirspaceWarningManager;
class AirspaceWarningManager;

/** #ConditionMonitor to track/warn on significant changes in wind speed */
class AirspaceEnterMonitor final {
  const ProtectedAirspaceWarningManager &protected_warnings;

  std::set<ConstAirspacePtr> last_near, last_inside;

  Serial last_serial;

public:
  explicit AirspaceEnterMonitor(const ProtectedAirspaceWarningManager &_warnings) noexcept
    :protected_warnings(_warnings) {}

  void Update(const NMEAInfo &basic, const DerivedInfo &calculated,
              const ComputerSettings &settings) noexcept;

private:
  void Update(const AirspaceWarningManager &warnings) noexcept;
};
