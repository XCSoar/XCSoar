// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InputEvents.hpp"

#include "ControlsWidget.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "PageActions.hpp"

void
WeatherMapOverlay::HandleInputEvent(const char *misc) noexcept
{
  if (misc == nullptr || misc[0] == '\0' ||
      CommonInterface::main_window == nullptr)
    return;

  auto *controls = dynamic_cast<ControlsWidget *>(
    CommonInterface::main_window->GetBottomWidget());
  if (controls == nullptr)
    return;

  if (!PageActions::GetCurrentLayout().UsesWeatherOverlay())
    return;

  controls->HandleWeatherOverlayInput(misc);
}
