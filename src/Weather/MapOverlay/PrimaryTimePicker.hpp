// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ControlsModel.hpp"
#include "TimePicker.hpp"

#include "Form/DataField/Enum.hpp"

#include "util/Compiler.h"

#include <utility>

namespace WeatherMapOverlay {

/**
 * Shared primary time picker workflow with overlay-specific callbacks.
 */
template<typename BuildChoicesFn, typename CurrentValueFn,
         typename ApplyAutoFn, typename ApplyNowFn, typename ApplyManualFn>
void
OpenPrimaryTimePicker(ControlsModel &model, const char *caption,
                      BuildChoicesFn &&build_choices,
                      CurrentValueFn &&current_value,
                      ApplyAutoFn &&apply_auto,
                      ApplyNowFn &&apply_now,
                      ApplyManualFn &&apply_manual,
                      bool enable_now = true) noexcept
{
  DataFieldEnum field;
  build_choices(field);
  field.SetValue(current_value());

  const ComboList combo_list = field.CreateComboList(nullptr);
  const TimePickerResult result =
    RunTimePicker(caption, combo_list, enable_now);

  switch (result.selection) {
  case TimePickerSelection::CANCEL:
    return;

  case TimePickerSelection::AUTO:
    apply_auto(model);
    return;

  case TimePickerSelection::NOW:
    apply_now(model);
    return;

  case TimePickerSelection::MANUAL:
    apply_manual(model, combo_list[result.manual_index].int_value);
    return;

  case TimePickerSelection::COUNT:
    break;
  }

  gcc_unreachable();
}

} // namespace WeatherMapOverlay
