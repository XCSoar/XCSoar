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

// created 20070428 sgi, basic NMEA sentance supported
// WARNING PosiGraph dont send any GGA sentance
// GGA sentance trigger nav process in XCSoar
// Posigraph driver trigger Nav process with RMC sentance (if driver is devA())

// ToDo

// adding baro alt sentance paser to support baro source priority  if (d == pDevPrimaryBaroSource){...}

#include "Device/devPosiGraph.h"
#include "Device/device.h"
#include "Math/FastMath.h"
#include "Math/Pressure.h"
#include "Device/Parser.h"
#include "Device/Port.h"
#include "UtilsText.hpp"
#include "NMEA/Info.h"

#include <tchar.h>

static BOOL GPWIN(PDeviceDescriptor_t d, const TCHAR *String,
                  NMEA_INFO *GPS_INFO);

BOOL PGParseNMEA(PDeviceDescriptor_t d, const TCHAR *String,
                 NMEA_INFO *GPS_INFO)
{
  (void)d;
  (void)String;
  (void)GPS_INFO;

  // $GPWIN ... Winpilot proprietary sentance includinh baro altitude
  // $GPWIN ,01900 , 0 , 5159 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 * 6 B , 0 7 * 6 0 E
  if(_tcsncmp(TEXT("$GPWIN"), String, 6)==0)
    {
      return GPWIN(d, &String[7], GPS_INFO);
    }

  return FALSE;

}


BOOL PGDeclare(PDeviceDescriptor_t d, Declaration_t *decl){

  (void)d;
  (void)decl;

  return(TRUE);
}

const struct DeviceRegister pgDevice = {
  TEXT("PosiGraph Logger"),
  drfGPS | drfBaroAlt | drfLogger,
  PGParseNMEA,			// ParseNMEA
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
  PGDeclare,			// Declare
  NULL,				// IsLogger
  NULL,				// IsGPSSource
  NULL,				// IsBaroSource
  NULL				// OnSysTicker
};

// *****************************************************************************
// local stuff

static BOOL GPWIN(PDeviceDescriptor_t d, const TCHAR *String,
                  NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];
  (void)GPS_INFO;
  (void)d;

  NMEAParser::ExtractParameter(String, ctemp, 2);

  if (d == pDevPrimaryBaroSource){
    GPS_INFO->BaroAltitudeAvailable = TRUE;
    GPS_INFO->BaroAltitude = AltitudeToQNHAltitude(iround(StrToDouble(ctemp, NULL)/10));
  }

  return FALSE;

}
