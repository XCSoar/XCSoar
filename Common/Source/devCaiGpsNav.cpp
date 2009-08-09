// $Id$

/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008

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

//


// CAUTION!
// caiGpsNavParseNMEA is called from com port read thread
// all other functions are called from windows message loop thread


#define  LOGSTREAM 0


#include <windows.h>
#include <tchar.h>


#include "externs.h"
#include "Utils.h"
#include "Parser.h"
#include "Port.h"

#include "devCaiGpsNav.h"

#ifdef _SIM_
static BOOL fSimMode = TRUE;
#else
static BOOL fSimMode = FALSE;
#endif


#define  CtrlC  0x03
#define  swap(x)      x = ((((x<<8) & 0xff00) | ((x>>8) & 0x00ff)) & 0xffff)


BOOL caiGpsNavOpen(PDeviceDescriptor_t d, int Port){

  if (!fSimMode){
	  d->Com->WriteString(TEXT("\x03"));
	  Sleep(50);
	  d->Com->WriteString(TEXT("NMEA\r"));

	  // This is for a slightly different mode, that
	  // apparently outputs pressure info too...
	  //(d->Com.WriteString)(TEXT("PNP\r\n"));
	  //(d->Com.WriteString)(TEXT("LOG 0\r\n"));
  }

  return(TRUE);
}


static const DeviceRegister_t caiGpsNavDevice = {
  TEXT("CAI GPS-NAV"),
  drfGPS,
  NULL,				// ParseNMEA
  NULL,				// PutMacCready
  NULL,				// PutBugs
  NULL,				// PutBallast
  NULL,				// PutQNH
  NULL,				// PutVoice
  NULL,				// PutVolume
  NULL,				// PutFreqActive
  NULL,				// PutFreqStandby
  caiGpsNavOpen,		// Open
  NULL,				// Close
  NULL,				// LinkTimeout
  NULL,				// Declare
  NULL,				// IsLogger - TODO feature: CAI GPS NAV declaration
  NULL,				// IsGPSSource
  NULL,				// IsBaroSource
  NULL,				// IsRadio
  NULL				// OnSysTicker
};

BOOL caiGpsNavRegister(void){
  return devRegister(&caiGpsNavDevice);
}

