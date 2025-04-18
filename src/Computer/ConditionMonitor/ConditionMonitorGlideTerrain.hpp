// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ConditionMonitor.hpp"

class ConditionMonitorGlideTerrain final : public ConditionMonitor {
public:
  constexpr ConditionMonitorGlideTerrain() noexcept
    :ConditionMonitor(std::chrono::minutes{5}, std::chrono::seconds{1}) {}

protected:
  bool CheckCondition(const NMEAInfo &basic,
                      const DerivedInfo &calculated,
                      const ComputerSettings &settings) noexcept override;
  void Notify() noexcept override;
  void SaveLast() noexcept override {}
};
