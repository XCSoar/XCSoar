// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InputEvents.hpp"

#include "Interface.hpp"
#include "MainWindow.hpp"
#include "PageActions.hpp"

#ifdef ENABLE_OPENGL
#include "ControlsWidget.hpp"
#endif

void
WeatherMapOverlay::HandleInputEvent(const char *misc) noexcept
{
  if (misc == nullptr || CommonInterface::main_window == nullptr)
    return;

#ifdef ENABLE_OPENGL
  auto *controls = dynamic_cast<ControlsWidget *>(
    CommonInterface::main_window->GetBottomWidget());
  if (controls == nullptr)
    return;

  if (!PageActions::GetCurrentLayout().UsesWeatherOverlay())
    return;

  controls->HandleWeatherOverlayInput(misc);
#else
  (void)misc;
#endif
}
