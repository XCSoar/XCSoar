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
#include "Protection.hpp"
#include "Units.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"

#include <stdlib.h>

static const unsigned int NUMTPS = 12;
enum LX_command {
  LX_PREFIX = 0x02,
  LX_ACK = 0x06,
  LX_SYN = 0x16,
  LX_WRITE_FLIGHT_INFO = 0xCA
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
  void StartNMEAMode();
  void LoadPilotInfo(const Declaration *decl);
  void WritePilotInfo();
  bool LoadTask(const Declaration *decl);
  void WriteTask();
  void WriteToNanoint32(int32_t i);
  void CRCWrite(const char *buff, unsigned size);
  void CRCWrite(char c);

  struct lxNanoDevice_Pilot_t { //strings have extra byte for NULL
    char unknown1[3];
    char PilotName[19];
    char GliderType[12];
    char GliderID[8];
    char CompetitionID[4];
    char unknown2[73];
  } gcc_packed;

  struct lxNanoDevice_Declaration_t { //strings have extra byte for NULL
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

  lxNanoDevice_Declaration_t lxNanoDevice_Declaration;
  lxNanoDevice_Pilot_t lxNanoDevice_Pilot;
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
calc_crc_char(char d, char crc) {
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
filser_calc_crc(const char *p0, size_t len, char crc) {
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
    GPS_INFO->ProvidePressureAltitude(NMEA_INFO::BARO_ALTITUDE_LX, alt);

  if (tas_available)
    GPS_INFO->ProvideTrueAirspeedWithAltitude(Units::ToSysUnit(airspeed,
                                                               unKiloMeterPerHour),
                                              alt);

  if (line.read_checked(value)) {
    GPS_INFO->ProvideTotalEnergyVario(value);
    TriggerVarioUpdate();
  }

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
    GPS_INFO->MacCready = value;

  // Ballast
  line.skip();
  /*
  if (line.read_checked(value))
    GPS_INFO->Ballast = value;
  */

  // Bugs
  if (line.read_checked(value))
    GPS_INFO->Bugs = fixed(100) - value;

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
LXDevice::WriteToNanoint32(int32_t i)
{
  CRCWrite((char) ((i>>24) & 0xFF));
  CRCWrite((char) ((i>>16) & 0xFF));
  CRCWrite((char) ((i>>8) & 0xFF));
  CRCWrite((char) (i & 0xFF));
}

void
LXDevice::StartNMEAMode()
{
  port->Write(LX_SYN);
  port->ExpectString("$$$");
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
  memset((void*)lxNanoDevice_Pilot.unknown1, 0, sizeof(lxNanoDevice_Pilot.unknown1));
  copy_space_padded(lxNanoDevice_Pilot.PilotName, decl->PilotName, sizeof(lxNanoDevice_Pilot.PilotName));
  copy_space_padded(lxNanoDevice_Pilot.GliderType, decl->AircraftType, sizeof(lxNanoDevice_Pilot.GliderType));
  copy_space_padded(lxNanoDevice_Pilot.GliderID, decl->AircraftReg, sizeof(lxNanoDevice_Pilot.GliderID));
  copy_space_padded(lxNanoDevice_Pilot.CompetitionID, decl->CompetitionId, sizeof(lxNanoDevice_Pilot.CompetitionID));
  memset((void*)lxNanoDevice_Pilot.unknown2, 0, sizeof(lxNanoDevice_Pilot.unknown2));
}

void
LXDevice::WritePilotInfo()
{
  CRCWrite((const char*)&lxNanoDevice_Pilot, sizeof(lxNanoDevice_Pilot));
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

  memset((void*)lxNanoDevice_Declaration.unknown1, 0, sizeof(lxNanoDevice_Declaration.unknown1));

  if (DeclDate.day > 0 && DeclDate.day < 32
      && DeclDate.month > 0 && DeclDate.month < 13) {
    lxNanoDevice_Declaration.dayinput = (unsigned char)DeclDate.day;
    lxNanoDevice_Declaration.monthinput = (unsigned char)DeclDate.month;
    int iCentury = DeclDate.year / 100; // Todo: if no gps fix, use system time
    iCentury *= 100;
    lxNanoDevice_Declaration.yearinput = (unsigned char)(DeclDate.year - iCentury);
  }
  else {
    lxNanoDevice_Declaration.dayinput = (unsigned char)1;
    lxNanoDevice_Declaration.monthinput = (unsigned char)1;
    lxNanoDevice_Declaration.yearinput = (unsigned char)10;
  }
  lxNanoDevice_Declaration.dayuser = lxNanoDevice_Declaration.dayinput;
  lxNanoDevice_Declaration.monthuser = lxNanoDevice_Declaration.monthinput;
  lxNanoDevice_Declaration.yearuser = lxNanoDevice_Declaration.yearinput;
  lxNanoDevice_Declaration.taskid = 0;
  lxNanoDevice_Declaration.numtps = decl->size();

  for (unsigned i = 0; i < NUMTPS; i++) {
    if (i == 0) { // takeoff
      lxNanoDevice_Declaration.tptypes[i] = 3;
      lxNanoDevice_Declaration.Latitudes[i] = 0;
      lxNanoDevice_Declaration.Longitudes[i] = 0;
      copy_space_padded(lxNanoDevice_Declaration.WaypointNames[i], _T("TAKEOFF"),
        sizeof(lxNanoDevice_Declaration.WaypointNames[i]));


    } else if (i <= decl->size()) {
      lxNanoDevice_Declaration.tptypes[i] = 1;
      lxNanoDevice_Declaration.Longitudes[i] =
          (int32_t)(decl->get_location(i - 1).Longitude.value_degrees() * 60000);
      lxNanoDevice_Declaration.Latitudes[i] =
          (int32_t)(decl->get_location(i - 1).Latitude.value_degrees() * 60000);
      copy_space_padded(lxNanoDevice_Declaration.WaypointNames[i], decl->get_name(i - 1),
          sizeof(lxNanoDevice_Declaration.WaypointNames[i]));

    } else if (i == decl->size() + 1) { // landing
      lxNanoDevice_Declaration.tptypes[i] = 2;
      lxNanoDevice_Declaration.Longitudes[i] = 0;
      lxNanoDevice_Declaration.Latitudes[i] = 0;
      copy_space_padded(lxNanoDevice_Declaration.WaypointNames[i], _T("LANDING"),
          sizeof(lxNanoDevice_Declaration.WaypointNames[i]));

    } else { // unused
      lxNanoDevice_Declaration.tptypes[i] = 0;
      lxNanoDevice_Declaration.Longitudes[i] = 0;
      lxNanoDevice_Declaration.Latitudes[i] = 0;
      memset((void*)lxNanoDevice_Declaration.WaypointNames[i], 0, 9);
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
  CRCWrite((const char*)&lxNanoDevice_Declaration,
                                    sizeof(lxNanoDevice_Declaration.unknown1) +
                                    sizeof(lxNanoDevice_Declaration.dayinput) +
                                    sizeof(lxNanoDevice_Declaration.monthinput) +
                                    sizeof(lxNanoDevice_Declaration.yearinput) +
                                    sizeof(lxNanoDevice_Declaration.dayuser) +
                                    sizeof(lxNanoDevice_Declaration.monthuser) +
                                    sizeof(lxNanoDevice_Declaration.yearuser));

  CRCWrite((const char*)&lxNanoDevice_Declaration.taskid, sizeof(lxNanoDevice_Declaration.taskid));
  CRCWrite((char)lxNanoDevice_Declaration.numtps);

  for (unsigned int i = 0; i < NUMTPS; i++) {
    CRCWrite((char)lxNanoDevice_Declaration.tptypes[i]);
  }
  for (unsigned int i = 0; i < NUMTPS; i++) {
    WriteToNanoint32(lxNanoDevice_Declaration.Longitudes[i]);
  }
  for (unsigned int i = 0; i < NUMTPS; i++) {
    WriteToNanoint32(lxNanoDevice_Declaration.Latitudes[i]);
  }
  for (unsigned int i = 0; i < NUMTPS; i++) {
    CRCWrite(lxNanoDevice_Declaration.WaypointNames[i], sizeof(lxNanoDevice_Declaration.WaypointNames[i]));
  }
  return;
}

bool
LXDevice::DeclareInner(const Declaration *decl, OperationEnvironment &env)
{
  if (!port->SetRxTimeout(500))
    return false;

  if (!StartCommandMode())
      return false;

  LoadPilotInfo(decl);
  if (!LoadTask(decl))
    return false;

  port->Write(LX_PREFIX);
  port->Write(LX_WRITE_FLIGHT_INFO);      // start declaration

  crc = 0xff;
  WritePilotInfo();
  WriteTask();
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

  StartNMEAMode();
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
