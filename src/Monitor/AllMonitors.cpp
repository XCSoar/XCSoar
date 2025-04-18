// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AllMonitors.hpp"
#include "Interface.hpp"

AllMonitors::AllMonitors()
  :RateLimiter(std::chrono::milliseconds(500), std::chrono::milliseconds(25))
{
  Reset();
  CommonInterface::GetLiveBlackboard().AddListener(*this);
}

AllMonitors::~AllMonitors() {
  CommonInterface::GetLiveBlackboard().RemoveListener(*this);
}
