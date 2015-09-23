/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "Device/Driver/Ridimuim.hpp"
#include "Device/Driver.hpp"
#include "Device/Parser.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "Units/System.hpp"
#include "Atmosphere/Temperature.hpp"

#include <stdlib.h>
#include <math.h>

class RidimuimDevice : public AbstractDevice {
public:
  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
};

static bool
LK8EX1(NMEAInputLine &line, NMEAInfo &info)
{
  unsigned pressure;
  bool pressure_available = (line.ReadChecked(pressure) && pressure != 999999);

  if (pressure_available)
    info.ProvideStaticPressure(AtmosphericPressure::Pascal(fixed(pressure)));

  unsigned altitude;
  bool altitude_available = (line.ReadChecked(altitude) && altitude != 99999);
   
  if (altitude_available && !pressure_available)
    info.ProvidePressureAltitude(fixed(altitude));

  int vario;
  if (line.ReadChecked(vario) && vario != 9999)
    info.ProvideNoncompVario(fixed(vario) / 100);

  int temperature;
  if (line.ReadChecked(temperature) && temperature != 99) {
    info.temperature = CelsiusToKelvin(fixed(temperature));
    info.temperature_available = true;
  }

  fixed battery_value;
  if (line.ReadChecked(battery_value) &&
      (unsigned)(battery_value + fixed(0.5)) != 999) {
    if (battery_value > fixed(1000)) {
      info.battery_level = battery_value - fixed(1000);
      info.battery_level_available.Update(info.clock);
    } else {
      info.voltage = battery_value;
      info.voltage_available.Update(info.clock);
    }
  }

  return true;
}

static bool
PLKAS(NMEAInputLine &line, NMEAInfo &info)
{
/*
New PLKAS   NMEA sentence.
    The syntax is:
    $PLKAS,nnn,*checksum
    where nnn is the Indicated Air Speed in m/s *10
    and checksum is the NMEA checksum
    Example for nnn  346 = 34.6 m/s  which is = 124.56 km/h
    This sentence can be sent anytime, with any device connected.
    It is normally a sub sentence of the LK8EX1 device.
*/
  int air_speed;
  if (line.ReadChecked(air_speed) && air_speed != 999)
      info.ProvideTrueAirspeed(Units::ToSysUnit(fixed(air_speed)*0.36, Unit::KILOMETER_PER_HOUR));
  
  return true;
}
bool
RidimuimDevice::ParseNMEA(const char *String, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(String))
    return false;

  NMEAInputLine line(String);
  char type[16];
  line.Read(type, 16);

  if (StringIsEqual(type, "$LK8EX1"))
    return LK8EX1(line, info);
  else if (StringIsEqual(type, "PLKAS"))
    return PLKAS(line, info);          
  
  return false;
}

static Device *
RidimuimCreateOnPort(gcc_unused const DeviceConfig &config, gcc_unused Port &port)
{
  return new RidimuimDevice();
}

const struct DeviceRegister ridimuim_driver = {
  _T("Ridimuim"),
  _T("Ridimuim"),
  0,
  RidimuimCreateOnPort,
};
