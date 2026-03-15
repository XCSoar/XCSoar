// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"
#include "time/BrokenDateTime.hpp"
#include "util/StaticString.hxx"

#include <chrono>

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
 * Check whether the user wants the EDL overlay visible on the normal map page.
 */
bool
ShouldShowOnMainMap() noexcept;

/**
 * Persist whether the EDL overlay shall remain visible on the normal map page.
 */
void
SetShowOnMainMap(bool enabled) noexcept;

/**
 * Remove the active EDL overlay from the map.
 */
void
ClearOverlay() noexcept;

/**
 * Check whether EDL support is enabled in weather settings.
 */
bool
OverlayEnabled() noexcept;

/**
 * Check whether the currently active map overlay is an EDL overlay.
 */
bool
OverlayVisible() noexcept;

/**
 * Set the EDL status to "loading" if the feature is enabled.
 */
void
SetLoadingStatus() noexcept;

/**
 * Set the EDL status back to the idle/disabled state.
 */
void
SetIdleStatus() noexcept;

/**
 * Set the EDL status to "error".
 */
void
SetErrorStatus() noexcept;

/**
 * Create and apply an EDL MBTiles overlay for the specified file.
 */
void
ApplyOverlay(Path path) noexcept;

/**
 * Clear the overlay if the current settings do not allow it on the main map.
 */
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
