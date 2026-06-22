// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/BrokenTime.hpp"
#include "util/StaticString.hxx"

class DataFieldEnum;
class RaspStore;
struct PageLayout;

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

/**
 * RASP field index for @p layout, or @c -1 when unavailable.
 */
[[gnu::pure]]
int
GetFieldIndex(const PageLayout &layout) noexcept;

/**
 * RASP field index for the current page layout.
 */
[[gnu::pure]]
int
GetActiveFieldIndex() noexcept;

/**
 * Field index for rendering: current layout, or persisted #weather.map
 * while the overlay is kept during pan full-screen.
 */
[[gnu::pure]]
int
GetEffectiveFieldIndex() noexcept;

[[gnu::pure]]
BrokenTime
TimeFromMinuteOfDay(unsigned minute_of_day) noexcept;

[[gnu::pure]]
unsigned
MinuteOfDayFromTime(BrokenTime time) noexcept;

/**
 * Step through local quarter hours (0:00–23:45), even when no raster exists.
 *
 * @return false when @p delta is zero
 */
bool
StepTime(const RaspStore *rasp, unsigned field_index,
         BrokenTime current_time, bool time_auto_advance,
         int delta, unsigned &minute_of_day) noexcept;

/**
 * Compact label for the active RASP field (map-scale PAN string).
 */
StaticString<64>
GetPanOverlayLabel(const PageLayout &configured) noexcept;

/**
 * Cursor-bar time label, e.g. @c "14:30 (+0:15)" or @c "AUTO: …".
 *
 * When @p auto_advance is true, the display follows GPS local time ("Now").
 */
void
FormatTimeCursorLabel(StaticString<64> &text, bool auto_advance) noexcept;

/**
 * Return true when the active RASP field has raster data for the
 * effective cursor-bar time ("Now"/AUTO or manual selection).
 */
[[gnu::pure]]
bool
HasSelectedTimeData(bool auto_advance) noexcept;

/**
 * When AUTO is active but the configured field has no raster for the
 * current quarter hour, queue a repository / RASP refresh (at most once
 * per quarter hour while the gap persists).
 */
void
MaybeRequestConfiguredRaspUpdateOnAutoNoData() noexcept;

} // namespace Rasp
