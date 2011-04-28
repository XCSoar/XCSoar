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

#include "Device/Driver/Zander.hpp"
#include "Device/Driver.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"
#include "Units/Units.hpp"

#include <stdlib.h>

class ZanderDevice : public AbstractDevice {
public:
  virtual bool ParseNMEA(const char *line, struct NMEA_INFO *info);
};

static bool
PZAN1(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  fixed baro_altitude;
  if (line.read_checked(baro_altitude))
    /* the ZS1 documentation does not specify wheter the altitude is
       STD or QNH, but Franz Poeschl confirmed via email that it is
       the QNH altitude */
    GPS_INFO->ProvideBaroAltitudeTrue(baro_altitude);

  return true;
}

static bool
PZAN2(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  fixed vtas, wnet;

  if (line.read_checked(vtas))
    GPS_INFO->ProvideTrueAirspeed(Units::ToSysUnit(vtas,
                                                   unKiloMeterPerHour));

  if (line.read_checked(wnet))
    GPS_INFO->ProvideTotalEnergyVario((wnet - fixed(10000)) / 100);

  return true;
}

static bool
PZAN3(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  // old: $PZAN3,+,026,V,321,035,A,321,035,V*cc
  // new: $PZAN3,+,026,A,321,035,V[,A]*cc

  line.skip(3);

  int direction, speed;
  if (!line.read_checked(direction) || !line.read_checked(speed))
    return false;

  char okay = line.read_first_char();
  if (okay == 'V') {
    okay = line.read_first_char();
    if (okay == 'V')
      return true;

    if (okay != 'A') {
      line.skip();
      okay = line.read_first_char();
    }
  }

  if (okay == 'A') {
    SpeedVector wind(Angle::degrees(fixed(direction)),
                     Units::ToSysUnit(fixed(speed), unKiloMeterPerHour));
    GPS_INFO->ProvideExternalWind(wind);
  }

  return true;
}

static bool
PZAN4(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  // $PZAN4,1.5,+,20,39,45*cc

  fixed mc;
  if (line.read_checked(mc))
    GPS_INFO->settings.ProvideMacCready(mc, GPS_INFO->Time);

  return true;
}

bool
ZanderDevice::ParseNMEA(const char *String, NMEA_INFO *GPS_INFO)
{
  if (!VerifyNMEAChecksum(String))
    return false;

  NMEAInputLine line(String);
  char type[16];
  line.read(type, 16);

  if (strcmp(type, "$PZAN1") == 0)
    return PZAN1(line, GPS_INFO);

  if (strcmp(type, "$PZAN2") == 0)
    return PZAN2(line, GPS_INFO);

  if (strcmp(type, "$PZAN3") == 0)
    return PZAN3(line, GPS_INFO);

  if (strcmp(type, "$PZAN4") == 0)
    return PZAN4(line, GPS_INFO);

  return false;
}

static Device *
ZanderCreateOnPort(Port *com_port)
{
  return new ZanderDevice();
}

const struct DeviceRegister zanderDevice = {
  _T("Zander"),
  drfBaroAlt,
  ZanderCreateOnPort,
};
