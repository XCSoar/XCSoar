// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WindEdit.hpp"
#include "Dialogs/Settings/WindSettingsPanel.hpp"

std::unique_ptr<Widget>
LoadWindEditPanel([[maybe_unused]] unsigned id)
{
  return std::make_unique<WindSettingsPanel>(true, true, true);
}
