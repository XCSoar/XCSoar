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
#include "Device/Parser.hpp"
#include "Device/Driver.hpp"
#include "Math/FastMath.h"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"

#include <string.h>

class PGDevice : public AbstractDevice {
public:
  virtual bool ParseNMEA(const char *line, struct NMEA_INFO *info,
                         bool enable_baro);
  virtual bool Declare(const struct Declaration *declaration);
};

static bool
GPWIN(NMEAInputLine &line, NMEA_INFO *GPS_INFO, bool enable_baro);

bool
PGDevice::ParseNMEA(const char *String, NMEA_INFO *GPS_INFO, bool enable_baro)
{
  NMEAInputLine line(String);
  char type[16];
  line.read(type, 16);

  // $GPWIN ... Winpilot proprietary sentance includinh baro altitude
  // $GPWIN ,01900 , 0 , 5159 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 * 6 B , 0 7 * 6 0 E
  if (strcmp(type, "$GPWIN") == 0)
    return GPWIN(line, GPS_INFO, enable_baro);
  else
    return false;

}

bool
PGDevice::Declare(const struct Declaration *decl)
{
  (void)decl;

  return true;
}

static Device *
PGCreateOnPort(Port *com_port)
{
  return new PGDevice();
}

const struct DeviceRegister pgDevice = {
  _T("PosiGraph Logger"),
  drfGPS | drfBaroAlt | drfLogger,
  PGCreateOnPort,
};

// *****************************************************************************
// local stuff

static bool
GPWIN(NMEAInputLine &line, NMEA_INFO *GPS_INFO, bool enable_baro)
{
  line.skip(2);

  fixed value;
  if (enable_baro && line.read_checked(value)) {
    GPS_INFO->BaroAltitude = GPS_INFO->pressure.AltitudeToQNHAltitude(value / 10);
    GPS_INFO->BaroAltitudeAvailable = true;
  }

  return false;
}
