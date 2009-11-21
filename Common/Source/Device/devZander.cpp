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

#include "Device/devZander.h"
#include "Device/device.h"
#include "XCSoar.h"
#include "Protection.hpp"
#include "Math/Pressure.h"
#include "Device/Parser.h"
#include "Device/Port.h"
#include "UtilsText.hpp"
#include "NMEA/Info.h"

static bool
PZAN1(struct DeviceDescriptor *d, const TCHAR *String, NMEA_INFO *aGPS_INFO);

static bool
PZAN2(struct DeviceDescriptor *d, const TCHAR *String, NMEA_INFO *aGPS_INFO);


static bool
ZanderParseNMEA(struct DeviceDescriptor *d, const TCHAR *String,
                NMEA_INFO *aGPS_INFO)
{
  (void)d;

  if(_tcsncmp(TEXT("$PZAN1"), String, 6)==0)
    {
      return PZAN1(d, &String[7], aGPS_INFO);
    }
  if(_tcsncmp(TEXT("$PZAN2"), String, 6)==0)
    {
      return PZAN2(d, &String[7], aGPS_INFO);
    }

  return false;

}

const struct DeviceRegister zanderDevice = {
  TEXT("Zander"),
  drfGPS | drfBaroAlt | drfSpeed | drfVario,
  ZanderParseNMEA,		// ParseNMEA
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
PZAN1(struct DeviceDescriptor *d, const TCHAR *String, NMEA_INFO *aGPS_INFO)
{
  TCHAR ctemp[80];
  aGPS_INFO->BaroAltitudeAvailable = true;
  NMEAParser::ExtractParameter(String,ctemp,0);
  aGPS_INFO->BaroAltitude = AltitudeToQNHAltitude(StrToDouble(ctemp,NULL));
  return true;
}


static bool
PZAN2(struct DeviceDescriptor *d, const TCHAR *String, NMEA_INFO *aGPS_INFO)
{
  TCHAR ctemp[80];
  double vtas, wnet, vias;

  NMEAParser::ExtractParameter(String,ctemp,0);
  vtas = StrToDouble(ctemp,NULL)/3.6;
  // JMW 20080721 fixed km/h->m/s conversion

  NMEAParser::ExtractParameter(String,ctemp,1);
  wnet = (StrToDouble(ctemp,NULL)-10000)/100; // cm/s
  aGPS_INFO->Vario = wnet;

  if (aGPS_INFO->BaroAltitudeAvailable) {
    vias = vtas/AirDensityRatio(aGPS_INFO->BaroAltitude);
  } else {
    vias = 0.0;
  }

  aGPS_INFO->AirspeedAvailable = true;
  aGPS_INFO->TrueAirspeed = vtas;
  aGPS_INFO->IndicatedAirspeed = vias;
  aGPS_INFO->VarioAvailable = true;

  TriggerVarioUpdate();

  return true;
}
