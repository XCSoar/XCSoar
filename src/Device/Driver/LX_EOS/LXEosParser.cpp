// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Geo/SpeedVector.hpp"
#include "LXEosDevice.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "Units/System.hpp"
#include "util/Macros.hpp"
#include "util/StringCompare.hxx"

bool
LXEosDevice::LXWP0(NMEAInputLine& line, NMEAInfo& info)
{
  /*
   * $LXWP0,Y,119.4,1717.6,0.02,0.02,0.02,0.02,0.02,0.02,,000,107.2*5b
   *
   * <is_logger_running> char     'Y'=yes, 'N'=no
   * <tas>               float    True airspeed in km/h
   * <altitude>          float    True altitude in meters
   * <varioN>            float    6 measurements of vario in last second in m/s
   * <heading>           uint16_t True heading in deg. Blank if no compass.
   * <wind_direction>    string   Wind dir in deg. Blank if spd is 0.0.
   * <wind_speed>        string   Wind speed in km/h. Blank if wind speed is 0.
   */

  line.Skip();

  double airspeed;
  bool tas_ok = line.ReadChecked(airspeed);

  double altitude_true;
  if (line.ReadChecked(altitude_true)) {
    /* If alt_offset is known, recalculate to standard altitude and provide it
    Otherwise provide true altitude */
    if (altitude_offset.known && altitude_offset.reliable) {
      info.ProvidePressureAltitude(altitude_true - altitude_offset.meters);
    } else {
      info.ProvideBaroAltitudeTrue(altitude_true);
    }
  }

  if (tas_ok)
    info.ProvideTrueAirspeed(
      Units::ToSysUnit(airspeed, Unit::KILOMETER_PER_HOUR));

  double vario = 0;
  bool vario_ok = true;
  double value = 0;
  // Filter the 6 reading using 5th order low-pass FIR filter
  static const double fir_coefficients[] = { -0.0421, 0.1628, 0.3793, 0.3793, 0.1628, -0.0421 };
  for (double fir_b : fir_coefficients) {
    vario_ok = vario_ok && line.ReadChecked(value);
    vario += value * fir_b;
  }

  if (vario_ok)
    info.ProvideTotalEnergyVario(vario);

  line.Skip(1); // Heading
  line.Skip(1); // Eos seems to put one more empty value that is not documented

  bool wind_ok = true;
  double wind_direction, wind_speed;
  wind_ok = line.ReadChecked(wind_direction);
  wind_ok = wind_ok && line.ReadChecked(wind_speed);
  SpeedVector wind;
  if (wind_ok) {
    wind.bearing = Angle::Degrees(wind_direction);
    wind.norm = Units::ToSysUnit(wind_speed, Unit::KILOMETER_PER_HOUR);
    info.ProvideExternalWind(wind);
  }

  return true;
}

bool
LXEosDevice::LXWP2(NMEAInputLine& line, NMEAInfo& info)
{
  /*
   * $LXWP2,
   * <mc>          float    MacCready factor/s
   * <load_factor> float    Total glider mass divided by polar reference mass
   * <bugs>        uint16_t Bugs factor in percent
   * <polar_a>     float    Polar -square coefficient, velocity in m/s
   * <polar_b>     float    Polar -linear coefficient, velocity in m/s
   * <polar_c>     float    Polar -constant coefficient, velocity in m/s
   * <volume>      uint8_t  Variometer volume in percent
   */

  /*
   * Ballast is expressed as Total glider mass divided by polar reference mass.
   * Apart from water ballast, it also includes pilot weight as set in vario).
   * Reference mass of polar in vario may differ from XCSoar.
   */

  double mc, bal, bugs, polar_a, polar_b, polar_c;

  if (!line.ReadChecked(mc))
    return false;
  if (!line.ReadChecked(bal))
    return false;
  if (!line.ReadChecked(bugs))
    return false;
  if (!line.ReadChecked(polar_a))
    return false;
  if (!line.ReadChecked(polar_b))
    return false;
  if (!line.ReadChecked(polar_c))
    return false;
  line.Skip(1); // Vario volume

  /*
   * Sending setting from XCSoar to vario requires sending MC, Bugs, and
   * Ballast all in one sentence. It is not possible to set one without
   * affecting the others. Last received settings are stored in vario_settings
   * struct. These will be used to set the other values when changing one of
   * them.
   */
  const std::lock_guard lock{ settings_mutex };

  info.settings.ProvideMacCready(mc, info.clock);
  double bugs_xcsoar = static_cast<double>(100 - bugs) / 100.0;
  info.settings.ProvideBugs(bugs_xcsoar, info.clock);

  PolarCoefficients polar(polar_a, polar_b, polar_c);

  if (!ComparePolarCoefficients(polar, vario_settings.device_polar)) {
    vario_settings.device_polar = polar;
    vario_settings.device_reference_mass_uptodate = false;
  }

  if(vario_settings.device_reference_mass_uptodate == false)
    CalculateDevicePolarReferenceMass(vario_settings);

  if (vario_settings.device_reference_mass_uptodate) {
    float total_mass = bal * vario_settings.device_reference_mass;
    float ballast_mass = total_mass - vario_settings.xcsoar_polar.GetDryMass();
    if(ballast_mass < 0)
      ballast_mass = 0;
    info.settings.ProvideBallastLitres(ballast_mass, info.clock);
  }

  vario_settings.mc = mc;
  vario_settings.bal = bal;
  vario_settings.bugs = bugs; // The original value (in percent) is stored
  vario_settings.uptodate = true;

  return true;
}

bool
LXEosDevice::LXWP3(NMEAInputLine& line, NMEAInfo& info)
{
  /*
   * $LXWP3,
   * <alt_offset>  int16_t Difference between true and std. alt. in feet
   * <sc_mode>     uint8_t  SC mode. 0 = manual, 1 = circling, 2 = speed
   * <filter>      float    SC filter factor in seconds
   * <reserved>    Reserved
   * <te_level>    uint16_t TE level in percent
   * <int_time>    uint16_t SC integration time in seconds
   * <range>       uint8_t  SC range in m/s
   * <silence>     float    SC silence in m/s
   * <switch_mode> uint8_t  SC switch mode. 0 = off, 1 = on, 2 = toggle.
   * <speed>       uint16_t SC speed in km/h
   * <polar_name>  string   Self explanatory
   * <reserved>    Reserved
   */

  double value;
  if (!line.ReadChecked(value))
    return false;

  // Update settings only if alt_offset changed from the last time
  if (!altitude_offset.known || value != altitude_offset.last_received) {
    altitude_offset.last_received = value;
    altitude_offset.meters = Units::ToSysUnit(value, Unit::FEET);
    auto qnh = AtmosphericPressure::PressureAltitudeToStaticPressure(
      -altitude_offset.meters);
    info.settings.ProvideQNH(qnh, info.clock);
    altitude_offset.known = true;
  }

  return true;
}
