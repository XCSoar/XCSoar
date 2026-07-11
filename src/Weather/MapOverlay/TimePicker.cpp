// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TimePicker.hpp"

#include "Dialogs/ComboPicker.hpp"
#include "Form/Form.hpp"
#include "Language/Language.hpp"

namespace WeatherMapOverlay {

TimePickerResult
RunTimePicker(const char *caption,
              const ComboList &choices,
              const bool enable_now) noexcept
{
  const int selected_index =
    ComboPicker(caption, choices, nullptr, false, _("Auto"),
                enable_now ? _("Now") : nullptr);

  TimePickerResult result;
  if (selected_index == mrExtra) {
    result.selection = TimePickerSelection::AUTO;
    return result;
  }

  if (selected_index == mrExtra2) {
    result.selection = TimePickerSelection::NOW;
    return result;
  }

  if (selected_index < 0)
    return result;

  result.selection = TimePickerSelection::MANUAL;
  result.manual_index = unsigned(selected_index);
  return result;
}

} // namespace WeatherMapOverlay
