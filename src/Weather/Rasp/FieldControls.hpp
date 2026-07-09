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
 * Step through forecast times available for @p field_index.
 *
 * @return false when stepping is not possible
 */
bool
StepTime(bool time_auto_advance, int field_index, int delta,
         unsigned &minute_of_day) noexcept;

/**
 * Synchronise #WeatherUIState::map from the active RASP page layout.
 */
void
SyncCursorFromPageLayout() noexcept;

/**
 * Select a manual cursor-bar time and end auto-advance.
 */
void
SetCursorTime(unsigned minute_of_day) noexcept;

[[nodiscard]] [[gnu::pure]]
bool
GetTimeAutoAdvance() noexcept;

void
SetTimeAutoAdvance(bool auto_advance) noexcept;

/**
 * Clear the manual time so rendering follows GPS local time ("Now").
 */
void
ApplyAutoAdvanceTime() noexcept;

void
ResumeAutoAdvance() noexcept;

/**
 * Step the cursor-bar time by @p delta available forecast slots and
 * apply the selection.
 *
 * @return false when stepping is not possible
 */
bool
StepCursorTime(int delta) noexcept;

/**
 * Compact label for the active RASP field (cursor bar).
 */
void
FormatFieldCursorLabel(StaticString<64> &text) noexcept;

/**
 * Number of RASP fields in the loaded repository, or @c 0.
 */
[[gnu::pure]]
unsigned
GetFieldCount() noexcept;

/**
 * Apply @p field_index to the current RASP page and active overlay.
 *
 * @return false when the field is invalid or the page is not RASP
 */
bool
SelectField(unsigned field_index) noexcept;

/**
 * Clear the selected RASP field while keeping the RASP overlay active.
 *
 * @return false when the current page is not a RASP overlay page
 */
bool
ClearSelectedField() noexcept;

/**
 * Step the active RASP field by @p delta (wraps at list ends).
 *
 * @return false when stepping is not possible
 */
bool
StepField(int delta) noexcept;

/**
 * Return true when a RASP field is loaded and selected.
 */
[[gnu::pure]]
bool
HasSelectedField() noexcept;

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
 *
 * Main-thread wrapper; uses #CommonInterface for state.
 */
[[gnu::pure]]
bool
HasSelectedTimeData(bool auto_advance) noexcept;

/**
 * Thread-safe variant for map rendering: caller supplies field index,
 * manual time, and GPS-based quarter-hour time for AUTO mode.
 */
[[gnu::pure]]
bool
HasSelectedTimeData(bool auto_advance, int field_index,
                    BrokenTime manual_time,
                    BrokenTime auto_local_time) noexcept;

/**
 * When AUTO is active but the configured field has no raster for the
 * current quarter hour, queue a repository / RASP refresh (at most once
 * per quarter hour while the gap persists).
 */
void
MaybeRequestConfiguredRaspUpdateOnAutoNoData() noexcept;

} // namespace Rasp
