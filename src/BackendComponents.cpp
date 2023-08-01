// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BackendComponents.hpp"
#include "Device/MultipleDevices.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Computer/GlideComputer.hpp"
#include "Logger/Logger.hpp"
#include "Logger/NMEALogger.hpp"
#include "Logger/GlueFlightLogger.hpp"
#include "MergeThread.hpp"
#include "CalculationThread.hpp"

BackendComponents::BackendComponents() noexcept
  :device_blackboard(new DeviceBlackboard())
{
}

BackendComponents::~BackendComponents() noexcept = default;
