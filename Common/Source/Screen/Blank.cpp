/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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
#include "XCSoar.h"

#ifdef HAVE_BLANK

#include "Interface.hpp"
#include "Battery.h"
#include "Dialogs.h"
#include "LogFile.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// GDI Escapes for ExtEscape()
#define QUERYESCSUPPORT    8
// The following are unique to CE
#define GETVFRAMEPHYSICAL   6144
#define GETVFRAMELEN    6145
#define DBGDRIVERSTAT    6146
#define SETPOWERMANAGEMENT   6147
#define GETPOWERMANAGEMENT   6148

typedef enum _VIDEO_POWER_STATE {
    VideoPowerOn = 1,
    VideoPowerStandBy,
    VideoPowerSuspend,
    VideoPowerOff
} VIDEO_POWER_STATE, *PVIDEO_POWER_STATE;

typedef struct _VIDEO_POWER_MANAGEMENT {
    ULONG Length;
    ULONG DPMSVersion;
    ULONG PowerState;
} VIDEO_POWER_MANAGEMENT, *PVIDEO_POWER_MANAGEMENT;

bool EnableAutoBlank = false;
int DisplayTimeOut = 0;
bool ScreenBlanked = false;
bool ForceShutdown = false;

void BlankDisplay(bool doblank) {
  static bool oldblank = false;

  BATTERYINFO BatteryInfo;
  BatteryInfo.acStatus = 0; // JMW initialise

  if (GetBatteryInfo(&BatteryInfo)) {
    PDABatteryPercent = BatteryInfo.BatteryLifePercent;
    PDABatteryTemperature = BatteryInfo.BatteryTemperature; // VENTA3
/*
	// All you need to display extra Battery informations...

	TCHAR vtemp[1000];
	_stprintf(vtemp,_T("Battpercent=%d Volt=%d Curr=%d AvCurr=%d mAhC=%d Temp=%d Lifetime=%d Fulllife=%d\n"),
 		BatteryInfo.BatteryLifePercent, BatteryInfo.BatteryVoltage,
 		BatteryInfo.BatteryCurrent, BatteryInfo.BatteryAverageCurrent,
 		BatteryInfo.BatterymAHourConsumed,
		BatteryInfo.BatteryTemperature, BatteryInfo.BatteryLifeTime, BatteryInfo.BatteryFullLifeTime);
	StartupStore( vtemp );
 */
  }

  if (!EnableAutoBlank) {
    return;
  }
  if (doblank == oldblank) {
    return;
  }

  HDC gdc;
  int iESC=SETPOWERMANAGEMENT;

  gdc = ::GetDC(NULL);
  if (ExtEscape(gdc, QUERYESCSUPPORT, sizeof(int), (LPCSTR)&iESC,
                0, NULL)==0) {
    // can't do it, not supported
  } else {

    VIDEO_POWER_MANAGEMENT vpm;
    vpm.Length = sizeof(VIDEO_POWER_MANAGEMENT);
    vpm.DPMSVersion = 0x0001;

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

      if (BatteryInfo.acStatus==0) {
#ifdef _SIM_
        if ((PDABatteryPercent < BATTERY_EXIT) && !ForceShutdown) {
          StartupStore(TEXT("Battery low exit...\n"));
          // TODO feature: Warning message on battery shutdown
	  ForceShutdown = true;
          SendMessage(hWndMainWindow, WM_CLOSE, 0, 0);
        } else
#endif
          if (PDABatteryPercent < BATTERY_WARNING) {
            DWORD LocalWarningTime = ::GetTickCount();
            if ((LocalWarningTime - BatteryWarningTime) > BATTERY_REMINDER) {
              BatteryWarningTime = LocalWarningTime;
              // TODO feature: Show the user what the batt status is.
              DoStatusMessage(TEXT("Organiser Battery Low"));
            }
          } else {
            BatteryWarningTime = 0;
          }
      }

      if (BatteryInfo.acStatus==0) {

        // Power off the display
        vpm.PowerState = VideoPowerOff;
        ExtEscape(gdc, SETPOWERMANAGEMENT, vpm.Length, (LPCSTR) &vpm,
                  0, NULL);
        oldblank = true;
        ScreenBlanked = true;
      } else {
        ResetDisplayTimeOut();
      }
    } else {
      if (oldblank) { // was blanked
        // Power on the display
        vpm.PowerState = VideoPowerOn;
        ExtEscape(gdc, SETPOWERMANAGEMENT, vpm.Length, (LPCSTR) &vpm,
                  0, NULL);
        oldblank = false;
        ScreenBlanked = false;
      }
    }

  }
  ::ReleaseDC(NULL, gdc);
}

void CheckDisplayTimeOut(bool sticky)
{
  if (DisplayTimeOut >= DISPLAYTIMEOUTMAX) {
    BlankDisplay(true);
  } else {
    BlankDisplay(false);
  }
  if (!sticky) {
    DisplayTimeOut++;
  } else {
    // JMW don't let display timeout while a dialog is active,
    // but allow button presses to trigger redisplay
    if (DisplayTimeOut>1) {
      DisplayTimeOut=1;
    }
  }
}

#endif /* HAVE_BLANK */
