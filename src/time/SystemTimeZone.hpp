// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Determine the local time zone offset [seconds] which is currently in
 * effect, i.e. including daylight saving time.
 *
 * Unlike GetTimeZoneOffset(), the return value is not constant: it
 * changes at daylight saving time transitions and whenever the
 * operating system's time zone is reconfigured (e.g. while
 * travelling).  Therefore it must not be cached.
 */
int
GetCurrentTimeZoneOffset() noexcept;
