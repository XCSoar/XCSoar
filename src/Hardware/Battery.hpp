/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#if defined(_WIN32_WCE) && !defined(GNAV)
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

#include <windows.h>

// Battery status for SIMULATOR mode
//	30% reminder, 20% exit, 30 second reminders on warnings
#define BATTERY_WARNING 30
#define BATTERY_EXIT 20
#define BATTERY_REMINDER 30000

extern DWORD BatteryWarningTime;

void
UpdateBatteryInfo(void);

#endif /* !HAVE_BATTERY */

#endif
