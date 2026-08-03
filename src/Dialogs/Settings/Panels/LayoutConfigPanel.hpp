// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/InfoBoxSettings.hpp"

#include <memory>

class Widget;

std::unique_ptr<Widget>
CreateLayoutConfigPanel();

/**
 * Geometry selected in Screen Layout, including unsaved changes while
 * that panel is still open.  Falls back to the committed UI setting.
 */
[[nodiscard]] InfoBoxSettings::Geometry
GetConfiguredInfoBoxGeometry() noexcept;
