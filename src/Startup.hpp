// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

namespace UI { class Display; }

bool
Startup(UI::Display &display);

/**
 * True if Startup() returned false because the USER chose to quit
 * (startup profile dialog cancelled, simulator prompt "Quit", quick
 * guide / warranty declined) - as opposed to a real startup failure.
 * Lets the caller exit with EXIT_SUCCESS for a deliberate quit.
 */
[[gnu::pure]]
bool
WasStartupCancelledByUser() noexcept;

/**
 * Write all user state which is only kept in memory (the profile
 * settings and the FLARM databases) to disk.  Apart from the regular
 * shutdown, this is called whenever the operating system may kill the
 * process without notifying us again, e.g. when iOS suspends the app.
 */
void
SaveUserState() noexcept;

void
Shutdown();
