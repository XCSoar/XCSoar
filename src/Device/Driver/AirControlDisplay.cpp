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

static bool
ParsePAAVS(NMEAInputLine &line, NMEAInfo &info)
{
  double value;

  char type[16];
  line.Read(type, 16);

  if (StringIsEqual(type, "ALT")) {
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
      info.settings.ProvideQNH(qnh, info.clock);
    }
  } else if (StringIsEqual(type, "COM")) {
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
    RadioFrequency freq;

    if (line.ReadChecked(value)) {
      freq.SetKiloHertz(value);
      info.settings.active_frequency = freq;
    }

    if (line.ReadChecked(value)) {
      freq.SetKiloHertz(value);
      info.settings.standby_frequency = freq;
    }

    if (line.ReadChecked(value))
      info.settings.volume = value;
  } else {
    // ignore responses from XPDR
    return false;
  }

  return true;
}

class ACDDevice : public AbstractDevice {
  Port &port;

public:
  ACDDevice(Port &_port):port(_port) {}

  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
  bool PutQNH(const AtmosphericPressure &pres,
              OperationEnvironment &env) override;
  bool PutVolume(unsigned volume, OperationEnvironment &env) override;
  bool PutStandbyFrequency(RadioFrequency frequency,
                           const TCHAR *name,
                           OperationEnvironment &env) override;
};

bool
ACDDevice::PutQNH(const AtmosphericPressure &pres, OperationEnvironment &env)
{
  char buffer[100];
  unsigned qnh = uround(pres.GetPascal());
  sprintf(buffer, "PAAVC,S,ALT,QNH,%u", qnh);
  return PortWriteNMEA(port, buffer, env);
}

bool
ACDDevice::PutVolume(unsigned volume, OperationEnvironment &env)
{
  char buffer[100];
  sprintf(buffer, "PAAVC,S,COM,RXVOL1,%u", volume);
  return PortWriteNMEA(port, buffer, env);
}

bool
ACDDevice::PutStandbyFrequency(RadioFrequency frequency,
                                   const TCHAR *name,
                                   OperationEnvironment &env)
{
  char buffer[100];
  unsigned freq = frequency.GetKiloHertz();
  sprintf(buffer, "PAAVC,S,COM,CHN2,%u", freq);
  return PortWriteNMEA(port, buffer, env);
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
AirControlDisplayCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new ACDDevice(com_port);
}

const struct DeviceRegister acd_driver = {
  _T("ACD"),
  _T("Air Control Display"),
  DeviceRegister::RECEIVE_SETTINGS | DeviceRegister::SEND_SETTINGS,
  AirControlDisplayCreateOnPort,
};
