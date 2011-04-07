/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Device/Driver/LX.hpp"
#include "Device/Driver.hpp"
#include "Device/Parser.hpp"
#include "Device/Port.hpp"
#include "Units.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "Operation.hpp"

#include <stdlib.h>

static const unsigned int NUMTPS = 12;
enum LX_command {
  LX_PREFIX = 0x02,
  LX_ACK = 0x06,
  LX_SYN = 0x16,
  LX_WRITE_FLIGHT_INFO = 0xCA,
  LX_WRITE_CONTEST_CLASS = 0xD0,
};
#define LX_ACK_STRING "\x06"

class LXDevice: public AbstractDevice
{
  Port *port;
  BrokenDate DeclDate;

public:
  LXDevice(Port *_port)
    :port(_port),
     crc(0xff) {
    DeclDate.day = 1;
    DeclDate.month = 1;
    DeclDate.year = 2010;
  }

protected:
  bool StartCommandMode();
  void StartNMEAMode(OperationEnvironment &env);
  void LoadPilotInfo(const Declaration *decl);
  void WritePilotInfo();
  bool LoadTask(const Declaration *decl);
  void WriteTask();
  void LoadContestClass(const Declaration *decl);
  void WriteContestClass();
  void CRCWriteint32(int32_t i);
  void CRCWrite(const char *buff, unsigned size);
  void CRCWrite(char c);

  struct lxDevice_Pilot_t { //strings have extra byte for NULL
    char unknown1[3];
    char PilotName[19];
    char GliderType[12];
    char GliderID[8];
    char CompetitionID[4];
    char unknown2[73];
  } gcc_packed;

  struct lxDevice_Declaration_t { //strings have extra byte for NULL
    unsigned char unknown1[5];
    unsigned char dayinput;
    unsigned char monthinput;
    unsigned char yearinput;
    unsigned char dayuser;
    unsigned char monthuser;
    unsigned char yearuser;
    int16_t taskid;
    unsigned char numtps;
    unsigned char tptypes[12];
    int32_t Longitudes[12];
    int32_t Latitudes[12];
    char WaypointNames[12][9];
  } gcc_packed;


  struct lxDevice_ContestClass_t { //strings have extra byte for NULL
    char contest_class[9];
  } gcc_packed;

  lxDevice_Declaration_t lxDevice_Declaration;
  lxDevice_Pilot_t lxDevice_Pilot;
  lxDevice_ContestClass_t lxDevice_ContestClass;
  char crc;

  bool DeclareInner(const Declaration *declaration,
                    OperationEnvironment &env);

public:
  virtual bool ParseNMEA(const char *line, struct NMEA_INFO *info,
      bool enable_baro);
  virtual bool Declare(const Declaration *declaration,
                       OperationEnvironment &env);
};

static char
calc_crc_char(char d, char crc)
{
  char tmp;
  const char crcpoly = 0x69;
  int count;

  for (count = 8; --count >= 0; d <<= 1) {
    tmp = crc ^ d;
    crc <<= 1;
    if (tmp & 0x80)
      crc ^= crcpoly;
  }
  return crc;
}

static char
filser_calc_crc(const char *p0, size_t len, char crc)
{
  const char *p = p0;
  size_t i;

  for (i = 0; i < len; i++)
    crc = calc_crc_char(p[i], crc);

  return crc;
}

static bool
ReadSpeedVector(NMEAInputLine &line, SpeedVector &value_r)
{
  fixed bearing, norm;

  bool bearing_valid = line.read_checked(bearing);
  bool norm_valid = line.read_checked(norm);

  if (bearing_valid && norm_valid) {
    value_r.bearing = Angle::degrees(bearing);
    value_r.norm = Units::ToSysUnit(norm, unKiloMeterPerHour);
    return true;
  } else
    return false;
}

static bool
LXWP0(NMEAInputLine &line, NMEA_INFO *GPS_INFO, bool enable_baro)
{
  /*
  $LXWP0,Y,222.3,1665.5,1.71,,,,,,239,174,10.1

   0 loger_stored (Y/N)
   1 IAS (kph) ----> Condor uses TAS!
   2 baroaltitude (m)
   3-8 vario (m/s) (last 6 measurements in last second)
   9 heading of plane
  10 windcourse (deg)
  11 windspeed (kph)
  */

  fixed value;

  line.skip();

  fixed airspeed;
  bool tas_available = line.read_checked(airspeed);

  fixed alt = fixed_zero;
  if (line.read_checked(alt) && enable_baro)
    /* a dump on a LX7007 has confirmed that the LX sends uncorrected
       altitude above 1013.25hPa here */
    GPS_INFO->ProvidePressureAltitude(alt);

  if (tas_available)
    GPS_INFO->ProvideTrueAirspeedWithAltitude(Units::ToSysUnit(airspeed,
                                              unKiloMeterPerHour),
                                              alt);

  if (line.read_checked(value))
    GPS_INFO->ProvideTotalEnergyVario(value);

  line.skip(6);

  SpeedVector wind;
  if (ReadSpeedVector(line, wind))
    GPS_INFO->ProvideExternalWind(wind);

  return true;
}

static bool
LXWP1(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  /*
   * $LXWP1,
   * serial number,
   * instrument ID,
   * software version,
   * hardware version,
   * license string
   */
  (void)GPS_INFO;
  return true;
}

static bool
LXWP2(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  /*
   * $LXWP2,
   * maccready value, (m/s)
   * ballast, (1.0 - 1.5)
   * bugs, (0 - 100%)
   * polar_a,
   * polar_b,
   * polar_c,
   * audio volume
   */

  fixed value;
  // MacCready value
  if (line.read_checked(value))
    GPS_INFO->settings.ProvideMacCready(value, GPS_INFO->Time);

  // Ballast
  line.skip();
  /*
  if (line.read_checked(value))
    GPS_INFO->ProvideBallast(value, GPS_INFO->Time);
  */

  // Bugs
  if (line.read_checked(value))
    GPS_INFO->settings.ProvideBugs(fixed(100) - value, GPS_INFO->Time);

  return true;
}

void
LXDevice::CRCWrite(const char *buff, unsigned size)
{
  port->Write(buff, size);
  crc = filser_calc_crc(buff, size, crc);
}

void
LXDevice::CRCWrite(char c)
{
  port->Write(c);
  crc = calc_crc_char(c, crc);
}

bool
LXDevice::ParseNMEA(const char *String, NMEA_INFO *GPS_INFO, bool enable_baro)
{
  NMEAInputLine line(String);
  char type[16];
  line.read(type, 16);

  if (strcmp(type, "$LXWP0") == 0)
    return LXWP0(line, GPS_INFO, enable_baro);

  if (strcmp(type, "$LXWP1") == 0)
    return LXWP1(line, GPS_INFO);

  if (strcmp(type, "$LXWP2") == 0)
    return LXWP2(line, GPS_INFO);

  return false;
}

void
LXDevice::CRCWriteint32(int32_t i)
{
  CRCWrite((char) ((i>>24) & 0xFF));
  CRCWrite((char) ((i>>16) & 0xFF));
  CRCWrite((char) ((i>>8) & 0xFF));
  CRCWrite((char) (i & 0xFF));
}

void
LXDevice::StartNMEAMode(OperationEnvironment &env)
{
  port->Write(LX_SYN);
  env.Sleep(500);
  port->Write(LX_SYN);
  env.Sleep(500);
  port->Write(LX_SYN);
  env.Sleep(500);
}

bool
LXDevice::StartCommandMode()
{
  port->Write(LX_SYN);
  port->ExpectString(LX_ACK_STRING);

  port->Write(LX_SYN);
  port->ExpectString(LX_ACK_STRING);

  port->Write(LX_SYN);
  if (!port->ExpectString(LX_ACK_STRING))
    return false;

  return true;
}

/**
 * fills dest with src and appends spaces to end
 * adds '\0' to end of string resulting in
 * len characters with last char = '\0'
 */
static void
copy_space_padded(char dest[], const TCHAR src[], unsigned int len)
{
  const unsigned slen = _tcslen(src);
  for(unsigned i = 0; i < (len - 1); i++) {
    if (i < slen)
      dest[i] = (char)max((src[i] & 0x7f), 0x20);
    else
      dest[i] = '\x20';
  }
  dest[len-1] = '\0';
}

void
LXDevice::LoadPilotInfo(const Declaration *decl)
{
  memset((void*)lxDevice_Pilot.unknown1, 0, sizeof(lxDevice_Pilot.unknown1));
  copy_space_padded(lxDevice_Pilot.PilotName, decl->PilotName,
                    sizeof(lxDevice_Pilot.PilotName));
  copy_space_padded(lxDevice_Pilot.GliderType, decl->AircraftType,
                    sizeof(lxDevice_Pilot.GliderType));
  copy_space_padded(lxDevice_Pilot.GliderID, decl->AircraftReg,
                    sizeof(lxDevice_Pilot.GliderID));
  copy_space_padded(lxDevice_Pilot.CompetitionID, decl->CompetitionId,
                    sizeof(lxDevice_Pilot.CompetitionID));
  memset((void*)lxDevice_Pilot.unknown2, 0, sizeof(lxDevice_Pilot.unknown2));
}

void
LXDevice::WritePilotInfo()
{
  CRCWrite((const char*)&lxDevice_Pilot, sizeof(lxDevice_Pilot));
  return;
}

/**
 * Loads LX task structure from XCSoar task structure
 * @param decl  The declaration
 */
bool
LXDevice::LoadTask(const Declaration *decl)
{
  if (decl->size() > 10)
    return false;

  if (decl->size() < 2)
      return false;

  memset((void*)lxDevice_Declaration.unknown1, 0,
          sizeof(lxDevice_Declaration.unknown1));

  if (DeclDate.day > 0 && DeclDate.day < 32
      && DeclDate.month > 0 && DeclDate.month < 13) {
    lxDevice_Declaration.dayinput = (unsigned char)DeclDate.day;
    lxDevice_Declaration.monthinput = (unsigned char)DeclDate.month;
    int iCentury = DeclDate.year / 100; // Todo: if no gps fix, use system time
    iCentury *= 100;
    lxDevice_Declaration.yearinput = (unsigned char)(DeclDate.year - iCentury);
  }
  else {
    lxDevice_Declaration.dayinput = (unsigned char)1;
    lxDevice_Declaration.monthinput = (unsigned char)1;
    lxDevice_Declaration.yearinput = (unsigned char)10;
  }
  lxDevice_Declaration.dayuser = lxDevice_Declaration.dayinput;
  lxDevice_Declaration.monthuser = lxDevice_Declaration.monthinput;
  lxDevice_Declaration.yearuser = lxDevice_Declaration.yearinput;
  lxDevice_Declaration.taskid = 0;
  lxDevice_Declaration.numtps = decl->size();

  for (unsigned i = 0; i < NUMTPS; i++) {
    if (i == 0) { // takeoff
      lxDevice_Declaration.tptypes[i] = 3;
      lxDevice_Declaration.Latitudes[i] = 0;
      lxDevice_Declaration.Longitudes[i] = 0;
      copy_space_padded(lxDevice_Declaration.WaypointNames[i], _T("TAKEOFF"),
        sizeof(lxDevice_Declaration.WaypointNames[i]));


    } else if (i <= decl->size()) {
      lxDevice_Declaration.tptypes[i] = 1;
      lxDevice_Declaration.Longitudes[i] =
          (int32_t)(decl->get_location(i - 1).Longitude.value_degrees()
           * 60000);
      lxDevice_Declaration.Latitudes[i] =
          (int32_t)(decl->get_location(i - 1).Latitude.value_degrees()
           * 60000);
      copy_space_padded(lxDevice_Declaration.WaypointNames[i],
                        decl->get_name(i - 1),
                        sizeof(lxDevice_Declaration.WaypointNames[i]));

    } else if (i == decl->size() + 1) { // landing
      lxDevice_Declaration.tptypes[i] = 2;
      lxDevice_Declaration.Longitudes[i] = 0;
      lxDevice_Declaration.Latitudes[i] = 0;
      copy_space_padded(lxDevice_Declaration.WaypointNames[i], _T("LANDING"),
          sizeof(lxDevice_Declaration.WaypointNames[i]));

    } else { // unused
      lxDevice_Declaration.tptypes[i] = 0;
      lxDevice_Declaration.Longitudes[i] = 0;
      lxDevice_Declaration.Latitudes[i] = 0;
      memset((void*)lxDevice_Declaration.WaypointNames[i], 0, 9);
    }
  }

  return true;
}

/**
 * Writes task structure to LX
 * LX task has max 12 points which include Takeoff,
 * start, tps, finish and landing.
 * Leave takeoff and landing as all 0's.
 * @param decl  The declaration
 */
void
LXDevice::WriteTask()
{
  CRCWrite((const char*)&lxDevice_Declaration,
            sizeof(lxDevice_Declaration.unknown1) +
            sizeof(lxDevice_Declaration.dayinput) +
            sizeof(lxDevice_Declaration.monthinput) +
            sizeof(lxDevice_Declaration.yearinput) +
            sizeof(lxDevice_Declaration.dayuser) +
            sizeof(lxDevice_Declaration.monthuser) +
            sizeof(lxDevice_Declaration.yearuser));

  CRCWrite((const char*)&lxDevice_Declaration.taskid,
            sizeof(lxDevice_Declaration.taskid));
  CRCWrite((char)lxDevice_Declaration.numtps);

  for (unsigned int i = 0; i < NUMTPS; i++) {
    CRCWrite((char)lxDevice_Declaration.tptypes[i]);
  }
  for (unsigned int i = 0; i < NUMTPS; i++) {
    CRCWriteint32(lxDevice_Declaration.Longitudes[i]);
  }
  for (unsigned int i = 0; i < NUMTPS; i++) {
    CRCWriteint32(lxDevice_Declaration.Latitudes[i]);
  }
  for (unsigned int i = 0; i < NUMTPS; i++) {
    CRCWrite(lxDevice_Declaration.WaypointNames[i],
             sizeof(lxDevice_Declaration.WaypointNames[i]));
  }
  return;
}

void
LXDevice::LoadContestClass(const Declaration *decl)
{
  copy_space_padded(lxDevice_ContestClass.contest_class, _T(""),
                    sizeof(lxDevice_ContestClass.contest_class));
}

void
LXDevice::WriteContestClass()
{
  CRCWrite((const char*)&lxDevice_ContestClass.contest_class,
            sizeof(lxDevice_ContestClass.contest_class));
  return;
}

bool
LXDevice::DeclareInner(const Declaration *decl, OperationEnvironment &env)
{
  if (!port->SetRxTimeout(2000))
    return false;

  if (!StartCommandMode())
      return false;

  LoadPilotInfo(decl);
  if (!LoadTask(decl))
    return false;
  LoadContestClass(decl);

  port->Write(LX_PREFIX);
  port->Write(LX_WRITE_FLIGHT_INFO);      // start declaration

  crc = 0xff;
  WritePilotInfo();
  WriteTask();
  port->Write(crc);
  if (!port->ExpectString(LX_ACK_STRING))
    return false;

  crc = 0xff;
  port->Write(LX_PREFIX);
  port->Write(LX_WRITE_CONTEST_CLASS);
  WriteContestClass();
  port->Write(crc);
  return port->ExpectString(LX_ACK_STRING);
}

bool
LXDevice::Declare(const Declaration *decl, OperationEnvironment &env)
{
  if (decl->size() < 2 || decl->size() > 12)
    return false;

  if (!port->StopRxThread())
    return false;

  bool success = DeclareInner(decl, env);

  StartNMEAMode(env);
  port->SetRxTimeout(0);
  port->StartRxThread();
  return success;
}

static Device *
LXCreateOnPort(Port *com_port)
{
  return new LXDevice(com_port);
}

const struct DeviceRegister lxDevice = {
  _T("LX"),
  drfLogger | drfBaroAlt,
  LXCreateOnPort,
};
