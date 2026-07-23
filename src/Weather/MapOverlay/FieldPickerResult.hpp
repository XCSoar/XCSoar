// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

namespace WeatherMapOverlay {

/**
 * Result of a weather FieldControls secondary picker (layer / level /
 * altitude). Shared by RASP, EDL, and XCTherm.
 */
enum class FieldPickerResult : uint8_t {
  NONE,
  CHANGED,
  OPEN_SETUP,
};

} // namespace WeatherMapOverlay
