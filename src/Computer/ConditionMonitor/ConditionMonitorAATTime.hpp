// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ConditionMonitor.hpp"

/** Checks whether arrival time will be less than AAT time */
class ConditionMonitorAATTime final : public ConditionMonitor {
public:
  constexpr ConditionMonitorAATTime() noexcept
    :ConditionMonitor(std::chrono::minutes{15}, std::chrono::seconds{10}) {}

protected:
  bool CheckCondition(const NMEAInfo &basic,
                      const DerivedInfo &calculated,
                      const ComputerSettings &settings) noexcept override;
  void Notify() noexcept override;
  void SaveLast() noexcept override {}
};
