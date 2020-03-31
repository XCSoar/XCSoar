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
  bool ComposeWrite(const char p_type,double a, double b, double c,
  		  OperationEnvironment &env);
  bool PutIdealPolar(bool force, const DerivedInfo *calculated,
  		  OperationEnvironment &env);
  bool PutRealPolar(bool force, const DerivedInfo *calculated,
  		  OperationEnvironment &env);
  bool RepeatBugs(OperationEnvironment &env);
  bool RepeatBallast(OperationEnvironment &env);
  bool RepeatMacCready(OperationEnvironment &env);

  // remember the settings to be able to repeat the most recent values upon request
  double _bugs = 1;
  bool   _bugs_valid = false;
  double _mc = 1;
  bool   _mc_valid = false;
  double _overload = 1;
  bool   _overload_valid = false;
  const DerivedInfo *_calculated = NULL;
};


bool
OpenVarioDevice::ComposeWrite(const char p_type,double a, double b, double c,
  OperationEnvironment &env)
{
  // NullOperationEnvironment env;
  char buffer[50];
  // Compose Polar String
  // TODO does the receiver understand %e format? Or do we have to use %f, even if
  //      the number of significant digits is less then 3.
  sprintf(buffer,"POV,C,%cPO,%e,%e,%e", p_type, a, b, c);
  return PortWriteNMEA(port, buffer, env);
}


bool
OpenVarioDevice::PutIdealPolar(bool force, const DerivedInfo *calculated,
  OperationEnvironment &env)
{
  bool rv = false;
  if (calculated == NULL) return rv;

  // Get ideal polar
  PolarCoefficients polar = calculated->glide_polar_safety.GetCoefficients();
  static double prev_a=0, prev_b=0, prev_c=0;
  if (force
     || (prev_a != polar.a)
     || (prev_b != polar.b)
     || (prev_c != polar.c)) {
       if (OpenVarioDevice::ComposeWrite('I',polar.a,polar.b,polar.c,env)){
	prev_a = polar.a; prev_b = polar.b; prev_c = polar.c;
	rv |= true;
       }
     }
  return rv;
}

bool
OpenVarioDevice::PutRealPolar(bool force, const DerivedInfo *calculated,
  OperationEnvironment &env)
{
  bool rv = false;
  if (calculated == NULL) return rv;

  // Get real polar
  PolarCoefficients polar = calculated->glide_polar_safety.GetRealCoefficients();
  static double prev_a=0, prev_b=0, prev_c=0;
  if (force
     || (prev_a != polar.a)
     || (prev_b != polar.b)
     || (prev_c != polar.c)) {
       if (OpenVarioDevice::ComposeWrite('R',polar.a,polar.b,polar.c,env)){
	prev_a = polar.a; prev_b = polar.b; prev_c = polar.c;
	rv |= true;
       }
     }
  return rv;
}

void
OpenVarioDevice::OnCalculatedUpdate(const MoreData &basic, 
    const DerivedInfo &calculated)
{
  _calculated = &calculated;
  NullOperationEnvironment env;

  // PutIdealPolar(false, _calculated, env);
  PutRealPolar(false, _calculated, env);
}

bool
OpenVarioDevice::RepeatMacCready(OperationEnvironment &env)
{
  if ((!EnableNMEA(env)) || (!_mc_valid))
    return false;
  
  char buffer[20];
  sprintf(buffer,"POV,C,MC,%0.2f", (double)_mc);
  return PortWriteNMEA(port, buffer, env);
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
  if ((!EnableNMEA(env)) || (!_overload_valid))
    return false;
  
  char buffer[20];
  sprintf(buffer,"POV,C,WL,%1.3f",(float)_overload);
  return PortWriteNMEA(port, buffer, env);
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
  if ((!EnableNMEA(env)) || (!_bugs_valid))
    return false;
 
  char buffer[32];
  sprintf(buffer, "POV,C,BU,%0.2f",(float)_bugs);
  return PortWriteNMEA(port, buffer, env);
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
   */


  while (!line.IsEmpty()) {
    char type = line.ReadOneChar();
    if (type == '\0')
      break;

    double value;
    if (!line.ReadChecked(value))
      break;

    switch (type) {
      case '?': {
	  NullOperationEnvironment env;
	  int sel = (int)(value+0.1);
	  if (sel & 0x10) RepeatBallast(env);
	  if (sel & 0x08) RepeatBugs(env);
	  if (sel & 0x04) RepeatMacCready(env);
	  if (sel & 0x02) PutIdealPolar(true, _calculated, env);
	  if (sel & 0x01) PutRealPolar(true, _calculated, env);
	  // TODO : what else might the connected device want to know ?
        break;
      }
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
