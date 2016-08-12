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

#include "Internal.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Info.hpp"
#include "Geo/SpeedVector.hpp"
#include "Units/System.hpp"
#include "Util/Macros.hpp"
#include "Util/StringCompare.hxx"

static bool
ReadSpeedVector(NMEAInputLine &line, SpeedVector &value_r)
{
  double bearing, norm;

  bool bearing_valid = line.ReadChecked(bearing);
  bool norm_valid = line.ReadChecked(norm);

  if (bearing_valid && norm_valid) {
    value_r.bearing = Angle::Degrees(bearing);
    value_r.norm = Units::ToSysUnit(norm, Unit::KILOMETER_PER_HOUR);
    return true;
  } else
    return false;
}

static bool
LXWP0(NMEAInputLine &line, NMEAInfo &info)
{
  /*
  $LXWP0,Y,222.3,1665.5,1.71,,,,,,239,174,10.1

   0 loger_stored (Y/N)
   1 IAS (kph) ----> Condor uses TAS!
   2 baroaltitude (m)
   3-8 vario (m/s) (last 6 measurements in last second)
   9 heading of plane
  10 windcourse (deg)
  11 windspeed (kph)
  */

  line.Skip();

  double airspeed;
  bool tas_available = line.ReadChecked(airspeed);
  if (tas_available && (airspeed < -50 || airspeed > 250))
    /* implausible */
    return false;
  double value;
  if (line.ReadChecked(value))
    /* a dump on a LX7007 has confirmed that the LX sends uncorrected
       altitude above 1013.25hPa here */
    info.ProvidePressureAltitude(value);

  if (tas_available)
    /*
     * Call ProvideTrueAirspeed() after ProvidePressureAltitude() to use
     * the provided altitude (if available)
     */
    info.ProvideTrueAirspeed(Units::ToSysUnit(airspeed, Unit::KILOMETER_PER_HOUR));

  if (line.ReadChecked(value))
    info.ProvideTotalEnergyVario(value);

  line.Skip(6);

  SpeedVector wind;
  if (ReadSpeedVector(line, wind))
    info.ProvideExternalWind(wind);

  return true;
}

template<size_t N>
static void
ReadString(NMEAInputLine &line, NarrowString<N> &value)
{
  line.Read(value.buffer(), value.capacity());
}

static void
LXWP1(NMEAInputLine &line, DeviceInfo &device)
{
  /*
   * $LXWP1,
   * instrument ID,
   * serial number,
   * software version,
   * hardware version,
   * license string
   */

  ReadString(line, device.product);
  ReadString(line, device.serial);
  ReadString(line, device.software_version);
  ReadString(line, device.hardware_version);
}

static bool
LXWP2(NMEAInputLine &line, NMEAInfo &info)
{
  /*
   * $LXWP2,
   * maccready value, (m/s)
   * ballast, (1.0 - 1.5)
   * bugs, (0 - 100%)
   * polar_a,
   * polar_b,
   * polar_c,
   * audio volume
   */

  double value;
  // MacCready value
  if (line.ReadChecked(value))
    info.settings.ProvideMacCready(value, info.clock);

  // Ballast
  if (line.ReadChecked(value))
    info.settings.ProvideBallastOverload(value, info.clock);

  // Bugs
  if (line.ReadChecked(value)) {
    if (value <= 1.5 && value >= 1.0)
      // LX160 (sw 3.04) reports bugs as 1.00, 1.05 or 1.10 (#2167)
      info.settings.ProvideBugs(2 - value, info.clock);
    else
      // All other known LX devices report bugs as 0, 5, 10, 15, ...
      info.settings.ProvideBugs((100 - value) / 100., info.clock);
  }

  line.Skip(3);

  unsigned volume;
  if (line.ReadChecked(volume))
    info.settings.ProvideVolume(volume, info.clock);

  return true;
}

static bool
LXWP3(NMEAInputLine &line, NMEAInfo &info)
{
  /*
   * $LXWP3,
   * altioffset
   * scmode
   * variofil
   * tefilter
   * televel
   * varioavg
   * variorange
   * sctab
   * sclow
   * scspeed
   * SmartDiff
   * glider name
   * time offset
   */

  double value;

  // Altitude offset -> QNH
  if (line.ReadChecked(value)) {
    value = Units::ToSysUnit(-value, Unit::FEET);
    auto qnh = AtmosphericPressure::PressureAltitudeToStaticPressure(value);
    info.settings.ProvideQNH(qnh, info.clock);
  }

  return true;
}

/**
 * Parse the $PLXV0 sentence (LXNav V7).
 */
static bool
PLXV0(NMEAInputLine &line, DeviceSettingsMap<std::string> &settings)
{
  char name[64];
  line.Read(name, ARRAY_SIZE(name));
  if (StringIsEmpty(name))
    return true;

  char type[2];
  line.Read(type, ARRAY_SIZE(type));
  if (type[0] != 'W')
    return true;

  const auto value = line.Rest();

  settings.Lock();
  settings.Set(name, std::string(value.begin(), value.end()));
  settings.Unlock();
  return true;
}

static void
ParseNanoInfo(NMEAInputLine &line, DeviceInfo &device)
{
  ReadString(line, device.product);
  ReadString(line, device.software_version);
  line.Skip(); /* ver.date, e.g. "May 12 2012 21:38:28" */
  ReadString(line, device.hardware_version);
}

/**
 * Parse the $PLXVC sentence (LXNAV Nano).
 *
 * $PLXVC,<key>,<type>,<values>*<checksum><cr><lf>
 */
static void
PLXVC(NMEAInputLine &line, DeviceInfo &device,
      DeviceInfo &secondary_device,
      DeviceSettingsMap<std::string> &settings)
{
  char key[64];
  line.Read(key, ARRAY_SIZE(key));

  char type[2];
  line.Read(type, ARRAY_SIZE(type));
  if (StringIsEqual(key, "SET") && type[0] == 'A') {
    char name[64];
    line.Read(name, ARRAY_SIZE(name));

    const auto value = line.Rest();
    if (!StringIsEmpty(name)) {
      settings.Lock();
      settings.Set(name, std::string(value.begin(), value.end()));
      settings.Unlock();
    }
  } else if (StringIsEqual(key, "INFO") && type[0] == 'A') {
    ParseNanoInfo(line, device);
  } else if (StringIsEqual(key, "GPSINFO") && type[0] == 'A') {
    /* the LXNAV V7 (firmware >= 2.01) forwards the Nano's INFO
       sentence with the "GPS" prefix */

    char name[64];
    line.Read(name, ARRAY_SIZE(name));

    if (StringIsEqual(name, "LXWP1")) {
      LXWP1(line, secondary_device);
    } else if (StringIsEqual(name, "INFO")) {
      line.Read(type, ARRAY_SIZE(type));
      if (type[0] == 'A')
        ParseNanoInfo(line, secondary_device);
    }
  }
}

/**
 * Parse the $PLXVF sentence (LXNav V7).
 *
 * $PLXVF,time ,AccX,AccY,AccZ,Vario,IAS,PressAlt*CS<CR><LF>
 *
 * Example: $PLXVF,1.00,0.87,-0.12,-0.25,90.2,244.3,*CS<CR><LF>
 *
 * @see http://www.xcsoar.org/trac/raw-attachment/ticket/1666/V7%20dataport%20specification%201.97.pdf
 */
static bool
PLXVF(NMEAInputLine &line, NMEAInfo &info)
{
  line.Skip(4);

  double vario;
  if (line.ReadChecked(vario))
    info.ProvideNettoVario(vario);

  double ias;
  bool have_ias = line.ReadChecked(ias);

  double altitude;
  if (line.ReadChecked(altitude)) {
    info.ProvidePressureAltitude(altitude);

    if (have_ias)
      info.ProvideIndicatedAirspeedWithAltitude(ias, altitude);
  }

  return true;
}

/**
 * Parse the $PLXVS sentence (LXNav V7).
 *
 * $PLXVS,OAT,mode,voltage *CS<CR><LF>
 *
 * Example: $PLXVS,23.1,0,12.3,*CS<CR><LF>
 *
 * @see http://www.xcsoar.org/trac/raw-attachment/ticket/1666/V7%20dataport%20specification%201.97.pdf
 */
static bool
PLXVS(NMEAInputLine &line, NMEAInfo &info)
{
  double temperature;
  if (line.ReadChecked(temperature)) {
    info.temperature = Temperature::FromCelsius(temperature);
    info.temperature_available = true;
  }

  int mode;
  info.switch_state.flight_mode = SwitchState::FlightMode::UNKNOWN;
  if (line.ReadChecked(mode)) {
    if (mode == 0)
      info.switch_state.flight_mode = SwitchState::FlightMode::CIRCLING;
    else if (mode == 1)
      info.switch_state.flight_mode = SwitchState::FlightMode::CRUISE;
  }

  double voltage;
  if (line.ReadChecked(voltage)) {
    info.voltage = voltage;
    info.voltage_available.Update(info.clock);
  }

  return true;
}

bool
LXDevice::ParseNMEA(const char *String, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(String))
    return false;

  NMEAInputLine line(String);
  char type[16];
  line.Read(type, 16);

  if (StringIsEqual(type, "$LXWP0"))
    return LXWP0(line, info);

  if (StringIsEqual(type, "$LXWP1")) {
    /* if in pass-through mode, assume that this line was sent by the
       secondary device */
    DeviceInfo &device_info = mode == Mode::PASS_THROUGH
      ? info.secondary_device
      : info.device;
    LXWP1(line, device_info);

    const bool saw_v7 = device_info.product.equals("V7");
    const bool saw_nano = device_info.product.equals("NANO") ||
                            device_info.product.equals("NANO3");
    const bool saw_lx16xx = device_info.product.equals("1606") ||
                             device_info.product.equals("1600");

    if (mode == Mode::PASS_THROUGH) {
      /* in pass-through mode, we should never clear the V7 flag,
         because the V7 is still there, even though it's "hidden"
         currently */
      is_v7 |= saw_v7;
      is_nano |= saw_nano;
      is_lx16xx |= saw_lx16xx;
      is_forwarded_nano = saw_nano;
    } else {
      is_v7 = saw_v7;
      is_nano = saw_nano;
      is_lx16xx = saw_lx16xx;
    }

    if (saw_v7 || saw_nano || saw_lx16xx)
      is_colibri = false;

    return true;
  }

  if (StringIsEqual(type, "$LXWP2"))
    return LXWP2(line, info);

  if (StringIsEqual(type, "$LXWP3"))
    return LXWP3(line, info);

  if (StringIsEqual(type, "$PLXV0")) {
    is_v7 = true;
    is_colibri = false;
    return PLXV0(line, v7_settings);
  }

  if (StringIsEqual(type, "$PLXVC")) {
    is_nano = true;
    is_colibri = false;
    PLXVC(line, info.device, info.secondary_device, nano_settings);
    is_forwarded_nano = info.secondary_device.product.equals("NANO") ||
                          info.secondary_device.product.equals("NANO3");
    return true;
  }

  if (StringIsEqual(type, "$PLXVF")) {
    is_v7 = true;
    is_colibri = false;
    return PLXVF(line, info);
  }

  if (StringIsEqual(type, "$PLXVS")) {
    is_v7 = true;
    is_colibri = false;
    return PLXVS(line, info);
  }

  return false;
}
