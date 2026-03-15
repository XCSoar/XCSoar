// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"
#include "time/BrokenDateTime.hpp"
#include "util/StaticString.hxx"

namespace EDL {

/**
 * Reset the shared EDL selection for entering the dedicated EDL page.
 */
void
ResetForDedicatedPage() noexcept;

/**
 * Mark the dedicated EDL page as left, so the next entry resets it.
 */
void
LeaveDedicatedPage() noexcept;

/**
 * Mark the dedicated EDL page as entered.
 *
 * @return true if this is the first entry since the last leave
 */
bool
EnterDedicatedPage() noexcept;

/**
 * Preserve dedicated-page state while temporarily switching away for pan mode.
 */
void
SuspendDedicatedPageForPan() noexcept;

/**
 * Clear the temporary dedicated-page pan suspension flag.
 */
void
ResumeDedicatedPageAfterPan() noexcept;

/**
 * Check whether the dedicated EDL page is only hidden temporarily for pan mode.
 */
bool
IsDedicatedPageSuspendedForPan() noexcept;

/**
 * Update the current EDL level from the current aircraft altitude.
 */
void
UpdateCurrentLevel() noexcept;

/**
 * Ensure the shared EDL UI state is valid before it is used.
 */
void
EnsureInitialised() noexcept;

/**
 * Check whether the overlay should stay synchronised in the background.
 */
bool
ShouldMaintainOverlay() noexcept;

/**
 * Apply the cached overlay for the current forecast/isobar, if available.
 *
 * @return true if an overlay was applied
 */
bool
TryApplyOverlayFromCache() noexcept;

/**
 * Advance the shared forecast to the tracked hour and refresh a cached
 * overlay when appropriate.
 */
void
OnTimeUpdate(BrokenDateTime utc) noexcept;

/**
 * Apply GPS-driven forecast and level updates while auto-advance is enabled.
 */
void
ProcessGpsUpdate(BrokenDateTime utc) noexcept;

/**
 * Return and clear a pending UI refresh signalled by ProcessGpsUpdate().
 */
bool
TakeGpsUiRefreshPending() noexcept;

/**
 * Discard a pending UI refresh without refreshing.
 */
void
ClearGpsUiRefreshPending() noexcept;

void
ClearOverlay() noexcept;

bool
OverlayEnabled() noexcept;

bool
OverlayVisible() noexcept;

void
SetLoadingStatus() noexcept;

void
SetIdleStatus() noexcept;

void
SetErrorStatus() noexcept;

void
ApplyOverlay(Path path);

void
RefreshOverlayVisibility() noexcept;

/**
 * Return the translated status label for the current EDL state.
 */
const char *
GetStatusLabel() noexcept;

/**
 * Return the currently selected EDL forecast time in UTC.
 */
BrokenDateTime
GetForecastTime() noexcept;

/**
 * Return the currently selected EDL forecast time converted to local time.
 */
BrokenDateTime
GetForecastTimeLocal() noexcept;

/**
 * Build the map legend text for the active EDL overlay.
 */
StaticString<64>
GetOverlayLabel() noexcept;

/**
 * Return the altitude corresponding to the selected EDL isobar.
 */
int
GetAltitude() noexcept;

/**
 * Return the selected EDL isobar in Pascal.
 */
unsigned
GetIsobar() noexcept;

/**
 * Convert a supported EDL isobar to its corresponding altitude.
 */
int
GetAltitudeForIsobar(unsigned isobar) noexcept;

} // namespace EDL
