// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeatherSetupDialog.hpp"

#include "Dialogs/Weather/WeatherDialog.hpp"
#include "PageActions.hpp"
#include "PageSettings.hpp"

namespace WeatherMapOverlay {

void
ShowWeatherSetupDialog() noexcept
{
  const char *page = nullptr;
  switch (PageActions::GetCurrentLayout().overlay) {
  case PageLayout::Overlay::RASP:
    page = "rasp";
    break;
  case PageLayout::Overlay::EDL:
    page = "edl";
    break;
  case PageLayout::Overlay::XCTHERM:
    page = "xctherm";
    break;
  case PageLayout::Overlay::NONE:
  case PageLayout::Overlay::MAX:
    break;
  }

  ShowWeatherDialog(page);
}

} // namespace WeatherMapOverlay
