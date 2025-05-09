// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/XCVario.hpp"
#include "Device/Driver/CAI302/PocketNav.hpp"
#include "Device/Driver.hpp"
#include "Units/System.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/Info.hpp"
#include "Device/Port/Port.hpp"
#include "NMEA/InputLine.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Operation/Operation.hpp"

#include <algorithm> // for std::clamp()

#include <math.h>

using std::string_view_literals::operator""sv;

class XVCDevice : public AbstractDevice {
  Port &port;
  enum {
    XCV_VERSION_UNKNOWN,  // as long device has not yet answered and no timeout
    XCV_VERSION_1,        // exchange legacy format with fractional ballast in $PXCV plus CAI302 !g command
    XCV_VERSION_2         // exchange absolute ballast in liters by !xc[s|v] messages
  };

  public:
  XVCDevice(Port &_port):port(_port) {};
  // virtual methods from class Device
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
  bool PutMacCready(double mc, OperationEnvironment &env) override;
  bool PutBugs(double bugs, OperationEnvironment &env) override;
  bool PutBallast(double fraction, double overload, OperationEnvironment &env) override;
  bool PutQNH(const AtmosphericPressure &pres, OperationEnvironment &env) override;
  bool EnableNMEA(OperationEnvironment &env) override;
  void OnSysTicker() override;
  void LinkTimeout() override;
  bool SendNMEAItem(const char *which, unsigned int value);

private:
  unsigned int protocol_negotiation_timer{5};         // timer to supervise $PXCV protocol talking peer on application level
  unsigned int protocol_version{XCV_VERSION_UNKNOWN}; // the protocol version negotiated between XCSoar and XCVario
  bool nmea_enable{false};                            // EnableNMEA() even has been received
  bool xcvario_protocol_up{false};                    // application level XCVario protocol $PXCV msg received

  bool PXCV(NMEAInputLine &line, NMEAInfo &info);
  bool XCV(NMEAInputLine &line, NMEAInfo &info);
  bool PutProtocolVersion(int version);
};

/*
  For a complete documentation of the protocol refer to https://xcvario.de/handbuch and
  search for NMEA Protokoll.

  Protocol XCVario

  Vario Sensor Data Sentence has following format:
  $PXCV,
  BBB.B  = Vario, -30 to +30 m/s, negative sign for sink,
  C.C    = MacCready 0 to 10 m/s
  EE     = bugs degradation, 0 = clean to 30 %,
  F.FF  = Ballast 1.00 to 1.60 ( for protocol version 1, empty in protocol version 2)
  G      = 0 in climb, 1 in cruise,
  HH.H   = Outside airtemp in degrees celcius ( may have leading negative sign ) e.g. 24.4,
  QQQQ.Q = QNH in hectoPascal e.g. 1013.2,
  PPPP.P = static pressure in hPa,
  RRR.R  = roll angle in degree related to earth system,
  III.I  = pitch angle related to earth system,
  X.XX   = acceleration in X-Axis (multiples of G),
  Y.YY   = acceleration in Y-Axis,
  Z.ZZ   = acceleration in Z-Axis,
  *
  CHK    = standard NMEA checksum
  CR,LF

  Exchange of Ballast and protocol version:
  !xc[s|v],bal-water,NNN*CHK   // exchange ballast NNN (integer) of glider in litres [s|v]: sender, s=XCSoar, v=XCVario
  !xc[s|v],version,N*CHK       // xcv protocol version; N=1 for legacy, N=2 for new ballast protocol

 */
bool
XVCDevice::PXCV(NMEAInputLine &line, NMEAInfo &info)
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
    info.settings.ProvideBugs(1 - std::clamp(value, 0., 30.) / 100.,
                              info.clock);

  // legacy reading of fractional water ballast in protocol version 1
  if (protocol_version == XCV_VERSION_1) {
    double ballast_overload;
    if (line.ReadChecked(ballast_overload))   // Parse ballast overload value
      info.settings.ProvideBallastOverload(ballast_overload, info.clock);
  } else {
    line.Read(-1);    // starting protocol version 2 we ignore this field
  }

  // inclimb/incruise 1=cruise,0=climb
  switch (line.Read(-1)) {
  case 1:
    info.switch_state.flight_mode = SwitchState::FlightMode::CRUISE;
    break;
  case 0:
    info.switch_state.flight_mode = SwitchState::FlightMode::CIRCLING;
    break;
  }

  // Outside air temperature (OAT)
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
  if (line.ReadChecked(x) && line.ReadChecked(y) && line.ReadChecked(z))
    info.acceleration.ProvideGLoad(SpaceDiagonal(x, y, z));

  return true;
}

bool
XVCDevice::XCV(NMEAInputLine &line, NMEAInfo &info)
{
  const auto topic = line.ReadView();
  if (topic == "bal-water"sv) {
    double value;
    if (line.ReadChecked(value)) {
      info.settings.ProvideBallastLitres(value, info.clock);
    }
  } else if (topic == "version"sv) {
    unsigned int value;
    if (line.ReadChecked(value)) {
      protocol_version = std::min(value, (unsigned int)XCV_VERSION_2);  // switch protocol version to the minimum version both can do
    }
  }
  return true;
}

void XVCDevice::LinkTimeout()
{
  protocol_version = XCV_VERSION_UNKNOWN;   // on link timeout protocol negotiation is restarted
  protocol_negotiation_timer = 5;                           // link timeout ticks = 5 seconds
  xcvario_protocol_up = false;
  nmea_enable = false;
}

bool XVCDevice::EnableNMEA([[maybe_unused]] OperationEnvironment &env)
{
  nmea_enable = true;
  return true;
}

bool
XVCDevice::ParseNMEA(const char *String, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(String))
    return false;
  NMEAInputLine line(String);
  const auto type = line.ReadView();
  if (type == "$PXCV"sv) {                // cyclic data from device useful for channel supervision
    xcvario_protocol_up = true;
    if (protocol_version != XCV_VERSION_UNKNOWN) {   // only parse NMEA once protocol version is set
      return PXCV(line, info);
    }
    return true;
  } else if (type == "!xcv"sv) {
    return XCV(line, info);
  }
  return false;
}

// For documentation refer to chapter 10.1.3 Device Driver/XCVario in mulilingual handbook: https://xcvario.de/handbuch
// Schedule sending of new protocol version 2; peer shall answer with the same message with its version
// Upon reception of XCV's version, the protocol will be switched, atfer timeout we assume legacy version 1
void
XVCDevice::OnSysTicker()
{
  // repeat sending protocol version every second until timeout ( 5 seconds )
  if (protocol_negotiation_timer && nmea_enable && protocol_version == XCV_VERSION_UNKNOWN && xcvario_protocol_up) {
    protocol_negotiation_timer--;
    if (protocol_version == XCV_VERSION_UNKNOWN) { // send our version until negotiation is finished
      PutProtocolVersion(XCV_VERSION_2);
    }
  }
  if (protocol_negotiation_timer == 0 && protocol_version == XCV_VERSION_UNKNOWN) {  // timeout waiting for device sending version: legacy version 1
    // after timer expiry, we work in legacy version 1 mode
    protocol_version = XCV_VERSION_1;
  }
}

// For documentation refer to chapter 10.1.3 Device Driver/XCVario in multilingual handbook: https://xcvario.de/handbuch
bool
XVCDevice::PutQNH(const AtmosphericPressure &pres, OperationEnvironment &env)
{
  /* the XCVario understands "!g,q<NNNN>" command for QNH updates */
  char buffer[32];
  unsigned qnh = uround(pres.GetHectoPascal());
  int msg_len = sprintf(buffer,"!g,q%u\r", std::min(qnh,(unsigned)2000));
  port.FullWrite(std::as_bytes(std::span{buffer}.first(msg_len)), env, std::chrono::seconds(2));
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
XVCDevice::PutBallast(double fraction, [[maybe_unused]] double overload,
                      OperationEnvironment &env)
{
  /* the XCVario understands CAI302 like command for ballast "!g,b" with
     float precision */
  char buffer[32];
  double ballast = fraction * 10.;
  int msg_len = sprintf(buffer,"!g,b%.3f\r", ballast);
  port.FullWrite(std::as_bytes(std::span{buffer}.first(msg_len)), env, std::chrono::seconds(2));
  return true;
}

bool
XVCDevice::SendNMEAItem(const char *which, unsigned int value)
{
  /* the new xcs protocol for various int items e.g. with absolute weights or protocol version including the NMEA checksum
   * We are using Port::Write() as do no need reliable and maybe blocking transmission here, there is retry on application layer
   * */
  char line[32];
  sprintf(line,"xcs,%s,%u", which, value);
  port.Write('!');
  port.Write(line);
  char checksum[16];
  sprintf(checksum, "*%02X\r\n", NMEAChecksum(line));
  port.Write(checksum);
  return true;
}

bool XVCDevice::PutProtocolVersion(int version)
{
  return SendNMEAItem("version", version );
}

static Device *
XVCCreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &com_port)
{
  return new XVCDevice(com_port);
}

const struct DeviceRegister xcv_driver = {
  _T("XCVario"),
  _T("XCVario"),
  DeviceRegister::RECEIVE_SETTINGS | DeviceRegister::SEND_SETTINGS,
  XVCCreateOnPort,
};
