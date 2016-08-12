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

#include "Device/Driver/Leonardo.hpp"
#include "Device/Driver.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "Units/System.hpp"

class LeonardoDevice : public AbstractDevice {
public:
  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
};

static bool
ReadSpeedVector(NMEAInputLine &line, SpeedVector &value_r)
{
  double norm, bearing;

  bool norm_valid = line.ReadChecked(norm);
  bool bearing_valid = line.ReadChecked(bearing);

  if (bearing_valid && norm_valid) {
    value_r.norm = Units::ToSysUnit(norm, Unit::KILOMETER_PER_HOUR);
    value_r.bearing = Angle::Degrees(bearing);
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
LeonardoParseC(NMEAInputLine &line, NMEAInfo &info)
{
  double value;

  // 0 = altitude [m]
  if (line.ReadChecked(value))
    info.ProvideBaroAltitudeTrue(value);

  // 1 = vario [cm/s]
  if (line.ReadChecked(value))
    info.ProvideTotalEnergyVario(value / 100);

  // 2 = airspeed [km/h]
  /* XXX is that TAS or IAS? */
  if (line.ReadChecked(value))
    info.ProvideTrueAirspeed(Units::ToSysUnit(value, Unit::KILOMETER_PER_HOUR));

  if (line.Rest().empty())
    /* short "$C" sentence ends after airspeed */
    return true;

  // 3 = netto vario [dm/s]
  if (line.ReadChecked(value))
    info.ProvideNettoVario(value / 10);

  // 4 = temperature [deg C]
  double oat;
  info.temperature_available = line.ReadChecked(oat);
  if (info.temperature_available)
    info.temperature = Temperature::FromCelsius(oat);

  line.Skip(5);

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
LeonardoParseD(NMEAInputLine &line, NMEAInfo &info)
{
  double value;

  // 0 = vario [dm/s]
  if (line.ReadChecked(value))
    info.ProvideTotalEnergyVario(value / 10);

  if (line.Rest().empty())
    /* short "$D" sentence ends after vario */
    return true;

  // 1 = air pressure [Pa]
  if (line.ReadChecked(value))
    info.ProvideStaticPressure(AtmosphericPressure::Pascal(value));

  // 2 = netto vario [dm/s]
  if (line.ReadChecked(value))
    info.ProvideNettoVario(value / 10);

  // 3 = airspeed [km/h]
  /* XXX is that TAS or IAS? */
  if (line.ReadChecked(value))
    info.ProvideTrueAirspeed(Units::ToSysUnit(value, Unit::KILOMETER_PER_HOUR));

  // 4 = temperature [deg C]
  double oat;
  info.temperature_available = line.ReadChecked(oat);
  if (info.temperature_available)
    info.temperature = Temperature::FromCelsius(oat);

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
PDGFTL1(NMEAInputLine &line, NMEAInfo &info)
{
  double value;

  //  Baro Altitude QNE(1013.25)     2025     meter        2025 mt
  if (line.ReadChecked(value))
    info.ProvidePressureAltitude(value);

  //  Baro Altitude QNH  2000     meter        2000 mt
  if (line.ReadChecked(value))
    info.ProvideBaroAltitudeTrue(value);

  //  Vario  250      cm/sec       +2,50 m/s
  if (line.ReadChecked(value))
    info.ProvideTotalEnergyVario(value / 100);

  //  Netto Vario  -14      dm/sec       -1,40 m/s
  if (line.ReadChecked(value))
    info.ProvideNettoVario(value / 10);

  //  Indicated Air Speed  45       km/h         45 km/h
  if (line.ReadChecked(value))
    info.ProvideIndicatedAirspeed(Units::ToSysUnit(value, Unit::KILOMETER_PER_HOUR));

  //  Ground Efficiency  134      ratio        13,4 : 1
  line.Skip();

  //  Wind Speed  28       km/h         28 km/h
  //  Wind Direction  65       degree       65 degree
  SpeedVector wind;
  if (ReadSpeedVector(line, wind))
    info.ProvideExternalWind(wind);

  //  Main Lithium Battery Voltage   382      0.01 volts   3,82 volts
  if (line.ReadChecked(value)) {
    info.voltage = value / 100;
    info.voltage_available.Update(info.clock);
  }

  //  Backup AA Battery Voltage      153      0.01 volts   1,53 volts

  return true;
}

bool
LeonardoDevice::ParseNMEA(const char *_line, NMEAInfo &info)
{
  NMEAInputLine line(_line);
  char type[16];
  line.Read(type, 16);

  if (StringIsEqual(type, "$C") ||
      StringIsEqual(type, "$c"))
    return LeonardoParseC(line, info);

  else if (StringIsEqual(type, "$D") ||
           StringIsEqual(type, "$d"))
    return LeonardoParseD(line, info);

  else if (StringIsEqual(type, "$PDGFTL1") ||
           StringIsEqual(type, "$PDGFTTL"))
    return PDGFTL1(line, info);

  return false;
}

static Device *
LeonardoCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new LeonardoDevice();
}

const struct DeviceRegister leonardo_driver = {
  _T("Leonardo"),
  _T("Digifly Leonardo"),
  0,
  LeonardoCreateOnPort,
};
