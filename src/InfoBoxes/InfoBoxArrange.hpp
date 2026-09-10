// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Point.hpp"

/**
 * Touch friendly arranging of the InfoBoxes on the current page.
 *
 * While the arrange mode is active, an overlay covers the screen: it
 * fades out everything behind it, replaces the InfoBoxes by simplified
 * cards and lets one card at a time be dragged onto another one to
 * exchange the two.  The cards are placed on the slots of the InfoBox
 * geometry, not on the current window positions, so every slot of the
 * layout is offered as a drop target.
 */
namespace InfoBoxArrange {

[[gnu::pure]]
bool
IsActive() noexcept;

/**
 * Enter the arrange mode and start dragging the InfoBox @p id, which
 * has just been pressed at @p pointer (in the coordinates of the
 * InfoBox' parent window).
 */
void
Begin(unsigned id, PixelPoint pointer) noexcept;

/**
 * Enter the arrange mode without a press, for example from the menu.
 * The first InfoBox is selected; the cursor keys take over from
 * there.
 */
void
Begin() noexcept;

/**
 * Give the keyboard focus back to the arrange overlay, so that the
 * cursor keys reach it instead of the map.
 *
 * @return false if the arrange mode is not active
 */
bool
SetFocus() noexcept;

/**
 * Leave the arrange mode and save the new order to the profile.
 */
void
Save() noexcept;

/**
 * Leave the arrange mode and restore the order the page had when it
 * was entered.
 */
void
Cancel() noexcept;

/**
 * Forget all state; called when the InfoBox windows are destroyed.
 */
void
Reset() noexcept;

} // namespace InfoBoxArrange
