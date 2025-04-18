// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DialogSettings.hpp"

void
DialogSettings::SetDefaults() noexcept
{
  text_input_style = TextInputStyle::Default;
  tab_style = TabStyle::Text;
  expert = false;
}
