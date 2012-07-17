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

#include "Internal.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Info.hpp"
#include "Engine/Navigation/SpeedVector.hpp"
#include "Units/System.hpp"
#include "Atmosphere/Temperature.hpp"

static bool
ReadSpeedVector(NMEAInputLine &line, SpeedVector &value_r)
{
  fixed bearing, norm;

  bool bearing_valid = line.read_checked(bearing);
  bool norm_valid = line.read_checked(norm);

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

  fixed value;

  line.skip();

  fixed airspeed;
  bool tas_available = line.read_checked(airspeed);
  if (tas_available && (airspeed < fixed(-50) || airspeed > fixed(250)))
    /* implausible */
    return false;

  fixed alt = fixed_zero;
  if (line.read_checked(alt))
    /* a dump on a LX7007 has confirmed that the LX sends uncorrected
       altitude above 1013.25hPa here */
    info.ProvidePressureAltitude(alt);

  if (tas_available)
    info.ProvideTrueAirspeedWithAltitude(Units::ToSysUnit(airspeed,
                                                          Unit::KILOMETER_PER_HOUR),
                                         alt);

  if (line.read_checked(value))
    info.ProvideTotalEnergyVario(value);

  line.skip(6);

  SpeedVector wind;
  if (ReadSpeedVector(line, wind))
    info.ProvideExternalWind(wind);

  return true;
}

static bool
LXWP1(gcc_unused NMEAInputLine &line, gcc_unused NMEAInfo &info)
{
  /*
   * $LXWP1,
   * serial number,
   * instrument ID,
   * software version,
   * hardware version,
   * license string
   */
  return true;
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

  fixed value;
  // MacCready value
  if (line.read_checked(value))
    info.settings.ProvideMacCready(value, info.clock);

  // Ballast
  if (line.read_checked(value))
    info.settings.ProvideBallastOverload(value, info.clock);

  // Bugs
  if (line.read_checked(value))
    info.settings.ProvideBugs((fixed(100) - value) / 100, info.clock);

  return true;
}

static bool
LXWP3(gcc_unused NMEAInputLine &line, gcc_unused NMEAInfo &info)
{
  /*
   * $LXWP3,
   * altioffset
   * scmode
   * variofil
   * tefilter
   * televel
   * varioavg.
   * glider name
   * time offset
   */
  return true;
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
  line.skip(4);

  fixed vario;
  if (line.read_checked(vario))
    info.ProvideNettoVario(vario);

  fixed ias;
  bool have_ias = line.read_checked(ias);

  fixed altitude;
  if (line.read_checked(altitude)) {
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
  fixed temperature;
  if (line.read_checked(temperature)) {
    info.temperature = CelsiusToKelvin(temperature);
    info.temperature_available = true;
  }

  int mode;
  if (line.read_checked(mode)) {
    if (mode == 0) {
      info.switch_state.flight_mode = SwitchInfo::FlightMode::CIRCLING;
      info.switch_state.speed_command = false;
      info.switch_state_available = true;
    } else if (mode == 1) {
      info.switch_state.flight_mode = SwitchInfo::FlightMode::CRUISE;
      info.switch_state.speed_command = true;
      info.switch_state_available = true;
    }
  }

  fixed voltage;
  if (line.read_checked(voltage)) {
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
  line.read(type, 16);

  if (StringIsEqual(type, "$LXWP0"))
    return LXWP0(line, info);

  if (StringIsEqual(type, "$LXWP1"))
    return LXWP1(line, info);

  if (StringIsEqual(type, "$LXWP2"))
    return LXWP2(line, info);

  if (StringIsEqual(type, "$LXWP3"))
    return LXWP3(line, info);

  if (StringIsEqual(type, "$PLXVF"))
    return PLXVF(line, info);

  if (StringIsEqual(type, "$PLXVS"))
    return PLXVS(line, info);

  return false;
}
