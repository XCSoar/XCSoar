/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

#include "Internal.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"

// RMN: Volkslogger
// Source data:
// $PGCS,1,0EC0,FFF9,0C6E,02*61
// $PGCS,1,0EC0,FFFA,0C6E,03*18
static bool
vl_PGCS1(NMEAInputLine &line, NMEAInfo &info)
{
  GPSState &gps = info.gps;

  if (line.read(1) != 1)
    return false;

  /* pressure sensor */
  line.skip();

  // four characers, hex, barometric altitude
  long altitude = line.read_hex(0L);

  if (altitude > 60000)
    /* Assuming that altitude has wrapped around.  60 000 m occurs at
       QNH ~2000 hPa */
    altitude -= 65535;

  info.ProvidePressureAltitude(fixed(altitude));

  // ExtractParameter(String,ctemp,3);
  // four characters, hex, constant.  Value 1371 (dec)

  // nSatellites = (int)(min(12,HexStrToDouble(ctemp, NULL)));

  if (gps.satellites_used <= 0) {
    gps.satellites_used = 4;
    // just to make XCSoar quit complaining.  VL doesn't tell how many
    // satellites it uses.  Without this XCSoar won't do wind
    // measurements.
  }

  return false;
}

bool
VolksloggerDevice::ParseNMEA(const char *String, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(String))
    return false;

  NMEAInputLine line(String);
  char type[16];
  line.read(type, 16);

  if (strcmp(type, "$PGCS") == 0)
    return vl_PGCS1(line, info);
  else
    return false;
}
