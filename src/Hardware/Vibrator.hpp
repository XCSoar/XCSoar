// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

/**
 * The macro HAVE_VIBRATOR specifies whether this platform has support
 * for a vibrator.  Before actually using it, you have to check
 * HaveVibrator().
 */
#ifdef ANDROID
#define HAVE_VIBRATOR
#elif defined(__APPLE__)
#if TARGET_OS_IPHONE
/* iOS generates haptic feedback with UIFeedbackGenerator */
#define HAVE_VIBRATOR
#endif
#endif

#ifdef HAVE_VIBRATOR

/**
 * Check whether this device has a vibrator.
 */
[[gnu::const]]
bool
HaveVibrator() noexcept;

/**
 * Vibrate for a very short amount of time.  This function has no
 * effect if the device does not have a vibrator.
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
