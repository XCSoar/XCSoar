// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Weather/MapOverlay/FieldPickerResult.hpp"
#include "time/BrokenDateTime.hpp"
#include "util/StaticString.hxx"

struct PageLayout;

namespace EDL {

using LevelPickerResult = WeatherMapOverlay::FieldPickerResult;

/**
 * Cursor-bar style forecast time label (UTC), with optional [no data].
 */
void
FormatTimeCursorLabel(StaticString<64> &text, bool auto_advance) noexcept;

/**
 * Cursor-bar style pressure/altitude label, with optional [no data].
 */
void
FormatLevelCursorLabel(StaticString<64> &text, bool auto_advance) noexcept;

void
SelectForecastTime(const BrokenDateTime &time) noexcept;

void
SelectLevel(unsigned isobar) noexcept;

void
EnableForecastAutoFromInput() noexcept;

void
EnableLevelAutoFromInput() noexcept;

/**
 * Apply the page's stored EDL forecast time to the live session.
 */
void
ApplyTimeFromPageLayout(const PageLayout &layout) noexcept;

/**
 * Persist the live EDL forecast time to a configured page.
 */
void
PersistTimeToPage(unsigned page_index) noexcept;

void
PersistTimeToCurrentPage() noexcept;

/**
 * Step forecast time by @p delta hours in the available list.
 *
 * @return false when stepping is not possible
 */
[[nodiscard]]
bool
StepForecastTime(int delta) noexcept;

/**
 * Step pressure level by @p delta bands (positive = higher altitude).
 *
 * @return false when stepping is not possible
 */
[[nodiscard]]
bool
StepLevel(int delta) noexcept;

/**
 * Edit @p page.edl_time via the shared time picker. Does not persist
 * or update the live overlay (dialogs use this on a draft layout).
 *
 * @return false when the user cancels or no forecast times are ready
 */
[[nodiscard]]
bool
EditTimeOnLayout(PageLayout &page) noexcept;

/**
 * Cursor-bar time picker: #EditTimeOnLayout on the current map page,
 * then persist and refresh live.
 */
void
OpenTimePicker() noexcept;

/**
 * Format Time from a page's stored cursor without changing the live session.
 */
void
FormatTimeLabelForPage(StaticString<64> &text,
                       const PageLayout &page) noexcept;

/**
 * Edit @p page.edl_isobar via the level picker. Does not persist or
 * update the live overlay (dialogs use this on a draft layout).
 */
[[nodiscard]]
LevelPickerResult
EditLevelOnLayout(PageLayout &page, bool offer_setup=false) noexcept;

/**
 * Cursor-bar level picker: #EditLevelOnLayout on the current map page,
 * then persist and refresh live. When @p offer_setup is true, includes
 * Setup.
 */
[[nodiscard]]
LevelPickerResult
OpenLevelPicker(bool offer_setup=false) noexcept;

/**
 * Format the Level control label from a page's stored isobar without
 * changing the live EDL session.
 */
void
FormatLevelLabelForPage(StaticString<64> &text,
                        const PageLayout &page) noexcept;

} // namespace EDL
