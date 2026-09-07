// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Copies the operating system's appearance setting to
 * GlobalSettings::dark_mode.
 *
 * macOS and iOS may switch the appearance while XCSoar is running,
 * e.g. on their sunset-to-sunrise schedule, and neither sends the
 * application a notification our event loop could see.  Therefore this
 * must be called periodically while UISettings::DarkMode::AUTO is
 * selected, and once before the look is rebuilt after the user has
 * selected it.
 *
 * @return true if the value has changed
 */
bool
UpdateAppleDarkMode() noexcept;
