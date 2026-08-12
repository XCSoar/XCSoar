// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <string>

struct PixelRect;

namespace UI::TextInput {

/**
 * Does the operating system provide an on-screen keyboard (e.g. on
 * iOS)?  If it does, ShowScreenKeyboard() can be used instead of
 * XCSoar's own #KeyboardWidget.
 */
[[gnu::pure]]
bool
HasScreenKeyboard() noexcept;

/**
 * Show the operating system's on-screen keyboard.  Characters typed
 * on it are delivered as regular character events.  Does nothing if
 * HasScreenKeyboard() is false (i.e. on desktops, where switching
 * text input off again would break the physical keyboard).
 */
void
ShowScreenKeyboard() noexcept;

/**
 * Hide the on-screen keyboard shown by ShowScreenKeyboard().
 */
void
HideScreenKeyboard() noexcept;

/**
 * Tell the operating system where the text is being edited, so it can
 * avoid covering it with the on-screen keyboard.  Best effort only.
 */
void
SetScreenKeyboardRect(const PixelRect &rc) noexcept;

/**
 * Can this build read the system clipboard?
 */
[[gnu::const]]
bool
HasClipboard() noexcept;

/**
 * Returns the clipboard contents; an empty string if the clipboard is
 * empty or unavailable.
 */
std::string
GetClipboardText();

} // namespace UI::TextInput
