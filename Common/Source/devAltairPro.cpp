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

#include "StdAfx.h"


#include "externs.h"
#include "Utils.h"
#include "Parser.h"
#include "Port.h"
#include "Math/Pressure.h"

#include "devAltairPro.h"

double AltitudeToQNHAltitude(double alt);

static double lastAlt = 0;


BOOL atrParseNMEA(PDeviceDescriptor_t d, const TCHAR *String,
                  NMEA_INFO *GPS_INFO)
{

  // no propriatary sentence

  if (_tcsncmp(TEXT("$PGRMZ"), String, 6) == 0){
    // eat $PGRMZ

    String = _tcschr(String + 6, ',');
    if (String == NULL)
      return FALSE;

    ++String;

    // get <alt>

    lastAlt = StrToDouble(String, NULL);

    String = _tcschr(String, ',');
    if (String == NULL)
      return FALSE;

    ++String;

    // get <unit>

    if (*String == 'f' || *String== 'F')
      lastAlt /= TOFEET;


    if (d == pDevPrimaryBaroSource){
      GPS_INFO->BaroAltitudeAvailable = TRUE;
      GPS_INFO->BaroAltitude = AltitudeToQNHAltitude(lastAlt);
    }

    return(TRUE);
  }

  return FALSE;

}


BOOL atrDeclare(PDeviceDescriptor_t d, Declaration_t *decl){
  (void) d;
  (void) decl;

  // TODO feature: Altair declaration

  return(TRUE);

}


BOOL atrPutQNH(DeviceDescriptor_t *d, double NewQNH){
  (void)NewQNH; // TODO code: JMW check sending QNH to Altair
  if (d == pDevPrimaryBaroSource){
    GPS_INFO.BaroAltitude = AltitudeToQNHAltitude(lastAlt);
  }

  return(TRUE);
}

BOOL atrOnSysTicker(DeviceDescriptor_t *d){
  (void)d;
  // Do To get IO data like temp, humid, etc

  return(TRUE);
}

static const DeviceRegister_t atrDevice = {
  TEXT("Altair Pro"),
  drfGPS | drfBaroAlt, // drfLogger - ToDo
  atrParseNMEA,			// ParseNMEA
  NULL,				// PutMacCready
  NULL,				// PutBugs
  NULL,				// PutBallast
  atrPutQNH,			// PutQNH
  NULL,				// PutVoice
  NULL,				// PutVolume
  NULL,				// PutFreqActive
  NULL,				// PutFreqStandby
  NULL,				// Open
  NULL,				// Close
  NULL,				// LinkTimeout
  atrDeclare,			// Declare
  NULL,				// IsLogger
  NULL,				// IsGPSSource
  NULL,				// IsBaroSource
  atrOnSysTicker		// OnSysTicker
};

BOOL atrRegister(void){
  return devRegister(&atrDevice);
}

