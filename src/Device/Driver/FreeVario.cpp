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
#include "LogFile.hpp"
#include "Interface.hpp"
#include "CalculationThread.hpp"
#include "Protection.hpp"
#include "Input/InputEvents.hpp"
#include <iostream>

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
 * $POV,C,STF*4B  -- if this command is received it is resend to NMEA Device 1 and Device 2 to sent variod to speed to fly audio mode
 * $POV,C,VAR*4F  -- if this command is received it is resend to NMEA Device 1 and Device 2 to set variod to vario audio mode
 *  
 */

class FreeVarioDevice : public AbstractDevice {
    Port &port;

public:
  explicit FreeVarioDevice(Port &_port):port(_port){}
  bool ParseNMEA(const char *line,NMEAInfo &info) override;
  static bool PFVParser(NMEAInputLine &line, NMEAInfo &info, Port &port);
  static bool PFVParserAndForward(NMEAInputLine &line, NMEAInfo &info, Port &port);
  bool PutMacCready(double mc, OperationEnvironment &env) override;
  bool PutBugs(double bugs, OperationEnvironment &env) override;
  bool PutQNH(const AtmosphericPressure &pres,OperationEnvironment &env) override;
  void OnCalculatedUpdate(const MoreData &basic, const DerivedInfo &calculated) override;
  void OnSensorUpdate(const MoreData &basic) override;
};

/**
 * Parse NMEA messsage and check if it is a valid FreeVario message
 * Is true when a valid message or false if no valid message
 */
bool
FreeVarioDevice::PFVParserAndForward(NMEAInputLine &line, NMEAInfo &info, Port &port)
{
  NullOperationEnvironment env;
  bool messageValid = false;

  while (!line.IsEmpty() ) {

     char command = line.ReadOneChar();

      char buff[4] ;
      line.Read(buff,4);
      StaticString<4> bufferAsString(buff);

     if ( 'C' == command && strcmp("STF",bufferAsString) == 0){
       messageValid = true;
       InputEvents::eventSendNMEAPort1("POV,C,STF*4B");
       InputEvents::eventSendNMEAPort2("POV,C,STF*4B");
       InputEvents::eventStatusMessage("Speed to Fly Mode");
     }
     if ('C' == command && strcmp("VAR",bufferAsString) == 0){
       messageValid = true;
       InputEvents::eventSendNMEAPort1("POV,C,VAR*4F");
       InputEvents::eventSendNMEAPort2("POV,C,VAR*4F");
       InputEvents::eventStatusMessage("Vario Mode");
     }
  }
  return messageValid;
}

/**
 * Parse NMEA messsage and check if it is a valid FreeVario message
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

    if (type == '\0')break;
    if (subCommand == '\0')
      break;
    
    switch (type) {
             case 'M': {
               if (subCommand == 'S'){

                   double mcIn;
                   bool mcReadOk = line.ReadChecked(mcIn);

                   if (subCommand == 'S' && mcReadOk){
                     info.settings.ProvideMacCready(mcIn,info.clock);
                     validMessage = true;
                   }
               }

               else if (subCommand == 'U'){

                   double mc = info.settings.mac_cready;
                   double newmc = mc + 0.1;
                   // Check for upper range
                   if (newmc > 5.0){newmc = 5.0;}
                   info.settings.ProvideMacCready(newmc,info.clock);
                   validMessage = true;
               }

               else if (subCommand == 'D'){

                   double  mc = info.settings.mac_cready;
                   double newmc = mc - 0.1;
                   // Check for lower Range
                   if (newmc <= 0){newmc = 0.0;}
                   info.settings.ProvideMacCready(newmc,info.clock);
                   validMessage = true;
               }
               break;
             }
             case 'B': {
                   if (subCommand == 'S'){
                        double bugsIn;
                        bool bugsOK = line.ReadChecked(bugsIn);

                        if (subCommand == 'S' && bugsOK){
                            info.settings.ProvideBugs((100 -bugsIn )/100.,info.clock);
                            validMessage = true;
                        }
                   }
                   break;
               }

             case 'Q': {
                          if (subCommand == 'S'){
                          double qnhIn;
                          bool qnhOK = line.ReadChecked(qnhIn);

                           if (subCommand == 'S' && qnhOK){
                             AtmosphericPressure pres = info.static_pressure.HectoPascal(qnhIn);
                             info.settings.ProvideQNH(pres, info.clock);
                             validMessage = true;
                            }
                         }
                         break;
            }

             case 'F': {
               if (subCommand == 'S'){
                   info.switch_state.flight_mode = SwitchState::FlightMode::CRUISE;
                   validMessage = true;
               }
               else if (subCommand == 'C'){
                 info.switch_state.flight_mode = SwitchState::FlightMode::CIRCLING;
                 validMessage = true;
               }
               break;
             }

             case 'S':
             {
                if (subCommand == 'S'){
                      char nmeaOutbuffer[80];
                      int soundState;
                      bool stateOK = line.ReadChecked(soundState);
                      if (stateOK)
                      sprintf(nmeaOutbuffer,"PFV,MUT,%d", soundState);
                      PortWriteNMEA(port, nmeaOutbuffer, env);
                      validMessage = true;
                 }
                  break;
             }

             case 'A':
             {
                if (subCommand == 'S'){
                      char nmeaOutbuffer[80];
                      int attenState;
                      bool stateOK = line.ReadChecked(attenState);
                      if (stateOK)
                      sprintf(nmeaOutbuffer,"PFV,ATT,%d", attenState);
                      PortWriteNMEA(port, nmeaOutbuffer, env);
                      validMessage = true;
                 }
                  break;
             }
           default:
           {
             // Just break on default
             break;
           }
       }
        
  }

  return validMessage;
 }

/**
 * Parse incomming NMEA messages to check for PFV messages
 */
bool
FreeVarioDevice::ParseNMEA(const char *_line, NMEAInfo &info)
{
 NullOperationEnvironment env;

  if ( VerifyNMEAChecksum(_line) ){
      // August2111: is this correct?
	  NMEAInputLine lineTestPfv(_line);
      NMEAInputLine lineTestPov(_line);
      if ( lineTestPfv.ReadCompare("$PFV") ){
            return PFVParser(lineTestPfv, info, port);
      }
      else if ( lineTestPov.ReadCompare("$POV") ){
            return PFVParserAndForward(lineTestPov, info, port);
      }
      else  {
        return false;
        }
   } else {
     return false;
   }

}

/*
 * Send total_energy_vario to FreeVario device on every sensor update.
 * Is needed to have a good refresh rate on the external device showing the
 * vario values
 */
void
FreeVarioDevice::OnSensorUpdate(const MoreData &basic){

   NullOperationEnvironment env;
   char nmeaOutbuffer[80];

   if (basic.total_energy_vario_available.IsValid()){
     sprintf(nmeaOutbuffer,"PFV,VAR,%f", basic.total_energy_vario);
     PortWriteNMEA(port, nmeaOutbuffer, env);
   }

}


/*
 * Always send the calculated updated values to the FreeVario to have a good refresh rate
 * on the external device
 */
void
FreeVarioDevice::OnCalculatedUpdate(const MoreData &basic,const DerivedInfo &calculated)
{

  NullOperationEnvironment env;

  char nmeaOutbuffer[80];

    if (basic.baro_altitude_available.IsValid()){
      sprintf(nmeaOutbuffer,"PFV,HIG,%f", basic.baro_altitude);
      PortWriteNMEA(port, nmeaOutbuffer, env);
    }
    else if (basic.gps_altitude_available.IsValid()){
      sprintf(nmeaOutbuffer,"PFV,HIG,%f", basic.gps_altitude);
      PortWriteNMEA(port, nmeaOutbuffer, env);
    }
    else {
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
FreeVarioDevice::PutMacCready(double mc, OperationEnvironment &env)
{
  if (!EnableNMEA(env)){return false;}
  char nmeaOutbuffer[80];
  sprintf(nmeaOutbuffer,"PFV,MCI,%0.2f", (double)mc);
  return PortWriteNMEA(port, nmeaOutbuffer, env);
}

bool
FreeVarioDevice::PutBugs(double bugs,OperationEnvironment &env){
  if (!EnableNMEA(env)){return false;}
     char nmeaOutbuffer[80];
     double bugsAsPercentage = (1 - bugs) * 100;
     sprintf(nmeaOutbuffer,"PFV,BUG,%f",bugsAsPercentage);
     return PortWriteNMEA(port, nmeaOutbuffer, env);
}

bool
FreeVarioDevice::PutQNH(const AtmosphericPressure &pres,
  OperationEnvironment &env)
{
  if (!EnableNMEA(env)){return false;}
      char nmeaOutbuffer[80];
      sprintf(nmeaOutbuffer,"PFV,QNH,%f",pres.GetHectoPascal());
      return PortWriteNMEA(port, nmeaOutbuffer, env);
}


static Device *
FreeVarioCreateOnPort(const DeviceConfig &config, Port &com_port)
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
