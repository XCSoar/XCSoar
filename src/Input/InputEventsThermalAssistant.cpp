// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InputEvents.hpp"
#include "PageActions.hpp"

void
InputEvents::eventThermalAssistant([[maybe_unused]] const char *misc)
{
  PageActions::ShowThermalAssistant();
}
