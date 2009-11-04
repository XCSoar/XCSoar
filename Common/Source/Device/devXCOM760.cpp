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

#include "Device/devXCOM760.h"
#include "Device/device.h"
#include "Device/Port.h"

#include <tchar.h>

static BOOL XCOM760PutVolume(PDeviceDescriptor_t d, int Volume) {
  TCHAR  szTmp[32];
  _stprintf(szTmp, TEXT("$RVOL=%d\r\n"), Volume);
  d->Com->WriteString(szTmp);
  return(TRUE);
}


static BOOL XCOM760PutFreqActive(PDeviceDescriptor_t d, double Freq) {
  TCHAR  szTmp[32];
  _stprintf(szTmp, TEXT("$TXAF=%.3f\r\n"), Freq);
  d->Com->WriteString(szTmp);
  return(TRUE);
}


static BOOL XCOM760PutFreqStandby(PDeviceDescriptor_t d, double Freq) {
  TCHAR  szTmp[32];
  _stprintf(szTmp, TEXT("$TXSF=%.3f\r\n"), Freq);
  d->Com->WriteString(szTmp);
  return(TRUE);
}


static const DeviceRegister_t xcom760Device = {
  TEXT("XCOM760"),
  drfRadio,
  NULL,				// ParseNMEA
  NULL,				// PutMacCready
  NULL,				// PutBugs
  NULL,				// PutBallast
  NULL,				// PutQNH
  NULL,				// PutVoice
  XCOM760PutVolume,		// PutVolume
  XCOM760PutFreqActive,		// PutFreqActive
  XCOM760PutFreqStandby,	// PutFreqStandby
  NULL,				// Open
  NULL,				// Close
  NULL,				// LinkTimeout
  NULL,				// Declare
  NULL,				// IsLogger
  NULL,				// IsGPSSource
  NULL,				// IsBaroSource
  NULL,				// OnSysTicker
  NULL                          // OnThermal
};

bool xcom760Register(void){
  return devRegister(&xcom760Device);
}


/* Commands

  $TOGG: return to main screen or toggle active and standby
  $DUAL=on/off
*/
