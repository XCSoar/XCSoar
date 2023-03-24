// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Vibrator.hpp"

#ifdef ANDROID

#include "Android/Vibrator.hpp"
#include "Android/Main.hpp"
#include "Interface.hpp"
#include "UISettings.hpp"

bool
HaveVibrator()
{
  return vibrator != nullptr;
}

void
VibrateShort()
{
  if (vibrator != nullptr) {
    const UISettings &ui_settings = CommonInterface::GetUISettings();
    if (ui_settings.haptic_feedback == UISettings::HapticFeedback::ON ||
         (ui_settings.haptic_feedback == UISettings::HapticFeedback::DEFAULT &&
          vibrator->IsOSHapticFeedbackEnabled()))
      vibrator->Vibrate(Java::GetEnv(), 25);
  }
}

#endif /* Android */
