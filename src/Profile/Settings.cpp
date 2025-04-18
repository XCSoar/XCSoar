// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Settings.hpp"
#include "SystemProfile.hpp"
#include "ComputerProfile.hpp"
#include "UIProfile.hpp"
#include "Interface.hpp"

void
Profile::Use(const ProfileMap &map)
{
  Load(map, CommonInterface::SetSystemSettings());
  Load(map, CommonInterface::SetComputerSettings());
  Load(map, CommonInterface::SetUISettings());
}
