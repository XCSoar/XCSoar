// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ConditionMonitor.hpp"

class ConditionMonitorLandableReachable final : public ConditionMonitor {
  bool last_reachable = false;
  bool now_reachable = false;

public:
  constexpr ConditionMonitorLandableReachable() noexcept
    :ConditionMonitor(std::chrono::minutes{5}, std::chrono::seconds{1}) {}

protected:
  bool CheckCondition(const NMEAInfo &basic,
                      const DerivedInfo &calculated,
                      const ComputerSettings &settings) noexcept override;
  void Notify() noexcept override;
  void SaveLast() noexcept override;
};
