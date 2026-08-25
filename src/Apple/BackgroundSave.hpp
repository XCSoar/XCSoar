// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Start writing the user's settings to disk whenever iOS moves the app
 * to the background.  A backgrounded app gets suspended, and a
 * suspended app may be killed - by the user swiping it up in the app
 * switcher, or by the system reclaiming memory - without ever running
 * again, i.e. without another chance to persist anything.
 *
 * This is a no-op on macOS, which does not suspend applications.
 */
void
InitializeAppleBackgroundSave() noexcept;

void
DeinitializeAppleBackgroundSave() noexcept;
