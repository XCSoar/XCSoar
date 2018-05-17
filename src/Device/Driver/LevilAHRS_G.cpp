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

#include "Device/Driver/LevilAHRS_G.hpp"
#include "Message.hpp"
#include "Device/Driver.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "Units/Units.hpp"

static bool error_reported = false;

class LevilDevice : public AbstractDevice {
public:
  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
};

static void
ErrorMessage(gcc_unused unsigned code)
{
  Message::AddMessage(_T("Levil AHRS: hardware error !"));
}

static bool
ParseRPYL(NMEAInputLine &line, NMEAInfo &info)
{
  // $RPYL,Roll,Pitch,MagnHeading,SideSlip,YawRate,G,errorcode,

  int roll;
  if (!line.ReadChecked(roll)) return false;

  int pitch;
  if (!line.ReadChecked(pitch)) return false;

  int heading;
  if (!line.ReadChecked(heading)) return false;

  int sideslip;
  if (!line.ReadChecked(sideslip)) return false;

  int yawrate;
  if (!line.ReadChecked(yawrate)) return false;

  int G;
  if (!line.ReadChecked(G)) return false;

  int errorcode;
  if (!line.ReadChecked(errorcode)) return false;

  /* Error bits:
   *   0: Roll gyro test failed  
   *   1: Roll gyro test failed 
   *   2: Roll gyro test failed 
   *   3: Acc X test failed 
   *   4: Acc Y test failed 
   *   5: Acc Z test failed 
   *   6: Watchdog test failed
   *   7: Ram test failed
   *   8: EEPROM access test failed
   *   9: EEPROM checksum test failed
   *  10: Flash checksum test failed
   *  11: Low voltage error
   *  12: High temperature error (>60 C)
   *  13: Inconsistent roll data between gyro and acc.
   *  14: Inconsistent pitch data between gyro and acc.
   *  15: Inconsistent yaw data between gyro and acc.
   */
  if (errorcode && !error_reported) {
    ErrorMessage(errorcode);
    error_reported = true;
  }

  info.attitude.bank_angle_available.Update(info.clock);
  info.attitude.bank_angle = Angle::Degrees(roll / 10.);

  info.attitude.pitch_angle_available.Update(info.clock);
  info.attitude.pitch_angle = Angle::Degrees(pitch / 10.);

  info.attitude.heading_available.Update(info.clock);
  info.attitude.heading = Angle::Degrees(heading / 10.);

  info.acceleration.ProvideGLoad(G / 1000., true);

  return true;
}

static bool
ParseAPENV1(NMEAInputLine &line, NMEAInfo &info)
{
  // $APENV1,IAS,Altitude,0,0,0,VerticalSpeed,

  int ias;
  if (!line.ReadChecked(ias)) return false;

  int altitude;
  if (!line.ReadChecked(altitude)) return false;

  line.Skip();
  line.Skip();
  line.Skip();

  // In ft/min, quality of this is limited, do not use for the time being
  int vs;
  if (!line.ReadChecked(vs)) return false;

  auto sys_alt = Units::ToSysUnit(altitude, Unit::FEET);
  info.ProvidePressureAltitude(sys_alt);
  info.ProvideIndicatedAirspeedWithAltitude(Units::ToSysUnit(ias, Unit::KNOTS),
                                            sys_alt);

  return true;
}

bool
LevilDevice::ParseNMEA(const char *_line, NMEAInfo &info)
{
  NMEAInputLine line(_line);
  char type[16];
  line.Read(type, 16);

  if (error_reported) return false;

  if (StringIsEqual(type, "$RPYL"))
    return ParseRPYL(line, info);

  if (StringIsEqual(type, "$APENV1"))
    return ParseAPENV1(line, info);

  return false;
}

static Device *
LevilCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new LevilDevice();
}

const struct DeviceRegister levil_driver = {
  _T("Levil AHRS"),
  _T("Levil AHRS"),
  0,
  LevilCreateOnPort,
};
