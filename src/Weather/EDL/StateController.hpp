// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"
#include "time/BrokenDateTime.hpp"
#include "util/StaticString.hxx"

namespace EDL {

/**
 * Initialise EDL state when entering an EDL page after leaving it.
 * Resyncs forecast time and level only while auto advance is enabled.
 */
void
ResetForDedicatedPage() noexcept;

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
 * Apply a cached overlay for the current cursor-bar selection, or clear
 * the map overlay when nothing is cached.
 */
void
ApplyOverlayFromSession() noexcept;

/**
 * Return true when the MBTiles file for the current forecast/isobar exists
 * in the local cache.
 */
bool
HasOverlayCache() noexcept;

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

/**
 * Return true when EDL should be active: the current page overlay is EDL,
 * a temporary dedicated EDL layout is active, or an EDL overlay is on the map.
 */
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

bool
TryApplyOverlay(Path path) noexcept;

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
 * Cursor-bar forecast label, e.g. @c "14:00 UTC (+1:30)" or @c "AUTO: …".
 * Uses the live session forecast time.
 */
void
FormatForecastCursorLabel(StaticString<64> &text,
                          bool auto_advance) noexcept;

/**
 * Like #FormatForecastCursorLabel for an explicit UTC forecast hour.
 */
void
FormatForecastCursorLabel(StaticString<64> &text, bool auto_advance,
                          BrokenDateTime forecast) noexcept;

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
