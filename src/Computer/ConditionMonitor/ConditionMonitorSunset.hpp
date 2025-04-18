// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ConditionMonitor.hpp"

class ConditionMonitorSunset final : public ConditionMonitor {
public:
  constexpr ConditionMonitorSunset() noexcept
    :ConditionMonitor(std::chrono::minutes{30}, std::chrono::minutes{1}) {}

protected:
  bool CheckCondition(const NMEAInfo &basic,
                      const DerivedInfo &calculated,
                      const ComputerSettings &settings) noexcept override;
  void Notify() noexcept override;
  void SaveLast() noexcept override {}
};
