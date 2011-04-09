/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Screen/Blank.hpp"

#ifdef HAVE_BLANK

#include "Interface.hpp"
#include "Hardware/Battery.hpp"
#include "Hardware/Display.hpp"
#include "Dialogs/Dialogs.h"
#include "LogFile.hpp"
#include "Message.hpp"
#include "Simulator.hpp"
#include "Language.hpp"
#include "Sizes.h"

int DisplayTimeOut = 0;

void
BlankDisplay(bool doblank)
{
  static bool oldblank = false;

  UpdateBatteryInfo();

  if (!XCSoarInterface::SettingsMap().EnableAutoBlank)
    return;

  if (doblank == oldblank)
    return;

  if (!Display::BlankSupported())
    // can't do it, not supported
    return;

  // TODO feature: Trigger a GCE (Glide Computer Event) when
  // switching to battery mode This can be used to warn users that
  // power has been lost and you are now on battery power - ie:
  // something else is wrong

  if (doblank) {
    /* Battery status - simulator only - for safety of battery data
       note: Simulator only - more important to keep running in your plane
    */

    // JMW, maybe this should be active always...
    // we don't want the PDA to be completely depleted.

    if (Power::External::Status == Power::External::OFF) {
      if (is_simulator() && Power::Battery::RemainingPercentValid &&
          Power::Battery::RemainingPercent < BATTERY_EXIT) {
        LogStartUp(TEXT("Battery low exit..."));
        // TODO feature: Warning message on battery shutdown
        XCSoarInterface::SignalShutdown(true);
      } else {
        if (Power::Battery::RemainingPercentValid &&
            Power::Battery::RemainingPercent < BATTERY_WARNING) {
          DWORD LocalWarningTime = ::GetTickCount();
          if ((LocalWarningTime - BatteryWarningTime) > BATTERY_REMINDER) {
            BatteryWarningTime = LocalWarningTime;
            // TODO feature: Show the user what the batt status is.
            Message::AddMessage(_("Organiser Battery Low"));
          }
        } else {
          BatteryWarningTime = 0;
        }
      }
    }

    if (Power::External::Status == Power::External::OFF) {
      // Power off the display
      Display::Blank(true);
      oldblank = true;
      XCSoarInterface::SetSettingsMap().ScreenBlanked = true;
    } else {
      ResetDisplayTimeOut();
    }
  } else if (oldblank) {
    // was blanked
    // Power on the display
    Display::Blank(false);
    oldblank = false;
    XCSoarInterface::SetSettingsMap().ScreenBlanked = false;
  }
}

void
CheckDisplayTimeOut(bool sticky)
{
#if defined(WIN32) && !defined(_WIN32_WCE)
  SystemIdleTimerReset();
#endif

  if (!sticky) {
    if (DisplayTimeOut < DISPLAYTIMEOUTMAX)
      DisplayTimeOut++;
  } else {
    // JMW don't let display timeout while a dialog is active,
    // but allow button presses to trigger redisplay
    if (DisplayTimeOut > 1)
      DisplayTimeOut = 1;
  }
  if (DisplayTimeOut >= DISPLAYTIMEOUTMAX)
    BlankDisplay(true);
  else
    BlankDisplay(false);
}

#endif /* HAVE_BLANK */
