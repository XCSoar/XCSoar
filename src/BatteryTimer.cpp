/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "BatteryTimer.hpp"
#include "Hardware/Battery.hpp"
#include "Hardware/PowerInfo.hpp"
#include "Hardware/PowerGlobal.hpp"
#include "UIActions.hpp"
#include "LogFile.hpp"
#include "Simulator.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"

void
BatteryTimer::Process()
{
#ifdef HAVE_BATTERY
  // TODO feature: Trigger a GCE (Glide Computer Event) when
  // switching to battery mode This can be used to warn users that
  // power has been lost and you are now on battery power - ie:
  // something else is wrong

#ifndef ANDROID
  Power::global_info = Power::GetInfo();
#endif

  const auto &info = Power::global_info;
  const auto &battery = info.battery;
  const auto &external = info.external;

  /* Battery status - simulator only - for safety of battery data
     note: Simulator only - more important to keep running in your plane
  */

  // JMW, maybe this should be active always...
  // we don't want the PDA to be completely depleted.

  if (external.status == Power::ExternalInfo::Status::OFF) {
    if (is_simulator() && battery.remaining_percent &&
        *battery.remaining_percent < BATTERY_EXIT) {
      LogFormat("Battery low exit...");
      // TODO feature: Warning message on battery shutdown
      UIActions::SignalShutdown(true);
    } else {
      if (battery.remaining_percent &&
          *battery.remaining_percent < BATTERY_WARNING) {
        if (last_warning.CheckUpdate(BATTERY_REMINDER))
          // TODO feature: Show the user what the batt status is.
          Message::AddMessage(_("Battery low"));
      } else {
        last_warning.Reset();
      }
    }
  }
#endif /* HAVE_BATTERY */
}
