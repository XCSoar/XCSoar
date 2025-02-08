// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
    if (!battery.remaining_percent || *battery.remaining_percent <= 0) {
      return;
    }

    if (is_simulator() && battery.remaining_percent &&
        *battery.remaining_percent < BATTERY_EXIT) {
      LogString("Battery low exit...");
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
