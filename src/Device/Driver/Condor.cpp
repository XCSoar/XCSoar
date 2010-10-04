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

#include "Device/Driver/Condor.hpp"
#include "Device/Driver.hpp"
#include "Protection.hpp"
#include "Units.hpp"
#include "Device/Parser.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"

#include <stdlib.h>

class CondorDevice : public AbstractDevice {
public:
  virtual bool ParseNMEA(const char *line, struct NMEA_INFO *info,
                         bool enable_baro);
};

static bool
cLXWP0(NMEAInputLine &line, NMEA_INFO *GPS_INFO, bool enable_baro)
{
  /*
  $LXWP0,Y,222.3,1665.5,1.71,,,,,,239,174,10.1

   0 logger_stored (Y/N)
   1 IAS (kph) ----> Condor uses TAS!
   2 baroaltitude (m)
   3 vario (m/s)
   4-8 unknown
   9 heading of plane
  10 windcourse (deg)
  11 windspeed (kph)
  */

  line.skip();

  fixed airspeed;
  GPS_INFO->AirspeedAvailable = line.read_checked(airspeed);

  fixed alt = line.read(fixed_zero);

  if (GPS_INFO->AirspeedAvailable) {
    GPS_INFO->TrueAirspeed = Units::ToSysUnit(airspeed, unKiloMeterPerHour);
    GPS_INFO->IndicatedAirspeed =
      GPS_INFO->TrueAirspeed / AtmosphericPressure::AirDensityRatio(alt);
  }

  if (enable_baro) {
    GPS_INFO->BaroAltitudeAvailable = true;
    GPS_INFO->BaroAltitude = alt; // ToDo check if QNH correction is needed!
  }

  GPS_INFO->TotalEnergyVarioAvailable =
    line.read_checked(GPS_INFO->TotalEnergyVario);

  TriggerVarioUpdate();

  return true;
}

static bool
cLXWP1(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  (void)GPS_INFO;
  return true;
}

static bool
cLXWP2(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  (void)GPS_INFO;
  return true;
}

bool
CondorDevice::ParseNMEA(const char *String, NMEA_INFO *GPS_INFO,
                        bool enable_baro)
{
  GPS_INFO->gps.Simulator = true;

  NMEAInputLine line(String);
  char type[16];
  line.read(type, 16);

  if (strcmp(type, "$LXWP0") == 0)
    return cLXWP0(line, GPS_INFO, enable_baro);

  if (strcmp(type, "$LXWP1") == 0)
    return cLXWP1(line, GPS_INFO);

  if (strcmp(type, "$LXWP2") == 0)
    return cLXWP2(line, GPS_INFO);

  return false;
}

static Device *
CondorCreateOnPort(Port *com_port)
{
  return new CondorDevice();
}

const struct DeviceRegister condorDevice = {
  _T("Condor"),
  drfGPS | drfBaroAlt | drfSpeed | drfVario,
  CondorCreateOnPort,
};
