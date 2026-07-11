// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Form/DataField/ComboList.hpp"

#include <cstdint>

namespace WeatherMapOverlay {

enum class TimePickerSelection : uint8_t {
  CANCEL,
  AUTO,
  NOW,
  MANUAL,
  COUNT,
};

struct TimePickerResult {
  TimePickerSelection selection = TimePickerSelection::CANCEL;
  unsigned manual_index = 0;
};

/**
 * Weather forecast time picker with dedicated Auto / Now buttons.
 */
[[nodiscard]]
TimePickerResult
RunTimePicker(const char *caption,
              const ComboList &choices,
              bool enable_now = true) noexcept;

} // namespace WeatherMapOverlay
