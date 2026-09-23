// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Vibrator.hpp"

#ifdef HAVE_VIBRATOR

#ifdef ANDROID
#include "Android/Vibrator.hpp"
#include "Android/Main.hpp"
#include "java/Global.hxx"
#else
#include "Apple/Vibrator.hpp"
#endif

bool
HaveVibrator() noexcept
{
#ifdef ANDROID
  return vibrator != nullptr;
#else
  return Apple::HaveHapticFeedback();
#endif
}

#ifdef ANDROID

/**
 * Android exposes only a plain vibration motor here, and the feedback
 * types are approximated with different pulse lengths.
 */
static constexpr unsigned
GetVibrationDuration(HapticFeedbackType type) noexcept
{
  switch (type) {
  case HapticFeedbackType::SELECTION:
    return 15;

  case HapticFeedbackType::PRESS:
    return 25;

  case HapticFeedbackType::LONG_PRESS:
  case HapticFeedbackType::GESTURE:
    return 40;

  case HapticFeedbackType::NOTIFICATION:
    return 150;

  case HapticFeedbackType::ALARM:
    return 400;
  }

  return 25;
}

#endif /* ANDROID */

void
Vibrate(HapticFeedbackType type) noexcept
{
#ifdef ANDROID
  if (vibrator != nullptr)
    vibrator->Vibrate(Java::GetEnv(), GetVibrationDuration(type));
#else
  Apple::Vibrate(type);
#endif
}

#endif /* HAVE_VIBRATOR */
