// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PageSettings.hpp"

#include <string_view>

namespace WeatherDialogOverlayActions {

bool
AddOverlayToCurrentPage(PageLayout::Overlay overlay,
                        int rasp_field=-1,
                        std::string_view skysight_layer_id={}) noexcept;

bool
AddOverlayToNewPage(PageLayout::Overlay overlay,
                    int rasp_field=-1,
                    std::string_view skysight_layer_id={}) noexcept;

} // namespace WeatherDialogOverlayActions
