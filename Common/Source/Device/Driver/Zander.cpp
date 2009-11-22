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

#include "Device/Driver/Zander.hpp"
#include "Device/Internal.hpp"
#include "Protection.hpp"
#include "Math/Pressure.h"
#include "Device/Parser.h"
#include "NMEA/Info.h"

#include <stdlib.h>

static bool
PZAN1(const TCHAR *String, NMEA_INFO *aGPS_INFO);

static bool
PZAN2(const TCHAR *String, NMEA_INFO *aGPS_INFO);

class ZanderDevice : public AbstractDevice {
  virtual bool ParseNMEA(const TCHAR *line, struct NMEA_INFO *info,
                         bool enable_baro);
};

bool
ZanderDevice::ParseNMEA(const TCHAR *String, NMEA_INFO *aGPS_INFO,
                        bool enable_baro)
{
  if(_tcsncmp(_T("$PZAN1"), String, 6)==0)
    {
      return PZAN1(&String[7], aGPS_INFO);
    }
  if(_tcsncmp(_T("$PZAN2"), String, 6)==0)
    {
      return PZAN2(&String[7], aGPS_INFO);
    }

  return false;

}

static Device *
ZanderCreateOnComPort(ComPort *com_port)
{
  return new ZanderDevice();
}

const struct DeviceRegister zanderDevice = {
  _T("Zander"),
  drfGPS | drfBaroAlt | drfSpeed | drfVario,
  ZanderCreateOnComPort,
};

// *****************************************************************************
// local stuff

static bool
PZAN1(const TCHAR *String, NMEA_INFO *aGPS_INFO)
{
  TCHAR ctemp[80];
  aGPS_INFO->BaroAltitudeAvailable = true;
  NMEAParser::ExtractParameter(String,ctemp,0);
  aGPS_INFO->BaroAltitude = AltitudeToQNHAltitude(_tcstod(ctemp, NULL));
  return true;
}


static bool
PZAN2(const TCHAR *String, NMEA_INFO *aGPS_INFO)
{
  TCHAR ctemp[80];
  double vtas, wnet, vias;

  NMEAParser::ExtractParameter(String,ctemp,0);
  vtas = _tcstod(ctemp, NULL) / 3.6;
  // JMW 20080721 fixed km/h->m/s conversion

  NMEAParser::ExtractParameter(String,ctemp,1);
  wnet = (_tcstod(ctemp, NULL) - 10000) / 100; // cm/s
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
