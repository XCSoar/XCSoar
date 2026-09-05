// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Point.hpp"

#include <chrono>

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

/**
 * How long a still press must last before lift-off opens the
 * picker, or, on a live InfoBox, the arrange mode.  Sliding off
 * the target cancels.  Movement past the touch slop before this
 * starts a drag instead.
 */
constexpr auto LONG_PRESS = std::chrono::milliseconds(500);

/**
 * Past this, the press is a hold, not a tap.  The fill starts
 * here and reaches full at #LONG_PRESS.
 */
constexpr auto TAP = std::chrono::milliseconds(150);

/** How often the long-press fill is redrawn while it grows.
 *  64 ms is about four 60 Hz frames; 32 ms fought vsync and
 *  each tick presents the whole OpenGL window. */
constexpr auto LONG_PRESS_FADE = std::chrono::milliseconds(64);

/**
 * 0…256 from #TAP to #LONG_PRESS.  Zero while the press can
 * still be a tap.
 */
[[gnu::pure]]
inline unsigned
LongPressFade(std::chrono::steady_clock::duration elapsed) noexcept
{
  if (elapsed < TAP)
    return 0;
  if (elapsed >= LONG_PRESS)
    return 256;

  return unsigned((elapsed - TAP) * 256 / (LONG_PRESS - TAP));
}

inline unsigned
LongPressFade(std::chrono::steady_clock::time_point start) noexcept
{
  return LongPressFade(std::chrono::steady_clock::now() - start);
}

/** Has the pointer moved far enough to win over a tap or hold? */
[[gnu::pure]]
bool
PastTouchSlop(PixelPoint a, PixelPoint b) noexcept;

[[gnu::pure]]
bool
IsActive() noexcept;

/**
 * Enter the arrange mode with InfoBox @p id selected.  Called
 * after lift-off from a hold.
 */
void
Begin(unsigned id) noexcept;

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
