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

#include "Device/Driver/ILEC.hpp"
#include "Device/Driver.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "Units/System.hpp"

class ILECDevice : public AbstractDevice {
public:
  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
};

static bool
ReadSpeedVector(NMEAInputLine &line, SpeedVector &value_r)
{
  double norm, bearing;

  bool bearing_valid = line.ReadChecked(bearing);
  bool norm_valid = line.ReadChecked(norm);

  if (bearing_valid && norm_valid) {
    value_r.norm = Units::ToSysUnit(norm, Unit::KILOMETER_PER_HOUR);
    value_r.bearing = Angle::Degrees(bearing);
    return true;
  } else
    return false;
}

/**
 * Parse a "$PILC,PDA1" sentence.
 *
 * Example: "$PILC,PDA1,1489,-3.21,274,15,58*7D"
 */
static bool
ParsePDA1(NMEAInputLine &line, NMEAInfo &info)
{
  double value;

  // altitude [m]
  int altitude;
  if (line.ReadChecked(altitude))
    info.ProvideBaroAltitudeTrue(altitude);

  // total energy vario [m/s]
  if (line.ReadChecked(value))
    info.ProvideTotalEnergyVario(value);

  // wind direction [degrees, kph]
  SpeedVector wind;
  if (ReadSpeedVector(line, wind))
    info.ProvideExternalWind(wind);

  // confidence [0..100]
  // not used

  return true;
}

bool
ILECDevice::ParseNMEA(const char *_line, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(_line))
    return false;

  NMEAInputLine line(_line);
  char type[16];
  line.Read(type, sizeof(type));

  if (StringIsEqual(type, "$PILC")) {
    line.Read(type, sizeof(type));
    if (StringIsEqual(type, "PDA1"))
      return ParsePDA1(line, info);
    else
      return false;
  } else
    return false;
}

static Device *
ILECCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new ILECDevice();
}

const struct DeviceRegister ilec_driver = {
  _T("ILEC SN10"),
  _T("ILEC SN10"),
  0,
  ILECCreateOnPort,
};
