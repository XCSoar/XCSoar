// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConditionMonitors.hpp"

void
ConditionMonitors::Update(const NMEAInfo &basic, const DerivedInfo &calculated,
                          const ComputerSettings &settings) noexcept
{
  if (suppressed)
    return;

  wind.Update(basic, calculated, settings);
  finalglide.Update(basic, calculated, settings);
  sunset.Update(basic, calculated, settings);
  aattime.Update(basic, calculated, settings);
  glideterrain.Update(basic, calculated, settings);
  landablereachable.Update(basic, calculated, settings);
}
