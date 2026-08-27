// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

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

/**
 * The kind of event the haptic feedback belongs to.  Each platform
 * maps these to the strength and the pattern which is customary
 * there.
 */
enum class HapticFeedbackType : uint_least8_t {
  /** the selection moved to another item */
  SELECTION,

  /** a button or an InfoBox was pressed */
  PRESS,

  /** a long press was recognised */
  LONG_PRESS,

  /** a gesture was recognised */
  GESTURE,

  /** a message was shown to the user */
  NOTIFICATION,
};

#ifdef HAVE_VIBRATOR

/**
 * Check whether this device has a vibrator.
 */
[[gnu::const]]
bool
HaveVibrator() noexcept;

/**
 * Generate haptic feedback for the given event.  This function has no
 * effect if the device does not have a vibrator or if the user has
 * disabled haptic feedback.
 */
void
Vibrate(HapticFeedbackType type) noexcept;

#else

static constexpr bool
HaveVibrator() noexcept
{
  return false;
}

#endif
