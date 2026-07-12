// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OverlayPageActions.hpp"

#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "PageActions.hpp"

namespace WeatherDialogOverlayActions {

bool
AddOverlayToCurrentPage(PageLayout::Overlay overlay, int rasp_field,
                        std::string_view skysight_layer_id) noexcept
{
  const auto result =
    PageActions::AddWeatherOverlayToCurrentPage(overlay, rasp_field,
                                                skysight_layer_id);
  if (result == PageActions::ConfigureWeatherOverlayResult::PAGE_LIMIT_REACHED)
    ShowMessageBox(_("Cannot add another weather page (page limit reached)."),
                   _("Weather"), MB_OK);

  return result == PageActions::ConfigureWeatherOverlayResult::APPLIED_CURRENT ||
         result == PageActions::ConfigureWeatherOverlayResult::ADDED_PAGE;
}

bool
AddOverlayToNewPage(PageLayout::Overlay overlay, int rasp_field,
                    std::string_view skysight_layer_id) noexcept
{
  const auto result =
    PageActions::AddWeatherOverlayToNewPage(overlay, rasp_field,
                                            skysight_layer_id);
  if (result == PageActions::ConfigureWeatherOverlayResult::PAGE_LIMIT_REACHED)
    ShowMessageBox(_("Cannot add another weather page (page limit reached)."),
                   _("Weather"), MB_OK);

  return result == PageActions::ConfigureWeatherOverlayResult::ADDED_PAGE ||
         result == PageActions::ConfigureWeatherOverlayResult::APPLIED_CURRENT;
}

} // namespace WeatherDialogOverlayActions
