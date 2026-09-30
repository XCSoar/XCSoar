// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * The display name of a repository area code ("de", "fr", ...): the
 * country's name, marked with N_() - pass it through gettext().
 * Returns nullptr for a code the table does not know; the caller
 * shows the bare code then.
 */
[[gnu::pure]]
const char *
GetAreaDisplayName(const char *area) noexcept;
