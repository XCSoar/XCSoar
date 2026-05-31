// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/BrokenTime.hpp"

class DataFieldEnum;
class RaspStore;

namespace Rasp {

struct FieldChoicesOptions {
  bool include_none = false;
};

/**
 * Fill a #DataFieldEnum with the available RASP fields.
 */
void
FillFieldChoices(DataFieldEnum &field, const RaspStore *rasp,
                 FieldChoicesOptions options={}) noexcept;

/**
 * Initialise a time selector with the "Now" choice.
 */
void
InitTimeChoices(DataFieldEnum &field) noexcept;

/**
 * Fill a #DataFieldEnum with the forecast times available for a field.
 */
void
FillTimeChoices(DataFieldEnum &field, const RaspStore *rasp,
                unsigned field_index, BrokenTime selected_time) noexcept;

[[gnu::pure]]
BrokenTime
TimeFromMinuteOfDay(unsigned minute_of_day) noexcept;

[[gnu::pure]]
unsigned
MinuteOfDayFromTime(BrokenTime time) noexcept;

/**
 * Step through the forecast times available for a field.
 *
 * @return false if stepping is not possible
 */
bool
StepTime(const RaspStore *rasp, unsigned field_index,
         BrokenTime current_time, int delta,
         unsigned &minute_of_day) noexcept;

} // namespace Rasp
