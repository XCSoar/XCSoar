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

void
VibrateShort() noexcept
{
#ifdef ANDROID
  if (vibrator != nullptr)
    vibrator->Vibrate(Java::GetEnv(), 25);
#else
  Apple::VibrateShort();
#endif
}

#endif /* HAVE_VIBRATOR */
