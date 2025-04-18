// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Computer/Settings.hpp"

class ComputerSettingsBlackboard
{
protected:
  ComputerSettings computer_settings;

public:
  [[gnu::const]]
  const ComputerSettings& GetComputerSettings() const {
    return computer_settings;
  }
};
