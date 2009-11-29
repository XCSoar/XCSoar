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

// created 20070428 sgi, basic NMEA sentance supported
// WARNING PosiGraph dont send any GGA sentance
// GGA sentance trigger nav process in XCSoar
// Posigraph driver trigger Nav process with RMC sentance (if driver is devA())

// ToDo

// adding baro alt sentance paser to support baro source priority  if (d == pDevPrimaryBaroSource){...}

#include "Device/Driver/PosiGraph.hpp"
#include "Device/Internal.hpp"
#include "Math/FastMath.h"
#include "Math/Pressure.h"
#include "Device/Parser.h"
#include "NMEA/Info.h"

#include <tchar.h>
#include <stdlib.h>

class PGDevice : public AbstractDevice {
public:
  virtual bool ParseNMEA(const TCHAR *line, struct NMEA_INFO *info,
                         bool enable_baro);
  virtual bool Declare(const struct Declaration *declaration);
};

static bool
GPWIN(const TCHAR *String, NMEA_INFO *GPS_INFO, bool enable_baro);

bool
PGDevice::ParseNMEA(const TCHAR *String, NMEA_INFO *GPS_INFO, bool enable_baro)
{
  (void)String;
  (void)GPS_INFO;

  // $GPWIN ... Winpilot proprietary sentance includinh baro altitude
  // $GPWIN ,01900 , 0 , 5159 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 * 6 B , 0 7 * 6 0 E
  if(_tcsncmp(_T("$GPWIN"), String, 6)==0)
    {
      return GPWIN(&String[7], GPS_INFO, enable_baro);
    }

  return false;

}

bool
PGDevice::Declare(const struct Declaration *decl)
{
  (void)decl;

  return true;
}

static Device *
PGCreateOnComPort(ComPort *com_port)
{
  return new PGDevice();
}

const struct DeviceRegister pgDevice = {
  _T("PosiGraph Logger"),
  drfGPS | drfBaroAlt | drfLogger,
  PGCreateOnComPort,
};

// *****************************************************************************
// local stuff

static bool
GPWIN(const TCHAR *String, NMEA_INFO *GPS_INFO, bool enable_baro)
{
  TCHAR ctemp[80];

  NMEAParser::ExtractParameter(String, ctemp, 2);

  if (enable_baro) {
    GPS_INFO->BaroAltitudeAvailable = true;
    GPS_INFO->BaroAltitude = AltitudeToQNHAltitude(iround(_tcstod(ctemp, NULL) / 10));
  }

  return false;
}
