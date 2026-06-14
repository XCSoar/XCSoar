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
#ifdef HAVE_HTTP
#include "Dialogs/Weather/XCThermControlsWidget.hpp"
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
#ifdef ENABLE_OPENGL
  case PageLayout::Overlay::RASP:
    if (auto *controls = dynamic_cast<RaspControlsWidget *>(widget))
      controls->HandleWeatherOverlayInput(misc);
    break;
#endif

#ifdef HAVE_EDL
  case PageLayout::Overlay::EDL:
    if (auto *controls = dynamic_cast<EdlControlsWidget *>(widget))
      controls->HandleWeatherOverlayInput(misc);
    break;
#endif

#ifdef HAVE_HTTP
  case PageLayout::Overlay::XCTHERM:
    if (auto *controls = dynamic_cast<XCThermControlsWidget *>(widget))
      controls->HandleWeatherOverlayInput(misc);
    break;
#endif

  case PageLayout::Overlay::NONE:
  case PageLayout::Overlay::MAX:
    break;
  }
}
