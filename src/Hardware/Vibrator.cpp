// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Vibrator.hpp"

#ifdef HAVE_VIBRATOR

#include "Interface.hpp"
#include "UISettings.hpp"
#include "GlobalSettings.hpp"

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

/**
 * Has the user enabled haptic feedback?
 */
[[gnu::pure]]
static bool
IsHapticFeedbackEnabled() noexcept
{
  switch (CommonInterface::GetUISettings().haptic_feedback) {
  case UISettings::HapticFeedback::DEFAULT:
    return GlobalSettings::haptic_feedback;

  case UISettings::HapticFeedback::OFF:
    return false;

  case UISettings::HapticFeedback::ON:
    return true;
  }

  return false;
}

void
VibrateShort() noexcept
{
  if (!IsHapticFeedbackEnabled())
    return;

#ifdef ANDROID
  if (vibrator != nullptr)
    vibrator->Vibrate(Java::GetEnv(), 25);
#else
  Apple::VibrateShort();
#endif
}

#endif /* HAVE_VIBRATOR */
