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

#include "Device/devCondor.h"
#include "Device/Internal.hpp"
#include "Device/device.h"
#include "XCSoar.h"
#include "Protection.hpp"
#include "UtilsText.hpp"
#include "Math/Pressure.h"
#include "Math/Units.h"
#include "Device/Parser.h"
#include "Device/Port.h"
#include "McReady.h"
#include "SettingsComputer.hpp"
#include "NMEA/Info.h"

#include <tchar.h>

static bool
cLXWP0(struct DeviceDescriptor *d, const TCHAR *String, NMEA_INFO *GPS_INFO);

static bool
cLXWP1(struct DeviceDescriptor *d, const TCHAR *String, NMEA_INFO *GPS_INFO);

static bool
cLXWP2(struct DeviceDescriptor *d, const TCHAR *String, NMEA_INFO *GPS_INFO);

static bool
CondorParseNMEA(struct DeviceDescriptor *d, const TCHAR *String,
                NMEA_INFO *GPS_INFO)
{
  (void)d;

  if(_tcsncmp(_T("$LXWP0"), String, 6)==0)
    {
      return cLXWP0(d, &String[7], GPS_INFO);
    }
  if(_tcsncmp(_T("$LXWP1"), String, 6)==0)
    {
      return cLXWP1(d, &String[7], GPS_INFO);
    }
  if(_tcsncmp(_T("$LXWP2"), String, 6)==0)
    {
      return cLXWP2(d, &String[7], GPS_INFO);
    }

  return false;
}

const struct DeviceRegister condorDevice = {
  _T("Condor"),
  drfGPS | drfBaroAlt | drfSpeed | drfVario | drfCondor,
  CondorParseNMEA,		// ParseNMEA
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

// *****************************************************************************
// local stuff

static bool
cLXWP1(struct DeviceDescriptor *d, const TCHAR *String, NMEA_INFO *GPS_INFO)
{
  //  TCHAR ctemp[80];
  (void)GPS_INFO;
  // do nothing!
  return true;
}


static bool
cLXWP2(struct DeviceDescriptor *d, const TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];
  (void)GPS_INFO;

  NMEAParser::ExtractParameter(String,ctemp,0);
  GlidePolar::SetMacCready(StrToDouble(ctemp,NULL));
  return true;
}


static bool
cLXWP0(struct DeviceDescriptor *d, const TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];

  /*
  $LXWP0,Y,222.3,1665.5,1.71,,,,,,239,174,10.1

   0 loger_stored (Y/N)
   1 IAS (kph) ----> Condor uses TAS!
   2 baroaltitude (m)
   3 vario (m/s)
   4-8 unknown
   9 heading of plane
  10 windcourse (deg)
  11 windspeed (kph)

  */
  double alt, airspeed;

  NMEAParser::ExtractParameter(String,ctemp,1);
  airspeed = StrToDouble(ctemp,NULL)/TOKPH;

  NMEAParser::ExtractParameter(String,ctemp,2);
  alt = StrToDouble(ctemp,NULL);

  GPS_INFO->IndicatedAirspeed = airspeed/AirDensityRatio(alt);
  GPS_INFO->TrueAirspeed = airspeed;

  if (d == pDevPrimaryBaroSource){
    GPS_INFO->BaroAltitudeAvailable = true;
    GPS_INFO->BaroAltitude = alt;    // ToDo check if QNH correction is needed!
  }

  NMEAParser::ExtractParameter(String,ctemp,3);
  GPS_INFO->Vario = StrToDouble(ctemp,NULL);

  GPS_INFO->AirspeedAvailable = true;
  GPS_INFO->VarioAvailable = true;

  TriggerVarioUpdate();

  return true;
}
