// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PageSettings.hpp"

namespace WeatherDialogOverlayActions {

bool
AddOverlayToCurrentPage(PageLayout::Overlay overlay,
                        int rasp_field=-1) noexcept;

bool
AddOverlayToNewPage(PageLayout::Overlay overlay,
                    int rasp_field=-1) noexcept;

} // namespace WeatherDialogOverlayActions
