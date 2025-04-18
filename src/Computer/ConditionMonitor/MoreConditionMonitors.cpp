// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MoreConditionMonitors.hpp"

void
MoreConditionMonitors::Update(const NMEAInfo &basic, const DerivedInfo &calculated,
                              const ComputerSettings &settings) noexcept
{
  airspace_enter.Update(basic, calculated, settings);
}
