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

#include "Device/Driver/XCVario.hpp"
#include "Device/Driver/CAI302/PocketNav.hpp"
#include "Device/Driver.hpp"
#include "Units/System.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/Info.hpp"
#include "Device/Port/Port.hpp"
#include "NMEA/InputLine.hpp"
#include "util/Clamp.hpp"
#include "Atmosphere/Pressure.hpp"
#include <math.h>

class XVCDevice : public AbstractDevice {
  Port &port;

public:
  XVCDevice(Port &_port):port(_port) {}

  // virtual methods from class Device
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;

  bool PutMacCready(double mc, OperationEnvironment &env) override;
  bool PutBugs(double bugs, OperationEnvironment &env) override;
  bool PutBallast(double fraction, double overload, OperationEnvironment &env) override;
  bool PutQNH(const AtmosphericPressure &pres, OperationEnvironment &env) override;
};

/*
  For a complete documentation of the protocol refer to https://xcvario.de/handbuch and
  search for NMEA Protokoll.
  
  Sentence has following format:
  $PXCV,
  BBB.B  = Vario, -30 to +30 m/s, negative sign for sink,
  C.C    = MacCready 0 to 10 m/s
  EE     = bugs degradation, 0 = clean to 30 %,
  F.FF   = Ballast 1.00 to 1.60,
  G      = 0 in climb, 1 in cruise,
  HH.H   = Outside airtemp in degrees celcius ( may have leading negative sign ) e.g. 24.4,
  QQQQ.Q = QNH in hectoPascal e.g. 1013.2,
  PPPP.P = static pressure in hPa,
  RRR.R  = roll angle in degree related to earth system,
  III.I  = pitch angle related to earth system,
  X.XX   = acceleration in X-Axis (multiples of G),
  Y.YY   = acceleration in Y-Axis,
  Z.ZZ   = acceleration in Z-Axis,
  *,
  CHK    = standard NMEA checksum, CR,LF
 */
static bool
PXCV(NMEAInputLine &line, NMEAInfo &info)
{
  // Format as defined above.
  double value;
  double x, y, z;

  // Kalman filtered TE Vario value in m/s
  if (line.ReadChecked(value))
    info.ProvideTotalEnergyVario(value);

  // MC value as set in XCVario in m/s
  if (line.ReadChecked(value))
    info.settings.ProvideMacCready(value, info.clock);

  // RMN: Changed bugs-calculation, swapped ballast and bugs to suit
  // the XVC-string for Borgelt, it's % degradation, for us, it is %
  // of max performance

  // Bugs setting as entered in XCVario
  if (line.ReadChecked(value))
    info.settings.ProvideBugs(1 - Clamp(value, 0., 30.) / 100.,
                              info.clock);

  // Ballast overload value in %
  double ballast_overload;
  if (line.ReadChecked(ballast_overload))
    info.settings.ProvideBallastOverload(ballast_overload, info.clock);

  // inclimb/incruise 1=cruise,0=climb, OAT
  switch (line.Read(-1)) {
  case 0:
    info.switch_state.flight_mode = SwitchState::FlightMode::CIRCLING;
    break;

  case 1:
    info.switch_state.flight_mode = SwitchState::FlightMode::CRUISE;
    break;
  }

  // Outside air temperature
  info.temperature_available = line.ReadChecked(value);
  if (info.temperature_available)
    info.temperature = Temperature::FromCelsius(value);

  // QNH as set or autoset in XCVario
  if (line.ReadChecked(value))
    info.settings.ProvideQNH(AtmosphericPressure::HectoPascal(value), info.clock);

  // Barometric pressure
  if (line.ReadChecked(value))
    info.ProvideStaticPressure(AtmosphericPressure::HectoPascal(value));

  // Pitot tube dynamic airspeed pressure
  if (line.ReadChecked(value))
    info.ProvideDynamicPressure(AtmosphericPressure::Pascal(value));

  // Roll respect to Earth system - Phi [°] (i.e. +110)
  if (line.ReadChecked(value)) {
    info.attitude.bank_angle_available.Update(info.clock);
    info.attitude.bank_angle = Angle::Degrees(value);
  }
  // Pitch angle respect to Earth system - Theta [°] (i.e.+020)
  if (line.ReadChecked(value)) {
    info.attitude.pitch_angle_available.Update(info.clock);
    info.attitude.pitch_angle = Angle::Degrees(value);
  }
  // Space diagonal acceleration in X,Y,Z axes measure
  if ( line.ReadChecked(x) && line.ReadChecked(y) && line.ReadChecked(z) )
    info.acceleration.ProvideGLoad(SpaceDiagonal(x, y, z));

  return true;
}

bool
XVCDevice::ParseNMEA(const char *String, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(String))
    return false;

  NMEAInputLine line(String);
  char type[16];
  line.Read(type, 16);

  if (StringIsEqual(type, "$PXCV"))
    return PXCV(line, info);
  else
    return false;
}

// For documentation refer to chapter 10.1.3 Device Driver/XCVario in mulilingual handbook: https://xcvario.de/handbuch

bool 
XVCDevice::PutQNH(const AtmosphericPressure &pres, OperationEnvironment &env)
{
  /* the XCVario understands "!g,q<NNNN>" command for QNH updates with recent builds */
  char buffer[32];
  unsigned qnh = uround(pres.GetHectoPascal());
  int msg_len = sprintf(buffer,"!g,q%u\r", std::min(qnh,(unsigned)2000));
  port.FullWrite(buffer, msg_len, env, std::chrono::seconds(2) );
  return true;
}



bool
XVCDevice::PutMacCready(double mac_cready, OperationEnvironment &env)
{
  /* the XCVario understands the CAI302 "!g" command for
     MacCready, ballast and bugs */

  return CAI302::PutMacCready(port, mac_cready, env);
}

bool
XVCDevice::PutBugs(double bugs, OperationEnvironment &env)
{
  /* the XCVario understands the CAI302 "!g" command for
     MacCready, ballast and bugs */

  return CAI302::PutBugs(port, bugs, env);
}

bool
XVCDevice::PutBallast(double fraction, gcc_unused double overload,
                      OperationEnvironment &env)
{
  /* the XCVario understands CAI302 like command for ballast "!g,b" with
     float precision */
   char buffer[32];
   double ballast = fraction * 10.;
   int msg_len = sprintf(buffer,"!g,b%.3f\r", ballast );
   port.FullWrite(buffer, msg_len, env, std::chrono::seconds(2) );
  return true;
}

static Device *
XVCCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new XVCDevice(com_port);
}

const struct DeviceRegister xcv_driver = {
  _T("XCVario"),
  _T("XCVario"),
  DeviceRegister::RECEIVE_SETTINGS | DeviceRegister::SEND_SETTINGS,
  XVCCreateOnPort,
};


