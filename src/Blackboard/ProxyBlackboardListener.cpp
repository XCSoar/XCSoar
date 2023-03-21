// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ProxyBlackboardListener.hpp"

void
ProxyBlackboardListener::OnGPSUpdate(const MoreData &basic)
{
  next.OnGPSUpdate(basic);
}

void
ProxyBlackboardListener::OnCalculatedUpdate(const MoreData &basic,
                                            const DerivedInfo &calculated)
{
  next.OnCalculatedUpdate(basic, calculated);
}

void
ProxyBlackboardListener::OnComputerSettingsUpdate(const ComputerSettings &settings)
{
  next.OnComputerSettingsUpdate(settings);
}

void
ProxyBlackboardListener::OnUISettingsUpdate(const UISettings &settings)
{
  next.OnUISettingsUpdate(settings);
}
