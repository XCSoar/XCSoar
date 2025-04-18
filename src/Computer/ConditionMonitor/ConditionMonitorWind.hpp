// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ConditionMonitor.hpp"
#include "Geo/SpeedVector.hpp"

/** #ConditionMonitor to track/warn on significant changes in wind speed */
class ConditionMonitorWind final : public ConditionMonitor {
  SpeedVector wind = SpeedVector::Zero();
  SpeedVector last_wind = SpeedVector::Zero();

public:
  constexpr ConditionMonitorWind() noexcept
    :ConditionMonitor(std::chrono::minutes{5}, std::chrono::seconds{10}) {}

protected:
  bool CheckCondition(const NMEAInfo &basic,
                      const DerivedInfo &calculated,
                      const ComputerSettings &settings) noexcept override;
  void Notify() noexcept override;
  void SaveLast() noexcept override;
};
