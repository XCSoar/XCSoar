#include "Device/Driver/FreeVario.hpp"
#include "Device/Driver.hpp"
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

/*
 * Commands via NMEA from FreeVario Device to XCSoar:
 *
 * $PFV,B,S,<double value> - Set Bugs to percent by capacity 0 - 100
 *
 * $PFV,M,S,<double value> - Set MC External to defined value
 * $PFV,M,U                - Set MC External up by 0.1
 * $PFV,M,D                - Set MC External down by 0.1
 * $PFV,M,A                - Set MC to Auto [NOT WORKING !]
 *
 * $PFV,F,C                - Set Vario FlightMode to Circle
 * $PFV,F,S                - Set Vario FlightMode to SpeedToFly
 *
 * $PFV,Q,S,<double value> - Set QNH to defined value
 *
 */

class FreeVarioDevice : public AbstractDevice {
    Port &port;

public:
  FreeVarioDevice(Port &_port):port(_port) {}

  bool ParseNMEA(const char *line,NMEAInfo &info) override;
  static bool PFVParser(NMEAInputLine &line, NMEAInfo &info,Port &port);
  bool PutMacCready(double mc, OperationEnvironment &env) override;
  bool PutBugs(double bugs, OperationEnvironment &env) override;
  bool PutQNH(const AtmosphericPressure &pres,OperationEnvironment &env) override;
  void OnCalculatedUpdate(const MoreData &basic, const DerivedInfo &calculated) override;

};


bool
FreeVarioDevice::PFVParser(NMEAInputLine &line, NMEAInfo &info, Port &port)
{

  char buffer[80];
  NullOperationEnvironment env;

  while (!line.IsEmpty()) {


    char type = line.ReadOneChar();
    char subCommand = line.ReadOneChar();

    if (type == '\0')break;
    if (subCommand == '\0')break;

       switch (type) {
             case 'M': {
               if (subCommand == 'S'){

                   double mcIn;
                   bool mcReadOk = line.ReadChecked(mcIn);

                   if (subCommand == 'S' && mcReadOk){
                     info.settings.ProvideMacCready(mcIn,info.clock);
                   }
                   else {
                     sprintf(buffer,"PFV,ERR,%s","Error MC IN value");
                     PortWriteNMEA(port, buffer, env);
                   }
               }

               else if (subCommand == 'U'){

                   double mc = info.settings.mac_cready;
                   double newmc = mc + 0.1;
                   // Check for upper and lower range
                   if (newmc <= 0){newmc = 0.0;}
                   else if (newmc > 5.0){newmc = 5.0;}
                   info.settings.ProvideMacCready(newmc,info.clock);
               }

               else if (subCommand == 'D'){

                   double  mc = info.settings.mac_cready;
                   double newmc = mc - 0.1;
                   // Check for lower Range
                   if (newmc <= 0){newmc = 0.0;}
                     info.settings.ProvideMacCready(newmc,info.clock);
               }
               break;
             }
             case 'B': {
                   if (subCommand == 'S'){

                                double bugsIn;
                                bool bugsOK = line.ReadChecked(bugsIn);

                                if (subCommand == 'S' && bugsOK){
                                  sprintf(buffer,"PFV,OK,%s","OK BUG IN value");
                                  PortWriteNMEA(port, buffer, env);
                                  info.settings.ProvideBugs((100 -bugsIn )/100.,info.clock);
                                }
                                else {
                                  sprintf(buffer,"PFV,ERR,%s","Error BUG IN value");
                                  PortWriteNMEA(port, buffer, env);
                                }
                            }
                   break;
               }

             case 'Q': {
                          if (subCommand == 'S'){
                          double qnhIn;
                          bool qnhOK = line.ReadChecked(qnhIn);

                           if (subCommand == 'S' && qnhOK){
                             // Ballast is setting in percent by capacity
                             AtmosphericPressure pres = info.static_pressure.HectoPascal(qnhIn);
                             info.settings.ProvideQNH(pres, info.clock);
                            }
                             else {
                                sprintf(buffer,"PFV,ERR,%s","Error Weight IN value");
                                 PortWriteNMEA(port, buffer, env);
                              }
                           }
                                     break;
                            }

             case 'F': {
               if (subCommand == 'S'){
                   info.switch_state.flight_mode = SwitchState::FlightMode::CRUISE;
               }
               else if (subCommand == 'C'){
                 info.switch_state.flight_mode = SwitchState::FlightMode::CIRCLING;
               }
               break;
             }
           }
       }
  return true;
 }

bool
FreeVarioDevice::ParseNMEA(const char *_line, NMEAInfo &info)
{
 NullOperationEnvironment env;
 char buffer[80];

  if (!VerifyNMEAChecksum(_line)){
      sprintf(buffer,"PFV,ERR,%s%s", "Checksum:",_line);
      PortWriteNMEA(port, buffer, env);
     return false;
   } else  {
       NMEAInputLine line(_line);
       if (line.ReadCompare("$PFV")){
           return PFVParser(line, info, port);
       } else {
           sprintf(buffer,"PFV,ERR,%s", "Wrong Class");
           PortWriteNMEA(port, buffer, env);
           return false;
         }
   }

}



void
FreeVarioDevice::OnCalculatedUpdate(const MoreData &basic,const DerivedInfo &calculated)
{

  NullOperationEnvironment env;

  char buffer[80];

    if (basic.baro_altitude_available.IsValid()){
      sprintf(buffer,"PFV,HIG,%f", basic.baro_altitude);
      PortWriteNMEA(port, buffer, env);
    }
    else if (basic.gps_altitude_available.IsValid()){
      sprintf(buffer,"PFV,HIG,%f", basic.gps_altitude);
      PortWriteNMEA(port, buffer, env);
    }
    else {
      sprintf(buffer,"PFV,HIG,%f", 0.0);
      PortWriteNMEA(port, buffer, env);
    }

      if (calculated.altitude_agl_valid){
        sprintf(buffer,"PFV,HAG,%f", calculated.altitude_agl);
        PortWriteNMEA(port, buffer, env);
       }

      bool tempAvil = basic.temperature_available;
      if (tempAvil){
        double temp = basic.temperature.ToCelsius();
        sprintf(buffer,"PFV,TEM,%f", temp);
        PortWriteNMEA(port, buffer, env);
      }

     double trueAirspeedKmh = ((basic.true_airspeed * 60 * 60) / 1000);
     sprintf(buffer,"PFV,TAS,%f", trueAirspeedKmh);
     PortWriteNMEA(port, buffer, env);

     if (basic.ground_speed_available.IsValid()){
       double groundSpeed = ((basic.ground_speed * 60 * 60) / 1000);;
       sprintf(buffer,"PFV,GRS,%f", groundSpeed);
       PortWriteNMEA(port, buffer, env);
     } else {
       sprintf(buffer,"PFV,GRS,%d", -1);
       PortWriteNMEA(port, buffer, env);
     }

     double stfKmh = ((calculated.V_stf * 60 * 60) / 1000);
     sprintf(buffer,"PFV,STF,%f", stfKmh);
     PortWriteNMEA(port, buffer, env);

     if (calculated.circling){
       sprintf(buffer,"PFV,MOD,%s", "C");
       PortWriteNMEA(port, buffer, env);
     } else {
       sprintf(buffer,"PFV,MOD,%s", "S");
       PortWriteNMEA(port, buffer, env);
     }

     if (basic.brutto_vario_available.IsValid()){
            sprintf(buffer,"PFV,VAR,%f", basic.brutto_vario);
            PortWriteNMEA(port, buffer, env);
     }

     // vario average last 30 secs
     sprintf(buffer,"PFV,VAA,%f",calculated.average);
     PortWriteNMEA(port, buffer, env);

     if (basic.settings.mac_cready_available.IsValid()){
           double externalMC = basic.settings.mac_cready;
           sprintf(buffer,"PFV,MCE,%0.2f", (double)externalMC);
           PortWriteNMEA(port, buffer, env);
     }

}

bool
FreeVarioDevice::PutMacCready(double mc, OperationEnvironment &env)
{
  if (!EnableNMEA(env)){return false;}
  char buffer[80];
  sprintf(buffer,"PFV,MCI,%0.2f", (double)mc);
  return PortWriteNMEA(port, buffer, env);
}

bool
FreeVarioDevice::PutBugs(double bugs,OperationEnvironment &env){
  if (!EnableNMEA(env)){return false;}
     char buffer[80];
     double bugsAsPercentage = (1 - bugs) * 100;
     sprintf(buffer,"PFV,BUG,%f",bugsAsPercentage);
     return PortWriteNMEA(port, buffer, env);
}

bool
FreeVarioDevice::PutQNH(const AtmosphericPressure &pres,OperationEnvironment &env) {
  if (!EnableNMEA(env)){return false;}
      char buffer[80];
      sprintf(buffer,"PFV,QNH,%f",pres.GetHectoPascal());
      return PortWriteNMEA(port, buffer, env);
}


static Device *
FreeVarioCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new FreeVarioDevice(com_port);
}

const struct DeviceRegister free_vario_driver = {
  _T("FreeVario"),
  _T("FreeVario"),
  0,
  FreeVarioCreateOnPort,
};
