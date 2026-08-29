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
  case PageLayout::Overlay::SKYSIGHT:
    page = "skysight";
    break;
  case PageLayout::Overlay::SATELLITE:
    /* the imagery is listed on the Overlay tab, so open it there
       rather than wherever the dialog was left last */
    page = "overlay";
    break;
  case PageLayout::Overlay::RADAR:
  case PageLayout::Overlay::NONE:
  case PageLayout::Overlay::MAX:
    break;
  }

  ShowWeatherDialog(page);
}

} // namespace WeatherMapOverlay
