// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ControlsModel.hpp"

#include "PageSettings.hpp"

#include <memory>

namespace WeatherMapOverlay {

[[nodiscard]]
std::unique_ptr<ControlsModel>
CreateControlsModel(PageLayout::Overlay overlay) noexcept;

void EnterOverlayPage(const PageLayout &layout) noexcept;
void LeaveOverlayPage(const PageLayout &layout) noexcept;

} // namespace WeatherMapOverlay
