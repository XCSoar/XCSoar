// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PageSettings.hpp"

#include <string_view>

namespace WeatherMapOverlay {

enum class AddPageResult {
  SUCCESS,
  INVALID_SOURCE_PAGE,
  PAGE_LIMIT_REACHED,
};

static inline void
ApplyWeatherOverlayToLayout(PageLayout &layout,
                            PageLayout::Overlay overlay,
                            int rasp_field=-1,
                            std::string_view skysight_layer_id={}) noexcept
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
  if (overlay == PageLayout::Overlay::SKYSIGHT)
    layout.skysight_overlay = skysight_layer_id;
  else
    layout.skysight_overlay.clear();
  layout.Normalise();
}

static inline bool
ApplyWeatherOverlayToPage(PageSettings &settings,
                          unsigned page_index,
                          PageLayout::Overlay overlay,
                          int rasp_field=-1,
                          std::string_view skysight_layer_id={}) noexcept
{
  if (page_index >= settings.n_pages)
    return false;

  ApplyWeatherOverlayToLayout(settings.pages[page_index], overlay,
                              rasp_field, skysight_layer_id);
  return true;
}

static inline bool
SetSkySightLayerOnPage(PageSettings &settings, unsigned page_index,
                       std::string_view layer_id) noexcept
{
  if (page_index >= settings.n_pages || layer_id.empty())
    return false;

  auto &page = settings.pages[page_index];
  if (!page.IsMapMain() || page.overlay != PageLayout::Overlay::SKYSIGHT)
    return false;

  page.skysight_overlay = layer_id;
  page.Normalise();
  return true;
}

static inline AddPageResult
AddWeatherOverlayPage(PageSettings &settings,
                      unsigned source_page_index,
                      PageLayout::Overlay overlay,
                      unsigned &new_page_index,
                      int rasp_field=-1,
                      std::string_view skysight_layer_id={}) noexcept
{
  if (source_page_index >= settings.n_pages)
    return AddPageResult::INVALID_SOURCE_PAGE;

  if (settings.n_pages >= PageSettings::MAX_PAGES)
    return AddPageResult::PAGE_LIMIT_REACHED;

  const unsigned append_index = settings.n_pages;
  settings.pages[append_index] = settings.pages[source_page_index];
  ApplyWeatherOverlayToLayout(settings.pages[append_index],
                              overlay, rasp_field, skysight_layer_id);
  ++settings.n_pages;
  new_page_index = append_index;
  return AddPageResult::SUCCESS;
}

} // namespace WeatherMapOverlay
