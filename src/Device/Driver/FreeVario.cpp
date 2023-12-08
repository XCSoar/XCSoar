#include "Device/Config.hpp"
#include "Device/Driver/FreeVario.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "Message.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/InputLine.hpp"
#include "Units/System.hpp"
#include "Operation/Operation.hpp"
// #include "Operation/PopupOperationEnvironment.hpp"
#include "LogFile.hpp"
#include "Interface.hpp"
#include "CalculationThread.hpp"
#include "Protection.hpp"
#include "Input/InputEvents.hpp"
#include <iostream>

using std::string_view_literals::operator""sv;

/*
 * Commands via NMEA from FreeVario Device to XCSoar:
 *
 * $PFV,M,S,<double value>  - set MC External to defined value
 * $PFV,M,U                 - set MC External up by 0.1
 * $PFV,M,D                 - set MC External down by 0.1
 *
 * $PFV,F,C                 - set Vario FlightMode to Circle
 * $PFV,F,S                 - set Vario FlightMode to SpeedToFly
 *
 * $PFV,Q,S,<double value>  - set QNH to defined value
 * $PFV,B,S,<double value>  - set Bugs to percent by capacity 0 - 100
 * $PFV,S,S,<integer value> - set Mute status and gives it back to sound board
 * $PFV,A,S,<integer value> - set attenuation and gives it back to sound board
 *
 * Commands for NMEA compatibility with OpenVario especially variod
 * 
 * $POV,C,STF*4B  -- if this command is received it is resend to NMEA
 *                   Device 1 and Device 2 to sent variod to speed to fly
 *                   audio mode
 * $POV,C,VAR*4F  -- if this command is received it is resend to NMEA
 *                   Device 1 and Device 2 to set variod to vario audio mode
 *  
 */

class FreeVarioDevice : public AbstractDevice {
    Port &port;

public:
  explicit FreeVarioDevice(Port &_port):port(_port){}
  bool ParseNMEA(const char *line,NMEAInfo &info) override;
  static bool PFVParser(NMEAInputLine &line, NMEAInfo &info, Port &port);
  bool POVParserAndForward(NMEAInputLine &line);
  bool SendCmd(double value, const char *cmd, OperationEnvironment &env);
  bool PutMacCready(double mc, OperationEnvironment &env) override;
  bool PutBugs(double bugs, OperationEnvironment &env) override;
  bool PutQNH(const AtmosphericPressure &pres,
              OperationEnvironment &env) override;
  void OnCalculatedUpdate(const MoreData &basic,
              const DerivedInfo &calculated) override;
  void OnSensorUpdate(const MoreData &basic) override;
};

/**
 * Parse NMEA messsage and check if it is a valid FreeVario message
 * Is true when a valid message or false if no valid message
 */
bool
FreeVarioDevice::POVParserAndForward(NMEAInputLine &line)
{
  NullOperationEnvironment env;
  // PopupOperationEnvironment env;
  bool messageValid = false;

  while (!line.IsEmpty() ) {
    char command = line.ReadOneChar();
    char buff[4] ;

    line.Read(buff,4);
    // TODO(August2111): I don't know, why this command should be
    // mirrored in the driver to the eventSendNMEAPort(1) and (2)
    // what is the sense for this
    // From FreeVario device is coming a '$POV,C,XXX command

    if (command == 'C') {
      char buffer[64];
      sprintf(buffer, "POV,C,%s", buff);
      PortWriteNMEA(port, buffer, env);
    }
#ifdef FREEVARIO_EVENT
    // TODO(August2111): in the moment this output is not allowed
    // with build-native it shows ' error: undefined reference to
    // 'InputEvents::eventXXX(char const*)', but I don't know why yet
    // info.switch_state.flight_mode = SwitchState::FlightMode::CIRCLING;
    StaticString<4> bufferAsString;
    bufferAsString.SetUTF8(buff);
    if ('C' == command && bufferAsString == _T("STF")) {
      messageValid = true;
      InputEvents::eventSendNMEAPort1(_T("POV,C,STF*4B"));
      InputEvents::eventSendNMEAPort2(_T("POV,C,STF*4B"));
      InputEvents::eventStatusMessage(_T("Speed to Fly Mode"));
    }
    if ('C' == command && bufferAsString == _T("VAR")) {
      messageValid = true;
      InputEvents::eventSendNMEAPort1(_T("POV,C,VAR*4F"));
      InputEvents::eventSendNMEAPort2(_T("POV,C,VAR*4F"));
      InputEvents::eventStatusMessage(_T("Vario Mode"));
    }
#endif
  }
  return messageValid;
}

/**
 * Parse NMEA message and check if it is a valid FreeVario message
 * Is true when a valid message or false if no valid message
 */
bool
FreeVarioDevice::PFVParser(NMEAInputLine &line, NMEAInfo &info, Port &port)
{
  NullOperationEnvironment env;
  bool validMessage = false;

  while (!line.IsEmpty()) {

    char type = line.ReadOneChar();
    char subCommand = line.ReadOneChar();

    if (subCommand == '\0')
      return validMessage;  // from now the string is invalid...

    switch (type) {
    case '\0':  // from now the string is invalid or finished
      return validMessage;
    case 'M': {
      if (subCommand == 'S') {
        double mcIn;
        if (line.ReadChecked(mcIn)) {
          info.settings.ProvideMacCready(mcIn, info.clock);
          validMessage = true;
        }
      }

      else if (subCommand == 'U') {
        double new_mc = std::min(info.settings.mac_cready + 0.1, 5.0);
        info.settings.ProvideMacCready(new_mc, info.clock);
        validMessage = true;
      }

      else if (subCommand == 'D') {
        double new_mc = std::max(info.settings.mac_cready - 0.1, 0.0);
        info.settings.ProvideMacCready(new_mc, info.clock);
        validMessage = true;
      }
      break;
    }
    case 'B': {
      if (subCommand == 'S') {
        double bugsIn;
        if (line.ReadChecked(bugsIn)) {
          info.settings.ProvideBugs((100 - bugsIn) / 100., info.clock);
          validMessage = true;
        }
      }
      break;
    }

    case 'Q': {
      if (subCommand == 'S') {
        double qnhIn;
        if (line.ReadChecked(qnhIn)) {
          AtmosphericPressure pres = info.static_pressure.HectoPascal(qnhIn);
          info.settings.ProvideQNH(pres, info.clock);
          validMessage = true;
        }
      }
      break;
    }

    case 'F': {
      if (subCommand == 'S') {
        info.switch_state.flight_mode = SwitchState::FlightMode::CRUISE;
        validMessage = true;
      } else if (subCommand == 'C') {
        info.switch_state.flight_mode = SwitchState::FlightMode::CIRCLING;
        validMessage = true;
      }
      break;
    }

    case 'S': {
      if (subCommand == 'S') {
        char nmeaOutbuffer[80];
        int soundState;
        bool stateOK = line.ReadChecked(soundState);
        if (stateOK)
          sprintf(nmeaOutbuffer, "PFV,MUT,%d", soundState);
        PortWriteNMEA(port, nmeaOutbuffer, env);
        validMessage = true;
      }
      break;
    }

    case 'A': {
      if (subCommand == 'S') {
        char nmeaOutbuffer[80];
        int attenState;
        bool stateOK = line.ReadChecked(attenState);
        if (stateOK)
          sprintf(nmeaOutbuffer, "PFV,ATT,%d", attenState);
        PortWriteNMEA(port, nmeaOutbuffer, env);
        validMessage = true;
      }
      break;
    }
    default:
      // Just break on default
      break;
    }
  }

  return validMessage;
 }

/**
 * Parse incoming NMEA messages to check for PFV messages
 */
bool
FreeVarioDevice::ParseNMEA(const char *_line, NMEAInfo &info)
{
  NullOperationEnvironment env;

  if (VerifyNMEAChecksum(_line)) {
    NMEAInputLine line(_line);
    const auto type = line.ReadView();
    if (type == "$PFV"sv) {
        return PFVParser(line, info, port);
    } else if (type == "$POV"sv) {
        return POVParserAndForward(line);  // no info and port necessary
    }
  }
  return false;
}

/*
 * Send total_energy_vario to FreeVario device on every sensor update.
 * Is needed to have a good refresh rate on the external device showing the
 * vario values
 */
void
FreeVarioDevice::OnSensorUpdate(const MoreData &basic)
{
   NullOperationEnvironment env;
   char nmeaOutbuffer[80];

   if (basic.total_energy_vario_available.IsValid()) {
     sprintf(nmeaOutbuffer,"PFV,VAR,%f", basic.total_energy_vario);
     PortWriteNMEA(port, nmeaOutbuffer, env);
   }

   // TODO(August2111): basic.netto_variable has no timestamp unfortunately?
   // if (basic.netto_vario_available.IsValid()) {
     sprintf(nmeaOutbuffer,"PFV,VAN,%f", basic.netto_vario);
     PortWriteNMEA(port, nmeaOutbuffer, env);
   // }
}


/*
 * Always send the calculated updated values to the FreeVario to have a good
 * refresh rate on the external device
 */
void
FreeVarioDevice::OnCalculatedUpdate(const MoreData &basic,
  const DerivedInfo &calculated)
{
  NullOperationEnvironment env;

  char nmeaOutbuffer[80];

 if (basic.baro_altitude_available.IsValid()){
   sprintf(nmeaOutbuffer,"PFV,HIG,%f", basic.baro_altitude);
   PortWriteNMEA(port, nmeaOutbuffer, env);
 } else if (basic.gps_altitude_available.IsValid()){
   sprintf(nmeaOutbuffer,"PFV,HIG,%f", basic.gps_altitude);
   PortWriteNMEA(port, nmeaOutbuffer, env);
 } else {
   sprintf(nmeaOutbuffer,"PFV,HIG,%f", 0.0);
   PortWriteNMEA(port, nmeaOutbuffer, env);
 }

 if (calculated.altitude_agl_valid){
   sprintf(nmeaOutbuffer,"PFV,HAG,%f", calculated.altitude_agl);
   PortWriteNMEA(port, nmeaOutbuffer, env);
 }

  bool tempAvil = basic.temperature_available;
  if (tempAvil){
    double temp = basic.temperature.ToCelsius();
    sprintf(nmeaOutbuffer,"PFV,TEM,%f", temp);
    PortWriteNMEA(port, nmeaOutbuffer, env);
  }

  double trueAirspeedKmh = ((basic.true_airspeed * 60 * 60) / 1000);
  sprintf(nmeaOutbuffer,"PFV,TAS,%f", trueAirspeedKmh);
  PortWriteNMEA(port, nmeaOutbuffer, env);

  if (basic.ground_speed_available.IsValid()){
    double groundSpeed = ((basic.ground_speed * 60 * 60) / 1000);;
    sprintf(nmeaOutbuffer,"PFV,GRS,%f", groundSpeed);
    PortWriteNMEA(port, nmeaOutbuffer, env);
  } else {
    sprintf(nmeaOutbuffer,"PFV,GRS,%d", -1);
    PortWriteNMEA(port, nmeaOutbuffer, env);
  }

  double stfKmh = ((calculated.V_stf * 60 * 60) / 1000);
  sprintf(nmeaOutbuffer,"PFV,STF,%f", stfKmh);
  PortWriteNMEA(port, nmeaOutbuffer, env);

  if (calculated.circling){
    sprintf(nmeaOutbuffer,"PFV,MOD,%s", "C");
    PortWriteNMEA(port, nmeaOutbuffer, env);
  } else {
    sprintf(nmeaOutbuffer,"PFV,MOD,%s", "S");
    PortWriteNMEA(port, nmeaOutbuffer, env);
  }

  // vario average last 30 secs
  sprintf(nmeaOutbuffer,"PFV,VAA,%f",calculated.average);
  PortWriteNMEA(port, nmeaOutbuffer, env);

  if (basic.settings.mac_cready_available.IsValid()){
        double externalMC = basic.settings.mac_cready;
        sprintf(nmeaOutbuffer,"PFV,MCE,%0.2f", (double)externalMC);
        PortWriteNMEA(port, nmeaOutbuffer, env);
  }

  if (basic.settings.qnh_available.IsValid()){
    double qnhHp = basic.settings.qnh.GetHectoPascal();
    sprintf(nmeaOutbuffer,"PFV,QNH,%f",qnhHp);
    PortWriteNMEA(port, nmeaOutbuffer, env);
  }
}

/*
 *  Send the internal xcsoar mc value to FreeVario device to
 *  be informed about MC changes doen in XCSoar
 */
bool
FreeVarioDevice::SendCmd(double value, const char *cmd,
  OperationEnvironment &env)
{
  if (!EnableNMEA(env)) {
    return false;
  }
  char nmeaOutbuffer[80];
  sprintf(nmeaOutbuffer, cmd, value);
  PortWriteNMEA(port, nmeaOutbuffer, env);
  return true;
}

bool
FreeVarioDevice::PutMacCready(double mc, OperationEnvironment &env)
{
  return SendCmd(mc, "PFV,MCI,%0.2f", env);
}

bool
FreeVarioDevice::PutBugs(double bugs,OperationEnvironment &env)
{
  double bugsAsPercentage = (1 - bugs) * 100;
  return SendCmd(bugsAsPercentage, "PFV,BUG,%f", env);
}

bool
FreeVarioDevice::PutQNH(const AtmosphericPressure &pres,
  OperationEnvironment &env)
{
  return SendCmd(pres.GetHectoPascal(), "PFV,QNH,%f", env);
}


static Device *
FreeVarioCreateOnPort([[maybe_unused]] const DeviceConfig &config,
  Port &com_port)
{
  return new FreeVarioDevice(com_port);
}

const struct DeviceRegister free_vario_driver = {
  _T("FreeVario"),
  _T("FreeVario"),
  DeviceRegister::SEND_SETTINGS|
  DeviceRegister::RECEIVE_SETTINGS,
  FreeVarioCreateOnPort,
};
