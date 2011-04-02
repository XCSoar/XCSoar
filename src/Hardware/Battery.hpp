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

#ifndef XCSOAR_HARDWARE_BATTERY_H
#define XCSOAR_HARDWARE_BATTERY_H

#if defined(ANDROID) || (defined(_WIN32_WCE) && !defined(GNAV))
#define HAVE_BATTERY

#include <stdbool.h>

namespace Power
{
  namespace Battery{
    enum batterystatus {
      LOW,
      HIGH,
      CRITICAL,
      CHARGING,
      NOBATTERY,
      UNKNOWN
    };

    extern unsigned Temperature;
    extern unsigned RemainingPercent;
    extern bool RemainingPercentValid;
    extern batterystatus Status;
  };
  namespace External{
    enum externalstatus{
      OFF,
      ON,
      UNKNOWN
    };

    extern externalstatus Status;
  };

}

#ifdef ANDROID

static inline void
UpdateBatteryInfo()
{
  /* nothing to do, this is updated by Android callbacks */
}

#else /* _WIN32_WCE */

#include <windows.h>

// Battery status for SIMULATOR mode
//	30% reminder, 20% exit, 30 second reminders on warnings
#define BATTERY_WARNING 30
#define BATTERY_EXIT 20
#define BATTERY_REMINDER 30000

extern DWORD BatteryWarningTime;

void
UpdateBatteryInfo(void);

#endif /* _WIN32_WCE */

#endif /* !HAVE_BATTERY */

#endif
