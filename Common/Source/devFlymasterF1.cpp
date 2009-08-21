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
#include "Math/Pressure.h"
#include "Utils.h"
#include "Parser.h"
#include "Port.h"

#include "devFlymasterF1.h"

#include <tchar.h>

static BOOL VARIO(PDeviceDescriptor_t d, const TCHAR *String,
                  NMEA_INFO *GPS_INFO);

static BOOL FlymasterF1ParseNMEA(PDeviceDescriptor_t d, const TCHAR *String,
                                 NMEA_INFO *GPS_INFO){
  (void)d;

  if(_tcsncmp(TEXT("$VARIO"), String, 6)==0)
    {
      return VARIO(d, &String[7], GPS_INFO);
    }

  return FALSE;

}


static const DeviceRegister_t flymasterf1Device = {
  TEXT("FlymasterF1"),
  drfGPS | drfBaroAlt | drfVario,
  FlymasterF1ParseNMEA,		// ParseNMEA
  NULL,				// PutMacCready
  NULL,				// PutBugs
  NULL,				// PutBallast
  NULL,				// PutQNH
  NULL,				// PutVoice
  NULL,				// PutVolume
  NULL,				// PutFreqActive
  NULL,				// PutFreqStandby
  NULL,				// Open
  NULL,				// Close
  NULL,				// LinkTimeout
  NULL,				// Declare
  NULL,				// IsLogger
  NULL,				// IsGPSSource
  NULL,				// IsBaroSource
  NULL				// OnSysTicker
};

BOOL flymasterf1Register(void){
  return devRegister(&flymasterf1Device);
}


// *****************************************************************************
// local stuff

static BOOL VARIO(PDeviceDescriptor_t d, const TCHAR *String,
                  NMEA_INFO *GPS_INFO)
{
  // $VARIO,fPressure,fVario,Bat1Volts,Bat2Volts,BatBank,TempSensor1,TempSensor2*CS

  TCHAR ctemp[80];
  NMEAParser::ExtractParameter(String,ctemp,0);
  double ps = StrToDouble(ctemp,NULL);
  GPS_INFO->BaroAltitude = (1 - pow(fabs(ps / QNH),  0.190284)) * 44307.69;
  NMEAParser::ExtractParameter(String,ctemp,1);
  GPS_INFO->Vario = StrToDouble(ctemp,NULL)/10.0;
  // JMW vario is in dm/s

  GPS_INFO->VarioAvailable = TRUE;
  GPS_INFO->BaroAltitudeAvailable = TRUE;

  TriggerVarioUpdate();

  return TRUE;
}
