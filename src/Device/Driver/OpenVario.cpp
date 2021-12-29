/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Device/Driver/OpenVario.hpp"
#include "Device/Driver.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/InputLine.hpp"
#include "Units/System.hpp"
#include "Operation/Operation.hpp"
#include "LogFile.hpp"

class OpenVarioDevice : public AbstractDevice {
  Port &port;

public:
  OpenVarioDevice(Port &_port):port(_port) {}

  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, NMEAInfo &info) override;
  bool PutMacCready(double mc, OperationEnvironment &env) override;
  bool PutBallast(double fraction, double overload,
                  OperationEnvironment &env) override;
  bool PutBugs(double bugs, OperationEnvironment &env) override;
  void OnCalculatedUpdate(const MoreData &basic,
                  const DerivedInfo &calculated) override;
private:
  bool POV(NMEAInputLine &line, NMEAInfo &info);
  bool ComposeWrite(const char p_type,PolarCoefficients &p,
                  OperationEnvironment &env);
  bool InformUnavailable(const char *obj, OperationEnvironment &env);
  bool RepeatIdealPolar(OperationEnvironment &env);
  bool RepeatRealPolar(OperationEnvironment &env);
  bool PutIdealPolar(const DerivedInfo &calculated,
                  OperationEnvironment &env);
  bool PutRealPolar(const DerivedInfo &calculated,
                  OperationEnvironment &env);
  bool RepeatBugs(OperationEnvironment &env);
  bool RepeatBallast(OperationEnvironment &env);
  bool RepeatMacCready(OperationEnvironment &env);
  // all 3 coeffs equal between the 2 polars
  constexpr bool IsEqual(const PolarCoefficients &p1, const PolarCoefficients &p2);
  // copy the 3 coffeicients from src to dest
  constexpr void Copy(PolarCoefficients &dest, const PolarCoefficients &src);

  // remember the settings to be able to repeat the most recent values upon request
  double _bugs = 1;
  bool   _bugs_valid = false;
  double _mc = 1;
  bool   _mc_valid = false;
  double _overload = 1;
  bool   _overload_valid = false;
  PolarCoefficients _ideal_polar; 
  bool _ideal_polar_valid = false;
  PolarCoefficients _real_polar; 
  bool _real_polar_valid = false;
};

constexpr bool
OpenVarioDevice::IsEqual(const PolarCoefficients &p1, const PolarCoefficients &p2)
{
  if (p1.a != p2.a) return false;
  if (p1.b != p2.b) return false;
  if (p1.c != p2.c) return false;
  return true;
}

constexpr void
OpenVarioDevice::Copy(PolarCoefficients &dest, const PolarCoefficients &src)
{
  dest.a = src.a;
  dest.b = src.b;
  dest.c = src.c;
}

bool
OpenVarioDevice::ComposeWrite(const char p_type,PolarCoefficients &p,
  OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  char buffer[50];
  // Compose Polar String
  sprintf(buffer,"POV,C,%cPO,%f,%f,%f", p_type, p.a, p.b, p.c);
  PortWriteNMEA(port, buffer, env);
  return true;
}

bool
OpenVarioDevice::InformUnavailable(const char *obj, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  char buffer[20];
  const char preamble[] = "POV,C,NA,";

  if (sizeof(preamble) + strlen(obj) >= sizeof(buffer)) return false;

  strcpy(buffer,preamble);
  strcat(buffer,obj);

  PortWriteNMEA(port, buffer, env);
  return true;
}

bool
OpenVarioDevice::PutIdealPolar(const DerivedInfo &calculated,
  OperationEnvironment &env)
{
  bool rv = false;
  PolarCoefficients polar = calculated.glide_polar_safety.GetCoefficients();

  if (!IsEqual(polar,_ideal_polar) || !_ideal_polar_valid) {
    rv = ComposeWrite('I',polar,env);
    Copy(_ideal_polar,polar);
    _ideal_polar_valid = true;
    }
  return rv;
}

bool
OpenVarioDevice::RepeatIdealPolar(OperationEnvironment &env)
{
  if (!_ideal_polar_valid) {
    return InformUnavailable("IPO", env);
  } else {
    return ComposeWrite('I',_ideal_polar,env);
  }
}

bool
OpenVarioDevice::RepeatRealPolar(OperationEnvironment &env)
{
  if (!_real_polar_valid) {
    return InformUnavailable("RPO", env);
  } else {
    return ComposeWrite('R',_real_polar,env);
  }
}

bool
OpenVarioDevice::PutRealPolar(const DerivedInfo &calculated,
  OperationEnvironment &env)
{
  bool rv = false;
  PolarCoefficients polar = calculated.glide_polar_safety.GetRealCoefficients();

  if (!IsEqual(polar,_real_polar) || !_real_polar_valid) {
    rv = ComposeWrite('R',polar,env);
    Copy(_real_polar,polar);
    _real_polar_valid = true;
    }
  return rv;
}

void
OpenVarioDevice::OnCalculatedUpdate(const MoreData &basic, 
    const DerivedInfo &calculated)
{
  NullOperationEnvironment env;

  PutIdealPolar(calculated, env);
  PutRealPolar(calculated, env);

  if (!_mc_valid) {
    // this is the MacCready at start up
    _mc = calculated.glide_polar_safety.GetMC();
    _mc_valid = true;
    RepeatMacCready(env);
    }
}

bool
OpenVarioDevice::RepeatMacCready(OperationEnvironment &env)
{
  if (!_mc_valid) {
    return InformUnavailable("MC", env);
  }
  
  char buffer[20];
  sprintf(buffer,"POV,C,MC,%0.2f", (double)_mc);
  PortWriteNMEA(port, buffer, env);
  return true;
}

bool
OpenVarioDevice::PutMacCready(double mc, OperationEnvironment &env)
{
  _mc = mc;
  _mc_valid = true;
  return RepeatMacCready(env);
}

bool
OpenVarioDevice::RepeatBallast(OperationEnvironment &env)
{
  if (!_overload_valid) {
    return InformUnavailable("WL", env);
  }
  
  char buffer[20];
  sprintf(buffer,"POV,C,WL,%1.3f",(float)_overload);
  PortWriteNMEA(port, buffer, env);
  return true;
}

bool
OpenVarioDevice::PutBallast(double fraction, double overload, OperationEnvironment &env)
{
  _overload = overload;
  _overload_valid = true;
  return RepeatBallast(env);
}

bool
OpenVarioDevice::RepeatBugs(OperationEnvironment &env)
{
  if (!_bugs_valid) {
    return InformUnavailable("BU", env);
  }
  
  char buffer[32];
  sprintf(buffer, "POV,C,BU,%0.2f",(float)_bugs);
  PortWriteNMEA(port, buffer, env);
  return true;
}

bool
OpenVarioDevice::PutBugs(double bugs, OperationEnvironment &env)
{
  _bugs = bugs;
  _bugs_valid = true;
  return RepeatBugs(env);
}

bool
OpenVarioDevice::ParseNMEA(const char *_line, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(_line))
    return false;

  NMEAInputLine line(_line);
  if (line.ReadCompare("$POV"))
    return POV(line, info);

  return false;
}

bool
OpenVarioDevice::POV(NMEAInputLine &line, NMEAInfo &info)
{
  /*
   * Type definitions:
   *
   * E: TE vario in m/s
   * H: relative humidity in %
   * P: static pressure in hPa
   * Q: dynamic pressure in Pa
   * R: total pressure in hPa
   * S: true airspeed in km/h
   * T: temperature in deg C
   * ?: respond with selected strings, e.g. Ballast, Bugs, Polar
   *    Example: $POV,?,RPO,MC,WL*2E means: Real Polar, MacCready, Wing Load
   */


  while (!line.IsEmpty()) {
    char type = line.ReadOneChar();
    if (type == '\0')
      break;

    if (type == '?') {
      NullOperationEnvironment env;
      char query_item[5];

      for (int i=0;i < 10;i++) { // not more than 10 loops!
        line.Read(query_item,sizeof(query_item));
        if (strlen(query_item) == 0) return true;

        if (StringIsEqual(query_item,"WL")) RepeatBallast(env);
        else if (StringIsEqual(query_item,"BU")) RepeatBugs(env);
        else if (StringIsEqual(query_item,"MC")) RepeatMacCready(env);
        else if (StringIsEqual(query_item,"IPO")) RepeatIdealPolar(env);
        else if (StringIsEqual(query_item,"RPO")) RepeatRealPolar(env);
      }
      return false;
    }

    double value;
    if (!line.ReadChecked(value))
      break;

    switch (type) {
      case 'E': {
        info.ProvideTotalEnergyVario(value);
        break;
      }
      case 'H': {
          info.humidity_available = true;
          info.humidity = value;
          break;
      }
      case 'P': {
        AtmosphericPressure pressure = AtmosphericPressure::HectoPascal(value);
        info.ProvideStaticPressure(pressure);
        break;
      }
      case 'Q': {
        AtmosphericPressure pressure = AtmosphericPressure::Pascal(value);
        info.ProvideDynamicPressure(pressure);
        break;
      }
      case 'R': {
        AtmosphericPressure pressure = AtmosphericPressure::HectoPascal(value);
        info.ProvidePitotPressure(pressure);
        break;
      }
      case 'S': {
        value = Units::ToSysUnit(value, Unit::KILOMETER_PER_HOUR);
        info.ProvideTrueAirspeed(value);
        break;
      }
      case 'T': {
        info.temperature = Temperature::FromCelsius(value);
        info.temperature_available = true;
        break;
      }
      case 'V': {
        info.voltage = value;
        info.voltage_available.Update(info.clock);
        break;
      }
    }
  }

  return true;
}

static Device *
OpenVarioCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new OpenVarioDevice(com_port);
}

const struct DeviceRegister open_vario_driver = {
  _T("OpenVario"),
  _T("OpenVario"),
  DeviceRegister::SEND_SETTINGS,
  OpenVarioCreateOnPort,
};
