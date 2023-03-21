// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WindMonitor.hpp"
#include "Interface.hpp"

void
WindMonitor::Check()
{
  auto &settings_computer = CommonInterface::SetComputerSettings();
  const auto &calculated = CommonInterface::Calculated();

  /* as soon as another wind setting is used, clear the manual wind */
  if (calculated.wind_available.Modified(settings_computer.wind.manual_wind_available))
    settings_computer.wind.manual_wind_available.Clear();
}
