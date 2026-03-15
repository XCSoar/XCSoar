// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PageSettings.hpp"

#include <memory>

class Widget;

std::unique_ptr<Widget>
CreateMapOverlayControlsOverlayWidget(PageLayout::Overlay overlay) noexcept;

std::unique_ptr<Widget>
CreateMapOverlayControlsBottomWidget(PageLayout::Overlay overlay) noexcept;
