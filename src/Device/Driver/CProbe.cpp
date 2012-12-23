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

#include "Device/Driver/CProbe.hpp"
#include "Device/Driver.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "Atmosphere/Temperature.hpp"

#include <stdint.h>

class CProbeDevice : public AbstractDevice {
public:
  virtual bool ParseNMEA(const char *line, struct NMEAInfo &info) gcc_override;
};

static bool
ParseData(NMEAInputLine &line, NMEAInfo &info)
{
  // $PCPROBE,T,Q0,Q1,Q2,Q3,ax,ay,az,temp,rh,batt,delta_press,abs_press,C,
  // see http://www.compassitaly.com/CMS/index.php/en/download/c-probe/231-c-probeusermanual/download

  unsigned _q[4];
  bool q_available = line.ReadHexChecked(_q[0]);
  if (!line.ReadHexChecked(_q[1]))
    q_available = false;
  if (!line.ReadHexChecked(_q[2]))
    q_available = false;
  if (!line.ReadHexChecked(_q[3]))
    q_available = false;

  if (q_available) {
    fixed q[4];
    for (unsigned i = 0; i < 4; ++i)
      // Cast to int16_t to interpret the 16th bit as the sign bit
      q[i] = fixed((int16_t)_q[i]) / 1000;

    fixed sin_pitch = -2 * (q[0] * q[2] - q[3] * q[1]);
    if (sin_pitch <= fixed(1) && sin_pitch >= fixed(-1)) {
      fixed pitch = asin(sin_pitch);

      info.attitude.pitch_angle_available = true;
      info.attitude.pitch_angle = Angle::Radians(pitch);

      fixed heading = fixed_pi + atan2(Double(q[1] * q[2] + q[3] * q[0]),
                                       sqr(q[3]) - sqr(q[0]) - sqr(q[1]) + sqr(q[2]));

      info.attitude.heading_available.Update(info.clock);
      info.attitude.heading = Angle::Radians(heading);

      fixed roll = atan2(Double(q[0] * q[1] + q[3] * q[2]),
                         sqr(q[3]) + sqr(q[0]) - sqr(q[1]) - sqr(q[2]));

      info.attitude.bank_angle_available = true;
      info.attitude.bank_angle = Angle::Radians(roll);
    }
  }

  unsigned _a[3];
  bool a_available = line.ReadHexChecked(_a[0]);
  if (!line.ReadHexChecked(_a[1]))
    a_available = false;
  if (!line.ReadHexChecked(_a[2]))
    a_available = false;

  if (a_available) {
    fixed a[3];
    for (unsigned i = 0; i < 3; ++i)
      // Cast to int16_t to interpret the 16th bit as the sign bit
      a[i] = fixed((int16_t)_a[i]) / 1000;

    info.acceleration.ProvideGLoad(sqrt(sqr(a[0]) + sqr(a[1]) + sqr(a[2])), true);
  }

  unsigned temperature;
  if (line.ReadHexChecked(temperature)) {
    info.temperature_available = true;
    info.temperature = CelsiusToKelvin(fixed((int16_t)temperature) / 10);
  }

  unsigned humidity;
  if (line.ReadHexChecked(humidity)) {
    info.humidity_available = true;
    info.humidity = fixed((int16_t)humidity) / 10;
  }

  unsigned battery_level;
  if (line.ReadHexChecked(battery_level)) {
    info.battery_level_available.Update(info.clock);
    info.battery_level = fixed((int16_t)battery_level);
  }

  return true;
}

bool
CProbeDevice::ParseNMEA(const char *_line, NMEAInfo &info)
{
  NMEAInputLine line(_line);
  char type[16];
  line.Read(type, 16);

  if (!StringIsEqual(type, "$PCPROBE"))
    return false;

  line.Read(type, 16);
  if (StringIsEqual(type, "T"))
    return ParseData(line, info);
  else
    return false;
}

static Device *
CProbeCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new CProbeDevice();
}

const struct DeviceRegister c_probe_driver = {
  _T("CProbe"),
  _T("Compass C-Probe"),
  0,
  CProbeCreateOnPort,
};
