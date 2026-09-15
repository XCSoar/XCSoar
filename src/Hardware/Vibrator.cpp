// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Vibrator.hpp"

#ifdef ANDROID

#include "Android/Vibrator.hpp"
#include "Android/Main.hpp"

bool
HaveVibrator() noexcept
{
  return vibrator != nullptr;
}

void
VibrateShort() noexcept
{
  if (vibrator != nullptr)
    vibrator->Vibrate(Java::GetEnv(), 25);
}

#endif /* Android */
