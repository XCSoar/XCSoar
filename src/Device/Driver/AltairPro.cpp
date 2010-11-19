/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Device/Driver/AltairPro.hpp"
#include "Device/Driver.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "Units.hpp"

#include <string.h>

class AltairProDevice : public AbstractDevice {
private:
  fixed lastAlt;
  bool last_enable_baro;

public:
  AltairProDevice():lastAlt(fixed_zero), last_enable_baro(false) {}

public:
  virtual bool ParseNMEA(const char *line, struct NMEA_INFO *info,
                         bool enable_baro);
  virtual bool PutQNH(const AtmosphericPressure& pres);
  virtual bool Declare(const struct Declaration *declaration);
  virtual void OnSysTicker();
};

static bool
ReadAltitude(NMEAInputLine &line, fixed &value_r)
{
  fixed value;
  bool available = line.read_checked(value);
  char unit = line.read_first_char();
  if (!available)
    return false;

  if (unit == _T('f') || unit == _T('F'))
    value = Units::ToSysUnit(value, unFeet);

  value_r = value;
  return true;
}

bool
AltairProDevice::ParseNMEA(const char *String, NMEA_INFO *GPS_INFO,
                           bool enable_baro)
{
  NMEAInputLine line(String);
  char type[16];
  line.read(type, 16);

  // no propriatary sentence

  if (strcmp(type, "$PGRMZ") == 0) {
    bool available = ReadAltitude(line, lastAlt);
    if (enable_baro && available) {
      GPS_INFO->BaroAltitudeAvailable = true;
      GPS_INFO->BaroAltitude = GPS_INFO->pressure.AltitudeToQNHAltitude(lastAlt);
    }

    last_enable_baro = enable_baro;

    return true;
  }

  return false;

}

bool
AltairProDevice::Declare(const struct Declaration *decl)
{
  (void) decl;

  // TODO feature: Altair declaration

  return true;
}

#include "DeviceBlackboard.hpp"

bool
AltairProDevice::PutQNH(const AtmosphericPressure &pres)
{
  // TODO code: JMW check sending QNH to Altair
  if (last_enable_baro)
    device_blackboard.SetBaroAlt(pres.AltitudeToQNHAltitude(fixed(lastAlt)));

  return true;
}

void
AltairProDevice::OnSysTicker()
{
  // Do To get IO data like temp, humid, etc
}

static Device *
AltairProCreateOnPort(Port *com_port)
{
  return new AltairProDevice();
}

const struct DeviceRegister atrDevice = {
  _T("Altair RU"),
  drfGPS | drfBaroAlt, // drfLogger - ToDo
  AltairProCreateOnPort,
};
