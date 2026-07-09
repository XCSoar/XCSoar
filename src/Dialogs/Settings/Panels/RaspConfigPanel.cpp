// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RaspConfigPanel.hpp"
#include "Dialogs/Weather/RASPDialog.hpp"
#include "Widget/Widget.hpp"

std::unique_ptr<Widget>
CreateRaspConfigPanel() noexcept
{
  return CreateRaspWidget();
}
