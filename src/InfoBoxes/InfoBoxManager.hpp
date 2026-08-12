// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxSettings.hpp"
#include "ui/dim/Rect.hpp"

struct InfoBoxLook;
class Canvas;
class ContainerWindow;
class InfoBoxWindow;

namespace InfoBoxLayout { struct Layout; }

namespace InfoBoxManager
{

extern InfoBoxLayout::Layout layout;

/**
 * The layout as the geometry alone defines it, before the contents of
 * the current panel were applied: every slot keeps the rectangle of
 * its own cell, even where an InfoBox has grown over a collapsed
 * neighbour.  #InfoBoxArrange offers one card per cell and therefore
 * uses this instead of #layout.
 */
[[gnu::pure]]
const InfoBoxLayout::Layout &
GetGeometryLayout() noexcept;

/**
 * Expand the given map area rectangle so that it also covers all
 * InfoBox slots of the current panel which are configured as
 * #InfoBoxFactory::e_Invisible.  Those InfoBox windows are never
 * shown, and because the map window is kept at the bottom of the
 * z-order, the map becomes visible in their place.
 *
 * Returns @p rc unmodified if there is no invisible InfoBox.
 */
[[gnu::pure]] PixelRect
ExpandOverInvisible(PixelRect rc) noexcept;

/**
 * Fill the area of all "invisible" InfoBox slots with the InfoBox
 * background colour.
 *
 * Their own window is hidden and usually the map window shows through,
 * but a page which replaces the map by a #Widget leaves this area to
 * no window at all: it would keep whatever pixels happened to be there
 * before.  #MainWindow::OnPaint() calls this for those pages.
 */
void
PaintInvisible(Canvas &canvas, const InfoBoxLook &look) noexcept;

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

[[gnu::pure]]
InfoBoxSettings::Panel &
GetPanel(unsigned index) noexcept;

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

void
SavePanel(unsigned index) noexcept;

/**
 * Clear focus from all InfoBoxes except the one with the specified ID.
 * This ensures only one InfoBox is selected at any time.
 * @param except_id The InfoBox ID to keep focused (or MAX_CONTENTS to clear all)
 */
void
ClearFocusExcept(unsigned except_id) noexcept;

} // namespace InfoBoxManager
