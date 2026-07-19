// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PageSettings.hpp"

namespace WeatherMapOverlay {

enum class AddPageResult {
  SUCCESS,
  PAGE_LIMIT_REACHED,
};

static inline void
ClearWeatherOverlayFromLayout(PageLayout &layout) noexcept
{
  if (!layout.UsesWeatherOverlay())
    return;

  layout.overlay = PageLayout::Overlay::NONE;
  layout.Normalise();
}

static inline void
CopyWeatherOverlayCursors(PageLayout &dest,
                          const PageLayout &src) noexcept
{
  switch (src.overlay) {
  case PageLayout::Overlay::RASP:
    dest.rasp_field = src.rasp_field;
    dest.rasp_time = src.rasp_time;
    break;

  case PageLayout::Overlay::EDL:
    dest.edl_time = src.edl_time;
    dest.edl_isobar = src.edl_isobar;
    break;

  case PageLayout::Overlay::XCTHERM:
    dest.xctherm_layer = src.xctherm_layer;
    dest.xctherm_time = src.xctherm_time;
    break;

  case PageLayout::Overlay::NONE:
  case PageLayout::Overlay::MAX:
    break;
  }

  dest.Normalise();
}

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
  if (overlay != PageLayout::Overlay::RASP)
    layout.rasp_time = PageLayout::RASP_TIME_AUTO;
  if (overlay != PageLayout::Overlay::EDL) {
    layout.edl_time = PageLayout::EDL_TIME_AUTO;
    layout.edl_isobar = 0;
  }
  if (overlay != PageLayout::Overlay::XCTHERM) {
    layout.xctherm_layer = PageLayout::XCTHERM_LAYER_AUTO;
    layout.xctherm_time = PageLayout::XCTHERM_TIME_AUTO;
  }
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
                      PageLayout::Overlay overlay,
                      unsigned &new_page_index,
                      int rasp_field=-1) noexcept
{
  if (settings.n_pages >= PageSettings::MAX_PAGES)
    return AddPageResult::PAGE_LIMIT_REACHED;

  const unsigned append_index = settings.n_pages;
  /* Always start from the default map+InfoBoxes layout. */
  settings.pages[append_index] = PageLayout::Default();
  ApplyWeatherOverlayToLayout(settings.pages[append_index],
                              overlay, rasp_field);
  ++settings.n_pages;
  new_page_index = append_index;
  return AddPageResult::SUCCESS;
}

/** Like AddWeatherOverlayPage, then copy draft cursor fields. */
static inline AddPageResult
AddWeatherOverlayPageFromDraft(PageSettings &settings,
                               const PageLayout &draft,
                               unsigned &new_page_index) noexcept
{
  const int rasp_field = draft.overlay == PageLayout::Overlay::RASP
    ? draft.rasp_field
    : -1;
  const auto result = AddWeatherOverlayPage(settings, draft.overlay,
                                            new_page_index, rasp_field);
  if (result != AddPageResult::SUCCESS)
    return result;

  CopyWeatherOverlayCursors(settings.pages[new_page_index], draft);
  return AddPageResult::SUCCESS;
}

/**
 * Enable @p overlay on @p page_index (map page only). Clears weather
 * overlays on every other page. Does not persist — caller saves.
 *
 * @return false when the index is invalid or the page is not a map
 */
static inline bool
EnsureWeatherOverlayOnPage(PageSettings &settings,
                           unsigned page_index,
                           PageLayout::Overlay overlay,
                           int rasp_field=-1) noexcept
{
  if (page_index >= settings.n_pages)
    return false;

  auto &page = settings.pages[page_index];
  if (!page.IsDefined() || !page.IsMapMain())
    return false;

  if (page.overlay == overlay) {
    if (page.bottom != PageLayout::Bottom::WEATHER_CONTROLS) {
      page.bottom = PageLayout::Bottom::WEATHER_CONTROLS;
      page.Normalise();
    }
    return true;
  }

  for (unsigned i = 0; i < settings.n_pages; ++i) {
    if (i == page_index)
      continue;

    auto &other = settings.pages[i];
    if (!other.UsesWeatherOverlay())
      continue;

    ClearWeatherOverlayFromLayout(other);
  }

  ApplyWeatherOverlayToLayout(page, overlay, rasp_field);
  return true;
}

} // namespace WeatherMapOverlay
