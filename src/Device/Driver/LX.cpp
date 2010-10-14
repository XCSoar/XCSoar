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

#include "Device/Driver/LX.hpp"
#include "Device/Driver.hpp"
#include "Device/Parser.hpp"
#include "Protection.hpp"
#include "Units.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"

#include <stdlib.h>

class LXDevice: public AbstractDevice
{
public:
  virtual bool ParseNMEA(const char *line, struct NMEA_INFO *info,
      bool enable_baro);
};

static bool
ReadSpeedVector(NMEAInputLine &line, SpeedVector &value_r)
{
  fixed bearing, norm;

  bool bearing_valid = line.read_checked(bearing);
  bool norm_valid = line.read_checked(norm);

  if (bearing_valid && norm_valid) {
    value_r.bearing = Angle::degrees(bearing);
    value_r.norm = Units::ToSysUnit(norm, unKiloMeterPerHour);
    return true;
  } else
    return false;
}

static bool
LXWP0(NMEAInputLine &line, NMEA_INFO *GPS_INFO, bool enable_baro)
{
  /*
  $LXWP0,Y,222.3,1665.5,1.71,,,,,,239,174,10.1

   0 loger_stored (Y/N)
   1 IAS (kph) ----> Condor uses TAS!
   2 baroaltitude (m)
   3-8 vario (m/s) (last 6 measurements in last second)
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

  line.skip(6);

  GPS_INFO->ExternalWindAvailable = ReadSpeedVector(line, GPS_INFO->wind);

  TriggerVarioUpdate();

  return true;
}

static bool
LXWP1(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  /*
   * $LXWP1,
   * serial number,
   * instrument ID,
   * software version,
   * hardware version,
   * license string
   */
  (void)GPS_INFO;
  return true;
}

static bool
LXWP2(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  /*
   * $LXWP2,
   * maccready value, (m/s)
   * ballast, (1.0 - 1.5)
   * bugs, (0 - 100%)
   * polar_a,
   * polar_b,
   * polar_c,
   * audio volume
   */

  fixed value;
  // MacCready value
  if (line.read_checked(value))
    GPS_INFO->MacCready = value;

  // Ballast
  line.skip();
  /*
  if (line.read_checked(value))
    GPS_INFO->Ballast = value;
  */

  // Bugs
  if (line.read_checked(value))
    GPS_INFO->Bugs = fixed(100) - value;

  return true;
}

bool
LXDevice::ParseNMEA(const char *String, NMEA_INFO *GPS_INFO, bool enable_baro)
{
  NMEAInputLine line(String);
  char type[16];
  line.read(type, 16);

  if (strcmp(type, "$LXWP0") == 0)
    return LXWP0(line, GPS_INFO, enable_baro);

  if (strcmp(type, "$LXWP1") == 0)
    return LXWP1(line, GPS_INFO);

  if (strcmp(type, "$LXWP2") == 0)
    return LXWP2(line, GPS_INFO);

  return false;
}

static Device *
LXCreateOnPort(Port *com_port)
{
  return new LXDevice();
}

const struct DeviceRegister lxDevice = {
  _T("LX"),
  drfGPS | drfBaroAlt | drfSpeed | drfVario,
  LXCreateOnPort,
};
