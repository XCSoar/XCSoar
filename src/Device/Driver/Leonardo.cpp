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

#include "Device/Driver/Leonardo.hpp"
#include "Device/Parser.hpp"
#include "Device/Driver.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "Units/Units.hpp"

#include <stdlib.h>
#include <math.h>

class LeonardoDevice : public AbstractDevice {
private:
  Port *port;

public:
  LeonardoDevice(Port *_port):port(_port) {}

public:
  virtual bool ParseNMEA(const char *line, struct NMEA_INFO *info);
};

static bool
ReadSpeedVector(NMEAInputLine &line, SpeedVector &value_r)
{
  fixed norm, bearing;

  bool norm_valid = line.read_checked(norm);
  bool bearing_valid = line.read_checked(bearing);

  if (bearing_valid && norm_valid) {
    value_r.norm = Units::ToSysUnit(norm, unKiloMeterPerHour);
    value_r.bearing = Angle::degrees(bearing);
    return true;
  } else
    return false;
}

/**
 * Parse a "$C" sentence.
 *
 * Example: "$C,+2025,-7,+18,+25,+29,122,314,314,0,-356,+25,45,T*3D"
 */
static bool
LeonardoParseC(NMEAInputLine &line, NMEA_INFO &info)
{
  fixed value;

  // 0 = altitude [m]
  if (line.read_checked(value))
    info.ProvideBaroAltitudeTrue(value);

  // 1 = vario [dm/s]
  if (line.read_checked(value))
    info.ProvideTotalEnergyVario(value / 10);

  // 2 = airspeed [km/h]
  /* XXX is that TAS or IAS? */
  if (line.read_checked(value))
    info.ProvideTrueAirspeed(Units::ToSysUnit(value, unKiloMeterPerHour));

  // 3 = netto vario [dm/s]
  if (line.read_checked(value))
    info.ProvideNettoVario(value / 10);
  else
    /* short "$C" sentence ends after airspeed */
    return true;

  // 4 = temperature [deg C]
  fixed oat;
  info.TemperatureAvailable = line.read_checked(oat);
  if (info.TemperatureAvailable)
    info.OutsideAirTemperature = Units::ToSysUnit(oat, unGradCelcius);

  line.skip(5);

  // 10 = wind speed [km/h]
  // 11 = wind direction [degrees]
  SpeedVector wind;
  if (ReadSpeedVector(line, wind))
    info.ProvideExternalWind(wind);

  return true;
}

/**
 * Parse a "$D" sentence.
 *
 * Example: "$D,+0,100554,+25,18,+31,,0,-356,+25,+11,115,96*6A"
 */
static bool
LeonardoParseD(NMEAInputLine &line, NMEA_INFO &info)
{
  fixed value;

  // 0 = vario [dm/s]
  if (line.read_checked(value))
    info.ProvideTotalEnergyVario(value / 10);

  // 1 = air pressure [Pa]
  if (line.skip() == 0)
    /* short "$C" sentence ends after airspeed */
    return true;

  // 2 = netto vario [dm/s]
  if (line.read_checked(value))
    info.ProvideNettoVario(value / 10);

  // 3 = airspeed [km/h]
  /* XXX is that TAS or IAS? */
  if (line.read_checked(value))
    info.ProvideTrueAirspeed(Units::ToSysUnit(value, unKiloMeterPerHour));

  // 4 = temperature [deg C]
  fixed oat;
  info.TemperatureAvailable = line.read_checked(oat);
  if (info.TemperatureAvailable)
    info.OutsideAirTemperature = Units::ToSysUnit(oat, unGradCelcius);

  // 5 = compass [degrees]
  /* XXX unsupported by XCSoar */

  // 6 = optimal speed [km/h]
  /* XXX unsupported by XCSoar */

  // 7 = equivalent MacCready [cm/s]
  /* XXX unsupported by XCSoar */

  // 8 = wind speed [km/h]
  /* not used here, the "$C" record repeats it together with the
     direction */

  return true;
}

/**
 * Parse a "$PDGFTL1" sentence.
 *
 * Example: "$PDGFTL1,2025,2000,250,-14,45,134,28,65,382,153*3D"
 */
static bool
PDGFTL1(NMEAInputLine &line, NMEA_INFO &info)
{
  fixed value;

  //  Baro Altitude QNE(1013.25)     2025     meter        2025 mt
  if (line.read_checked(value))
    info.ProvidePressureAltitude(value);

  //  Baro Altitude QNH  2000     meter        2000 mt
  if (line.read_checked(value))
    info.ProvideBaroAltitudeTrue(value);

  //  Vario  250      cm/sec       +2,50 m/s
  if (line.read_checked(value))
    info.ProvideTotalEnergyVario(value / 100);

  //  Netto Vario  -14      dm/sec       -1,40 m/s
  if (line.read_checked(value))
    info.ProvideNettoVario(value / 10);

  //  Indicated Air Speed  45       km/h         45 km/h
  //  Ground Efficiency  134      ratio        13,4 : 1
  line.skip(2);

  //  Wind Speed  28       km/h         28 km/h
  //  Wind Direction  65       degree       65 degree
  SpeedVector wind;
  if (ReadSpeedVector(line, wind))
    info.ProvideExternalWind(wind);

  //  Main Lithium Battery Voltage   382      0.01 volts   3,82 volts
  //  Backup AA Battery Voltage      153      0.01 volts   1,53 volts

  return true;
}

bool
LeonardoDevice::ParseNMEA(const char *_line, NMEA_INFO *info)
{
  NMEAInputLine line(_line);
  char type[16];
  line.read(type, 16);

  if (strcmp(type, "$C") == 0 || strcmp(type, "$c") == 0)
    return LeonardoParseC(line, *info);
  else if (strcmp(type, "$D") == 0 || strcmp(type, "$d") == 0)
    return LeonardoParseD(line, *info);
  else if (strcmp(type, "$PDGFTL1") == 0)
    return PDGFTL1(line, *info);
  else
    return false;
}

static Device *
LeonardoCreateOnPort(Port *com_port)
{
  return new LeonardoDevice(com_port);
}

const struct DeviceRegister leonardo_device_driver = {
  _T("Leonardo"),
  0,
  LeonardoCreateOnPort,
};
