// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BatteryTimer.hpp"
#include "Hardware/Battery.hpp"
#include "Hardware/PowerInfo.hpp"
#include "Hardware/PowerGlobal.hpp"
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

  if (external.status == Power::ExternalInfo::Status::OFF) {
    if (!battery.remaining_percent || *battery.remaining_percent <= 0)
      return;

    if (*battery.remaining_percent < BATTERY_WARNING) {
      if (last_warning.CheckUpdate(BATTERY_REMINDER))
        // TODO feature: Show the user what the batt status is.
        Message::AddMessage(_("Battery low"));
    } else {
      last_warning.Reset();
    }
  }
#endif /* HAVE_BATTERY */
}
