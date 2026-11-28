// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

/**
 * Manages "device.xcd" in the XCSoarData directory.
 *
 * This file stores device-local state that is shared by all profiles:
 * things that describe this XCSoar installation rather than a pilot's
 * settings, e.g. whether to start in fly or simulator mode.  Unlike a
 * profile, it is not meant to be copied to other devices, and changing
 * it never touches any (possibly shareable) profile file.
 */
namespace DeviceFile {

/**
 * Which mode XCSoar starts in; ASK shows the fly/simulator prompt.
 * Ignored when "-fly" or "-simulator" was passed on the command line.
 */
enum class StartupMode : uint_least8_t {
  ASK,
  FLY,
  SIMULATOR,
};

/**
 * Load device.xcd into memory.  A missing file is not an error;
 * it leaves all values at their defaults.
 */
void
Load() noexcept;

/**
 * Write the in-memory state back to device.xcd if it was
 * modified.  Errors will be caught and logged.
 */
void
Save() noexcept;

[[gnu::pure]]
StartupMode
GetStartupMode() noexcept;

/**
 * Update the startup mode and persist it (calls Save()).
 */
void
SetStartupMode(StartupMode mode) noexcept;

/**
 * MSAA sample count for this computer's screen (0 = off, or 2, 4, 8,
 * 16).  Missing or unrecognised values are off.  This is a property
 * of the display, so it lives here rather than in a shareable
 * profile (#1993).
 */
[[gnu::pure]]
unsigned
GetAntialiasing() noexcept;

/**
 * Store the sample count and persist it (calls Save()).  A value
 * that is not one of the counts above is stored as off.
 */
void
SetAntialiasing(unsigned samples) noexcept;

} // namespace DeviceFile
