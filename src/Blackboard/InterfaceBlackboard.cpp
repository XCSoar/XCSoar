// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InterfaceBlackboard.hpp"

void
InterfaceBlackboard::ReadBlackboardCalculated(const DerivedInfo &derived_info) noexcept
{
  calculated_info = derived_info;
}

void
InterfaceBlackboard::ReadBlackboardBasic(const MoreData &nmea_info) noexcept
{
  gps_info = nmea_info;
}

void
InterfaceBlackboard::ReadComputerSettings(const ComputerSettings &settings) noexcept
{
  computer_settings = settings;
}

