/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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
