/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Math/Util.hpp"

#include <stdint.h>

class CProbeDevice : public AbstractDevice {
public:
  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
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
    double q[4];
    for (unsigned i = 0; i < 4; ++i)
      // Cast to int16_t to interpret the 16th bit as the sign bit
      q[i] = int16_t(_q[i]) / 1000.;

    double sin_pitch = -2 * (q[0] * q[2] - q[3] * q[1]);
    if (sin_pitch <= 1 && sin_pitch >= -1) {
      info.attitude.pitch_angle_available.Update(info.clock);
      info.attitude.pitch_angle = Angle::asin(sin_pitch);

      Angle heading = Angle::HalfCircle() +
        Angle::FromXY(Square(q[3]) - Square(q[0]) - Square(q[1]) + Square(q[2]),
                      2 * (q[1] * q[2] + q[3] * q[0]));

      info.attitude.heading_available.Update(info.clock);
      info.attitude.heading = heading;

      Angle roll = Angle::FromXY(Square(q[3]) + Square(q[0]) - Square(q[1]) - Square(q[2]),
                                 2 * (q[0] * q[1] + q[3] * q[2]));

      info.attitude.bank_angle_available.Update(info.clock);
      info.attitude.bank_angle = roll;
    }
  }

  unsigned _a[3];
  bool a_available = line.ReadHexChecked(_a[0]);
  if (!line.ReadHexChecked(_a[1]))
    a_available = false;
  if (!line.ReadHexChecked(_a[2]))
    a_available = false;

  if (a_available) {
    double a[3];
    for (unsigned i = 0; i < 3; ++i)
      // Cast to int16_t to interpret the 16th bit as the sign bit
      a[i] = int16_t(_a[i]) / 1000.;

    info.acceleration.ProvideGLoad(SpaceDiagonal(a[0], a[1], a[2]),
                                   true);
  }

  unsigned temperature;
  if (line.ReadHexChecked(temperature)) {
    info.temperature_available = true;
    info.temperature = Temperature::FromCelsius(int16_t(temperature) / 10.);
  }

  unsigned humidity;
  if (line.ReadHexChecked(humidity)) {
    info.humidity_available = true;
    info.humidity = int16_t(humidity) / 10.;
  }

  unsigned battery_level;
  if (line.ReadHexChecked(battery_level)) {
    info.battery_level_available.Update(info.clock);
    info.battery_level = int16_t(battery_level);
  }

  unsigned _delta_pressure;
  if (line.ReadHexChecked(_delta_pressure)) {
    auto delta_pressure = int16_t(_delta_pressure) / 10.;
    info.ProvideDynamicPressure(AtmosphericPressure::Pascal(delta_pressure));
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
