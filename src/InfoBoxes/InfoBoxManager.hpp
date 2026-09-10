// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxSettings.hpp"

struct InfoBoxLook;
class ContainerWindow;
class InfoBoxWindow;

namespace InfoBoxLayout { struct Layout; }

namespace InfoBoxManager
{

extern InfoBoxLayout::Layout layout;

void
ProcessTimer() noexcept;

[[nodiscard]] bool
IsReady() noexcept;

/**
 * Returns the InfoBox window for the given slot, or nullptr if the
 * manager was reinitialised and the window no longer exists.
 */
[[nodiscard]] InfoBoxWindow *
GetWindow(unsigned id) noexcept;

void
SetDirty() noexcept;

/**
 * Call after the UI language was switched (#ReadLanguageFile) so
 * captions and content use the new gettext catalogue on the next draw
 * (#2314).
 */
void
InvalidateAfterLanguageChange() noexcept;

void
ScheduleRedraw() noexcept;

void
Create(ContainerWindow &parent, const InfoBoxLayout::Layout &layout,
       const InfoBoxLook &look) noexcept;

void
Destroy() noexcept;

void
Show() noexcept;

void
Hide() noexcept;

/**
 * Opens a dialog to select the content of one InfoBox of @p panel.
 *
 * @return true if the user has chosen a different InfoBox
 */
bool
ShowInfoBoxPicker(InfoBoxSettings::Panel &panel, unsigned i) noexcept;

/**
 * Opens a dialog to select the InfoBox contents for the InfoBox
 * indicated by id, and saves the change to the profile.
 *
 * @param id The id of the InfoBox to configure; nothing happens if it
 * is negative.
 */
void
ShowInfoBoxPicker(int id) noexcept;

/**
 * The InfoBox configuration of the page which is currently shown.
 */
[[gnu::pure]]
InfoBoxSettings::Panel &
GetCurrentPanel() noexcept;

/**
 * Update the InfoBox windows after #GetCurrentPanel() was modified.
 */
void
Refresh() noexcept;

/**
 * Save the configuration of the page which is currently shown to the
 * profile.
 */
void
SaveCurrentPanel() noexcept;

/**
 * Clear focus from all InfoBoxes except the one with the specified ID.
 * This ensures only one InfoBox is selected at any time.
 * @param except_id The InfoBox ID to keep focused (or MAX_CONTENTS to clear all)
 */
void
ClearFocusExcept(unsigned except_id) noexcept;

} // namespace InfoBoxManager
