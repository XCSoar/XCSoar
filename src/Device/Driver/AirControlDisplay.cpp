/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#include "Device/Driver/AirControlDisplay.hpp"
#include "Device/Driver.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"
#include "Atmosphere/Pressure.hpp"
#include "RadioFrequency.hpp"
#include "Units/System.hpp"
#include "Math/Util.hpp"
#include "Operation/Operation.hpp"

using std::string_view_literals::operator""sv;

class ACDDevice : public AbstractDevice {
  Port &port;
  static bool syncInSync;
  static double lastReceivedPascalPresure;
  static double lastSendPascalPresure;

public:
  ACDDevice(Port &_port):port(_port) { syncInSync = false; lastReceivedPascalPresure = 0; lastSendPascalPresure = 0;}

  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
  bool PutQNH(const AtmosphericPressure &pres,OperationEnvironment &env) override;
  bool PutVolume(unsigned volume, OperationEnvironment &env) override;
  bool PutStandbyFrequency(RadioFrequency frequency,
                           const TCHAR *name,
                           OperationEnvironment &env) override;
  static bool ParsePAAVS(NMEAInputLine &line, NMEAInfo &info);
private:
  static void setDeviceInSync(bool isSynchon);
  static bool getDeviceInSyncStatus();
  static void setLastReceivedPascalPresure(double pascal);
  static double getLastReceivedPascalPresure();
  static void setLastSendPascalPresure(double pascal);
  static double getLastSendPascalPresure();
};

bool ACDDevice::syncInSync = false;
double ACDDevice::lastReceivedPascalPresure = 0;
double ACDDevice::lastSendPascalPresure = 0;

void
ACDDevice::setDeviceInSync(bool sync){
    syncInSync = sync;
}

bool
ACDDevice::getDeviceInSyncStatus(){
    return syncInSync;
}

void 
ACDDevice::setLastReceivedPascalPresure(double pascal){
  lastReceivedPascalPresure = pascal;
}

double
ACDDevice::getLastReceivedPascalPresure(){
 return lastReceivedPascalPresure;
}

void 
ACDDevice::setLastSendPascalPresure(double pascal){
  lastSendPascalPresure = pascal;
}

double 
ACDDevice::getLastSendPascalPresure(){
  return lastSendPascalPresure;
}

bool
ACDDevice::ParsePAAVS(NMEAInputLine &line, NMEAInfo &info)
{
  double value;
  bool result = false;
  NullOperationEnvironment env;

  const auto type = line.ReadView();

  if (type == "ALT"sv) {
    /*
    $PAAVS,ALT,<ALTQNE>,<ALTQNH>,<QNH>
     <ALTQNE> Current QNE altitude in meters with two decimal places
     <ALTQNH> Current QNH altitude in meters with two decimal places
     <QNH> Current QNH setting in pascal (unsigned integer (e.g. 101325))
    */
    if (line.ReadChecked(value))
      info.ProvidePressureAltitude(value);

    if (line.ReadChecked(value))
      info.ProvideBaroAltitudeTrue(value);

    if (line.ReadChecked(value)) {
      auto qnh = AtmosphericPressure::Pascal(value);

      // If an external QNH is available on the xcsoar settings, send this value
      // to the AirControl device before setting the new value received. So the old 
      // value is synced to the AirControl display
      char buffer[100];

      // First device contact with qnh value. Sync with device.

      if (getLastReceivedPascalPresure() == 0){

        // external qnh available in xcsoar, sync to device
         if (info.settings.qnh_available.IsValid()) {
              unsigned qnhAsInt = uround(info.settings.qnh.GetPascal());
              sprintf(buffer, "PAAVC,S,ALT,QNH,%u", qnhAsInt);
              setLastReceivedPascalPresure(qnh.GetPascal());
              setLastSendPascalPresure(info.settings.qnh.GetPascal());
              return true;
          }
          // NO external qnh available in xcsoar, sync device value to xcsoar
          else {
              info.settings.ProvideQNH(qnh,info.clock);
              setLastReceivedPascalPresure(info.settings.qnh.GetPascal());
              setLastSendPascalPresure(info.settings.qnh.GetPascal());
              return true;
          }
      }

      else if (getLastReceivedPascalPresure() != qnh.GetPascal() && getDeviceInSyncStatus() == false ){
        // Reset for new sync run
        setLastReceivedPascalPresure(0);
        return true;
      }

     else if (getLastReceivedPascalPresure() == qnh.GetPascal() && getDeviceInSyncStatus() == false ){
         setDeviceInSync(true);
         return true;
    }

      else if (getLastReceivedPascalPresure() == qnh.GetPascal() && getDeviceInSyncStatus() == true ){

          if (info.settings.qnh.GetPascal() != qnh.GetPascal()){
            unsigned qnhAsInt = uround(info.settings.qnh.GetPascal());
            sprintf(buffer, "PAAVC,S,ALT,QNH,%u", qnhAsInt);
            setLastReceivedPascalPresure(qnh.GetPascal());
            setLastSendPascalPresure(info.settings.qnh.GetPascal());
            setDeviceInSync(false);
          }
         return true;
      }

      else if (getLastReceivedPascalPresure() != qnh.GetPascal() && getDeviceInSyncStatus() == true ){
          info.settings.ProvideQNH(qnh,info.clock);
          setLastReceivedPascalPresure(info.settings.qnh.GetPascal());
          setLastSendPascalPresure(info.settings.qnh.GetPascal());
          return true;
      }
      else{
        return true;
      }

    }
  } else if (type == "COM"sv) {
    /*
    $PAAVS,COM,<CHN1>,<CHN2>,<RXVOL1>,<RXVOL2>,<DWATCH>,<RX1>,<RX2>,<TX1>
     <CHN1> Primary radio channel;
            25kHz frequencies and 8.33kHz channels as unsigned integer
            values between 118000 and 136990
     <CHN2> Secondary radio channel;
            25kHz frequencies and 8.33kHz channels as unsigned integer
            values between 118000 and 136990
     <RXVOL1> Primary radio channel volume (Unsigned integer values, 0–100)
     <RXVOL2> Secondary radio channel volume (Unsigned integer values, 0–100)
     <DWATCH> Dual watch mode (0 = off; 1 = on)
     <RX1> Primary channel rx state (0 = no signal rec; 1 = signal rec)
     <RX2> Secondary channel rx state (0 = no signal rec; 1 = signal rec)
     <TX1> Transmit active (0 = no transmission; 1 = transmitting signal)
     */

    if (line.ReadChecked(value)) {
      info.settings.has_active_frequency.Update(info.clock);
      info.settings.active_frequency = RadioFrequency::FromKiloHertz(value);
      result = true;
    }

    if (line.ReadChecked(value)) {
      info.settings.has_standby_frequency.Update(info.clock);
      info.settings.standby_frequency = RadioFrequency::FromKiloHertz(value);
      result = true;
    }

    unsigned volume;
    if (line.ReadChecked(volume)){
      info.settings.ProvideVolume(volume, info.clock);
      result = true;
    }

  }
  return result; 
}

bool
ACDDevice::PutQNH(const AtmosphericPressure &pres, OperationEnvironment &env)
{
  char buffer[100];
  unsigned qnh = uround(pres.GetPascal());
  sprintf(buffer, "PAAVC,S,ALT,QNH,%u", qnh);
  setLastSendPascalPresure(pres.GetPascal());
  setDeviceInSync(false);
  PortWriteNMEA(port, buffer, env);
  return true;
}

bool
ACDDevice::PutVolume(unsigned volume, OperationEnvironment &env)
{
  char buffer[100];
  sprintf(buffer, "PAAVC,S,COM,RXVOL1,%u", volume);
  PortWriteNMEA(port, buffer, env);
  return true;
}

bool
ACDDevice::PutStandbyFrequency(RadioFrequency frequency,
                               [[maybe_unused]] const TCHAR *name,
                               OperationEnvironment &env)
{
  char buffer[100];
  unsigned freq = frequency.GetKiloHertz();
  sprintf(buffer, "PAAVC,S,COM,CHN2,%u", freq);
  PortWriteNMEA(port, buffer, env);
  return true;
}

bool
ACDDevice::ParseNMEA(const char *_line, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(_line))
    return false;

  NMEAInputLine line(_line);

  if (line.ReadCompare("$PAAVS"))
    return ParsePAAVS(line, info);
  else
    return false;
}

static Device *
AirControlDisplayCreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &com_port)
{
  return new ACDDevice(com_port);
}

const struct DeviceRegister acd_driver = {
  _T("ACD"),
  _T("Air Control Display"),
  DeviceRegister::RECEIVE_SETTINGS | DeviceRegister::SEND_SETTINGS,
  AirControlDisplayCreateOnPort,
};
