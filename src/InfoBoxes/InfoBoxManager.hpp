// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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

/**
 * Find the "invisible" InfoBox slot containing the given point.
 *
 * Those slots show the map and their window is hidden, so the map
 * window receives their input events; it uses this to offer the
 * InfoBox picker on a long press (see
 * #GlueMapWindow::OnInfoBoxPickerTimer()).
 *
 * @param p a point in #MainWindow client coordinates
 * @return the InfoBox id, or -1 if there is no invisible slot there
 */
[[gnu::pure]] int
FindInvisibleAt(PixelPoint p) noexcept;

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
 * Opens a dialog to select the InfoBox contents for
 * the InfoBox indicated by id, or the focused InfoBox.
 * @param id The id of the InfoBox to configure.  If negative,
 * then it configures the focused InfoBox if there is one.
 */
void
ShowInfoBoxPicker(const int id = -1) noexcept;

/**
 * Clear focus from all InfoBoxes except the one with the specified ID.
 * This ensures only one InfoBox is selected at any time.
 * @param except_id The InfoBox ID to keep focused (or MAX_CONTENTS to clear all)
 */
void
ClearFocusExcept(unsigned except_id) noexcept;

} // namespace InfoBoxManager
