// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WindConfigPanel.hpp"
#include "../WindSettingsPanel.hpp"

std::unique_ptr<Widget>
CreateWindConfigPanel()
{
  return std::make_unique<WindSettingsPanel>(false, false, false);
}
