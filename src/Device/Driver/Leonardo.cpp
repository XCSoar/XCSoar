// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/Leonardo.hpp"
#include "Device/Driver.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "Units/System.hpp"

using std::string_view_literals::operator""sv;

class LeonardoDevice : public AbstractDevice {
public:
  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
};

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
  if (SpeedVector wind; line.ReadSwappedSpeedVectorKPH(wind))
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
  if (SpeedVector wind; line.ReadSwappedSpeedVectorKPH(wind))
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

  const auto type = line.ReadView();
  if (type == "$C"sv || type == "$c"sv)
    return LeonardoParseC(line, info);

  else if (type == "$D"sv || type == "$d"sv)
    return LeonardoParseD(line, info);

  else if (type == "$PDGFTL1"sv || type == "$PDGFTTL"sv)
    return PDGFTL1(line, info);

  else
    return false;
}

static Device *
LeonardoCreateOnPort([[maybe_unused]] const DeviceConfig &config, [[maybe_unused]] Port &com_port)
{
  return new LeonardoDevice();
}

const struct DeviceRegister leonardo_driver = {
  "Leonardo",
  "Digifly Leonardo",
  0,
  LeonardoCreateOnPort,
};
