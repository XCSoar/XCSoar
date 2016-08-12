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

#include "Device/Driver/BorgeltB50.hpp"
#include "Device/Driver/CAI302/PocketNav.hpp"
#include "Device/Driver.hpp"
#include "Units/System.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "Util/Clamp.hpp"

#include <math.h>

class B50Device : public AbstractDevice {
  Port &port;

public:
  B50Device(Port &_port):port(_port) {}

  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;

  bool PutMacCready(double mc, OperationEnvironment &env) override;
  bool PutBugs(double bugs, OperationEnvironment &env) override;
  bool PutBallast(double fraction, double overload,
                  OperationEnvironment &env) override;
};

/*
Sentence has following format:

$PBB50,AAA,BBB.B,C.C,DDDDD,EE,F.FF,G,HH*CHK crlf

AAA = TAS 0 to 150 knots
BBB.B = Vario, -10 to +15 knots, negative sign for sink
C.C = MacCready 0 to 8.0 knots
DDDDD = IAS squared 0 to 22500
EE = bugs degradation, 0 = clean to 30 %
F.FF = Ballast 1.00 to 1.60
G = 0 in climb, 1 in cruise
HH = Outside airtemp in degrees celcius ( may have leading negative sign )
CHK = standard NMEA checksum
*/
static bool
PBB50(NMEAInputLine &line, NMEAInfo &info)
{
  // $PBB50,100,0,10,1,10000,0,1,0,20*4A..
  // $PBB50,0,.0,.0,0,0,1.07,0,-228*58
  // $PBB50,14,-.2,.0,196,0,.92,0,-228*71

  double vtas, value;

  bool vtas_av = line.ReadChecked(vtas);

  if (line.ReadChecked(value))
    info.ProvideTotalEnergyVario(Units::ToSysUnit(value, Unit::KNOTS));

  if (line.ReadChecked(value))
    info.settings.ProvideMacCready(Units::ToSysUnit(value, Unit::KNOTS),
                                   info.clock);

  /// @todo: OLD_TASK device MC/bugs/ballast is currently not implemented, have to push MC to master
  ///  oldGlidePolar::SetMacCready(info.MacCready);

  if (line.ReadChecked(value) && vtas_av)
    info.ProvideBothAirspeeds(Units::ToSysUnit(sqrt(value), Unit::KNOTS),
                              Units::ToSysUnit(vtas, Unit::KNOTS));
  else if (vtas_av)
    info.ProvideTrueAirspeed(Units::ToSysUnit(vtas, Unit::KNOTS));

  // RMN: Changed bugs-calculation, swapped ballast and bugs to suit
  // the B50-string for Borgelt, it's % degradation, for us, it is %
  // of max performance

  if (line.ReadChecked(value))
    info.settings.ProvideBugs(1 - Clamp(value, 0., 30.) / 100.,
                              info.clock);

  double ballast_overload;
  if (line.ReadChecked(ballast_overload))
    info.settings.ProvideBallastOverload(ballast_overload, info.clock);

  // inclimb/incruise 1=cruise,0=climb, OAT
  switch (line.Read(-1)) {
  case 0:
    info.switch_state.flight_mode = SwitchState::FlightMode::CRUISE;
    break;

  case 1:
    info.switch_state.flight_mode = SwitchState::FlightMode::CIRCLING;
    break;
  }

  info.temperature_available = line.ReadChecked(value);
  if (info.temperature_available)
    info.temperature = Temperature::FromCelsius(value);

  return true;
}

bool
B50Device::ParseNMEA(const char *String, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(String))
    return false;

  NMEAInputLine line(String);
  char type[16];
  line.Read(type, 16);

  if (StringIsEqual(type, "$PBB50"))
    return PBB50(line, info);
  else
    return false;
}

bool
B50Device::PutMacCready(double mac_cready, OperationEnvironment &env)
{
  /* the Borgelt B800 understands the CAI302 "!g" command for
     MacCready, ballast and bugs */

  return CAI302::PutMacCready(port, mac_cready, env);
}

bool
B50Device::PutBugs(double bugs, OperationEnvironment &env)
{
  /* the Borgelt B800 understands the CAI302 "!g" command for
     MacCready, ballast and bugs */

  return CAI302::PutBugs(port, bugs, env);
}

bool
B50Device::PutBallast(double fraction, gcc_unused double overload,
                      OperationEnvironment &env)
{
  /* the Borgelt B800 understands the CAI302 "!g" command for
     MacCready, ballast and bugs */

  return CAI302::PutBallast(port, fraction, env);
}

static Device *
B50CreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new B50Device(com_port);
}

const struct DeviceRegister b50_driver = {
  _T("Borgelt B50"),
  _T("Borgelt B50/B800"),
  DeviceRegister::RECEIVE_SETTINGS | DeviceRegister::SEND_SETTINGS,
  B50CreateOnPort,
};
