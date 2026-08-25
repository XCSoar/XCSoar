// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

namespace UI { class Display; }

bool
Startup(UI::Display &display);

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
