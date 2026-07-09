// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PageSettings.hpp"

namespace WeatherMapOverlay {

enum class AddPageResult {
  SUCCESS,
  INVALID_SOURCE_PAGE,
  PAGE_LIMIT_REACHED,
};

static inline void
ApplyWeatherOverlayToLayout(PageLayout &layout,
                            PageLayout::Overlay overlay,
                            int rasp_field=-1) noexcept
{
  if (!layout.IsDefined())
    layout = PageLayout::Default();

  if (!layout.IsMapMain())
    layout.main = PageLayout::Main::MAP;

  layout.overlay = overlay;
  layout.bottom = PageLayout::Bottom::WEATHER_CONTROLS;
  layout.rasp_field = overlay == PageLayout::Overlay::RASP
    ? rasp_field
    : -1;
  layout.Normalise();
}

static inline bool
ApplyWeatherOverlayToPage(PageSettings &settings,
                          unsigned page_index,
                          PageLayout::Overlay overlay,
                          int rasp_field=-1) noexcept
{
  if (page_index >= settings.n_pages)
    return false;

  ApplyWeatherOverlayToLayout(settings.pages[page_index], overlay, rasp_field);
  return true;
}

static inline AddPageResult
AddWeatherOverlayPage(PageSettings &settings,
                      unsigned source_page_index,
                      PageLayout::Overlay overlay,
                      unsigned &new_page_index,
                      int rasp_field=-1) noexcept
{
  if (source_page_index >= settings.n_pages)
    return AddPageResult::INVALID_SOURCE_PAGE;

  if (settings.n_pages >= PageSettings::MAX_PAGES)
    return AddPageResult::PAGE_LIMIT_REACHED;

  const unsigned append_index = settings.n_pages;
  settings.pages[append_index] = settings.pages[source_page_index];
  ApplyWeatherOverlayToLayout(settings.pages[append_index],
                              overlay, rasp_field);
  ++settings.n_pages;
  new_page_index = append_index;
  return AddPageResult::SUCCESS;
}

} // namespace WeatherMapOverlay
