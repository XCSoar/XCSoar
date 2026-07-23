// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Weather/MapOverlay/FieldPickerResult.hpp"
#include "time/BrokenTime.hpp"
#include "util/StaticString.hxx"

#include <cstdint>

class DataFieldEnum;
class RaspStore;
struct PageLayout;

namespace Rasp {

using LayerPickerResult = WeatherMapOverlay::FieldPickerResult;

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
 * Encode session RASP time state for #PageLayout::rasp_time.
 */
[[gnu::pure]]
int
EncodePageTime(bool time_auto_advance, BrokenTime time) noexcept;

/**
 * Apply #PageLayout::rasp_time to the shared RASP session state.
 */
void
ApplyTimeFromPageLayout(const PageLayout &layout) noexcept;

/**
 * Persist the current session RASP time onto @p page_index.
 */
void
PersistTimeToPage(unsigned page_index) noexcept;

/**
 * Persist the current session RASP time onto the configured page.
 */
void
PersistTimeToCurrentPage() noexcept;

/**
 * Select a manual cursor-bar time and end auto-advance.
 */
void
SetCursorTime(unsigned minute_of_day) noexcept;

/**
 * Enable time auto-advance and clear the manual time (AUTO mode).
 * May request a RASP download when the current quarter hour has no data.
 */
void
EnableTimeAutoFromInput() noexcept;

/**
 * Edit @p page.rasp_time via the shared time picker. Does not persist
 * or update the live overlay (dialogs use this on a draft layout).
 *
 * @return false when the user cancels or no field is available
 */
[[nodiscard]]
bool
EditTimeOnLayout(PageLayout &page) noexcept;

/**
 * Cursor-bar time picker: #EditTimeOnLayout on the current map page,
 * then persist and refresh the live overlay.
 */
void
OpenTimePicker() noexcept;

/**
 * Format the Time control label from a page's stored field/time.
 * Does not mutate the live RASP session.
 */
void
FormatTimeLabelForPage(StaticString<64> &text,
                       const PageLayout &page) noexcept;

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
 * Open the RASP layer picker.  When @p offer_setup is true, includes a
 * Setup button (cursor-bar use); Info → Weather passes false.
 */
[[nodiscard]]
LayerPickerResult
OpenLayerPicker(bool offer_setup=false) noexcept;

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
 * Cursor-bar time label, e.g. @c "14:30 (+0:15)" or @c "AUTO: …".
 *
 * When @p auto_advance is true, the display follows GPS local time ("Now").
 */
void
FormatTimeCursorLabel(StaticString<64> &text, bool auto_advance) noexcept;

/**
 * Minute-of-day for the effective cursor-bar time (manual or AUTO).
 */
[[gnu::pure]]
unsigned
GetCursorBarMinuteOfDay() noexcept;

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
