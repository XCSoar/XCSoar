// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef ANDROID

/**
 * This macro specifies whether this platform has support for a
 * vibrator.  Before actually using it, you have to check
 * HaveVibrator().
 */
#define HAVE_VIBRATOR

/**
 * Check whether this device has a vibrator.
 */
[[gnu::const]]
bool
HaveVibrator() noexcept;

/**
 * Vibrate for a very short amount of time.  This function has no
 * effect if the device does not have a vibrator.  The caller must
 * check the haptic-feedback setting before calling.
 */
void
VibrateShort() noexcept;

#else

static constexpr bool
HaveVibrator() noexcept
{
  return false;
}

#endif
