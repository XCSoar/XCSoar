// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InputEvents.hpp"

#include "Interface.hpp"
#include "MainWindow.hpp"
#include "PageActions.hpp"
#include "PageSettings.hpp"

#ifdef HAVE_EDL
#include "Dialogs/Weather/EdlControlsWidget.hpp"
#endif
#ifdef ENABLE_OPENGL
#include "Dialogs/Weather/RaspControlsWidget.hpp"
#endif

void
WeatherMapOverlay::HandleInputEvent(const char *misc) noexcept
{
  if (misc == nullptr || CommonInterface::main_window == nullptr)
    return;

  Widget *widget = CommonInterface::main_window->GetBottomWidget();
  if (widget == nullptr)
    return;

  const PageLayout &layout = PageActions::GetCurrentLayout();
  if (!layout.UsesWeatherOverlay())
    return;

  switch (layout.overlay) {
  case PageLayout::Overlay::RASP:
#ifdef ENABLE_OPENGL
    if (auto *controls = dynamic_cast<RaspControlsWidget *>(widget))
      controls->HandleWeatherOverlayInput(misc);
#endif
    break;

  case PageLayout::Overlay::EDL:
#ifdef HAVE_EDL
    if (auto *controls = dynamic_cast<EdlControlsWidget *>(widget))
      controls->HandleWeatherOverlayInput(misc);
#endif
    break;

  case PageLayout::Overlay::NONE:
  case PageLayout::Overlay::MAX:
    break;
  }
}
