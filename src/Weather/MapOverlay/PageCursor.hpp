// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Interface.hpp"
#include "PageSettings.hpp"
#include "Profile/Current.hpp"
#include "Profile/PageProfile.hpp"

namespace WeatherMapOverlay {

/**
 * Invoke @p fn with the page at @p page_index when it uses @p overlay.
 * #Normalise and profile-save when @p fn returns true.
 *
 * @return false when the index is out of range, the overlay does not
 * match, or @p fn returns false
 */
template<typename F>
bool
MutateOverlayPage(unsigned page_index, PageLayout::Overlay overlay,
                  F &&fn)
{
  PageSettings &settings = CommonInterface::SetUISettings().pages;
  if (page_index >= settings.n_pages)
    return false;

  PageLayout &page = settings.pages[page_index];
  if (page.overlay != overlay)
    return false;

  if (!fn(page))
    return false;

  page.Normalise();
  Profile::Save(Profile::map, page, page_index);
  return true;
}

} // namespace WeatherMapOverlay
