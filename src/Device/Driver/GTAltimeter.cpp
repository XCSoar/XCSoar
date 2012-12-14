/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "GTAltimeter.hpp"
#include "Device/Driver.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Checksum.hpp"

class GTAltimeterDevice : public AbstractDevice {
public:
  virtual bool ParseNMEA(const char *line, struct NMEAInfo &info) gcc_override;
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
    info.ProvideNettoVario(fixed(vario) / 100);

  int temperature;
  if (line.ReadChecked(temperature) && temperature != 99) {
    info.temperature = fixed(temperature);
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

bool
GTAltimeterDevice::ParseNMEA(const char *_line, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(_line))
    return false;

  NMEAInputLine line(_line);
  char type[16];
  line.Read(type, 16);

  if (StringIsEqual(type, "$LK8EX1"))
    return LK8EX1(line, info);

  return false;
}

static Device *
GTAltimeterDeviceCreateOnPort(gcc_unused const DeviceConfig &config,
                              gcc_unused Port &port)
{
  return new GTAltimeterDevice();
}

const struct DeviceRegister gt_altimeter_device_driver = {
  _T("GTAltimeter"),
  _T("GT Altimeter (GliderTools)"),
  0,
  GTAltimeterDeviceCreateOnPort,
};
