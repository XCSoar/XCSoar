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

// CAUTION!
// cai302ParseNMEA is called from com port read thread
// all other functions are called from windows message loop thread

#include "Device/Driver/CAI302.hpp"
#include "Device/Port.hpp"
#include "Device/Driver.hpp"
#include "Units.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"
#include "Waypoint/Waypoint.hpp"
#include "Math/FastMath.h"
#include "Operation.hpp"

#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#ifdef _UNICODE
#include <windows.h>
#endif

#define CtrlC 0x03
#define swap(x) x = ((((x<<8) & 0xff00) | ((x>>8) & 0x00ff)) & 0xffff)

#pragma pack(push, 1) // force byte alignment

/** Structure for CAI302 device info */
struct cai302_Wdata_t {
  unsigned char result[3];
  unsigned char reserved[15];
  unsigned char ID[3];
  unsigned char Type;
  unsigned char Version[5];
  unsigned char reserved2[6];
  unsigned char cai302ID;
  unsigned char reserved3;
};

/** Structure for CAI302 Odata info */
struct cai302_OdataNoArgs_t {
  unsigned char result[3];
  unsigned char PilotCount;
  unsigned char PilotRecordSize;
};

/** Structure for CAI302 settings */
struct cai302_OdataPilot_t {
  unsigned char  result[3];
  char           PilotName[24];
  unsigned char  OldUnit; // old unit
  unsigned char  OldTemperaturUnit; // 0 = Celcius, 1 = Farenheight
  unsigned char  SinkTone;
  unsigned char  TotalEnergyFinalGlide;
  unsigned char  ShowFinalGlideAltitude;
  unsigned char  MapDatum; // ignored on IGC version
  unsigned short ApproachRadius;
  unsigned short ArrivalRadius;
  unsigned short EnrouteLoggingInterval;
  unsigned short CloseTpLoggingInterval;
  unsigned short TimeBetweenFlightLogs; // [Minutes]
  unsigned short MinimumSpeedToForceFlightLogging; // (Knots)
  unsigned char  StfDeadBand; // (10ths M/S)
  unsigned char  ReservedVario; // multiplexed w/ vario mode:
                                // Tot Energy, SuperNetto, Netto
  unsigned short UnitWord;
  unsigned short Reserved2;
  unsigned short MarginHeight; // (10ths of Meters)
  unsigned char  Spare[60]; // 302 expect more data than the documented filed
                            // be shure there is space to hold the data
};

/** Structure for CAI302 glider response */
struct cai302_GdataNoArgs_t {
  unsigned char result[3];
  unsigned char GliderRecordSize;
};

/** Structure for CAI302 glider data */
struct cai302_Gdata_t {
  unsigned char  result[3];
  unsigned char  GliderType[12];
  unsigned char  GliderID[12];
  unsigned char  bestLD;
  unsigned char  BestGlideSpeed;
  unsigned char  TwoMeterSinkAtSpeed;
  unsigned char  Reserved1;
  unsigned short WeightInLiters;
  unsigned short BallastCapacity;
  unsigned short Reserved2;
  unsigned short ConfigWord; // locked(1) = FF FE.  unlocked(0) = FF FF
  unsigned short WingArea; // 100ths square meters
  unsigned char  Spare[60]; // 302 expect more data than the documented filed
                            // be shure there is space to hold the data
};

#pragma pack(pop)

/** 
 * Device driver for Cambridge Aero Instruments 302 
 */
class CAI302Device : public AbstractDevice {
private:
  Port *port;

public:
  CAI302Device(Port *_port)
    :port(_port) {}

public:
  virtual bool Open(OperationEnvironment &env);
  virtual bool ParseNMEA(const char *line, struct NMEA_INFO *info);
  virtual bool PutMacCready(fixed MacCready);
  virtual bool PutBugs(fixed bugs);
  virtual bool PutBallast(fixed ballast);
  virtual bool Declare(const Declaration *declaration,
                       OperationEnvironment &env);

private:
  bool cai_w(NMEAInputLine &line, NMEA_INFO *GPS_INFO);
};

/*
$PCAIB,<1>,<2>,<CR><LF>
<1> Destination Navpoint elevation in meters, format XXXXX (leading zeros will be transmitted)
<2> Destination Navpoint attribute word, format XXXXX (leading zeros will be transmitted)
*/
static bool
cai_PCAIB(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  (void)line;
  (void)GPS_INFO;
  return true;
}

/*
$PCAID,<1>,<2>,<3>,<4>*hh<CR><LF>
<1> Logged 'L' Last point Logged 'N' Last Point not logged
<2> Barometer Altitude in meters (Leading zeros will be transmitted)
<3> Engine Noise Level
<4> Log Flags
*hh Checksum, XOR of all bytes of the sentence after the ‘$’ and before the ‘*’
*/
static bool
cai_PCAID(NMEAInputLine &line, NMEA_INFO &data)
{
  line.skip();

  fixed value;
  if (line.read_checked(value))
    data.ProvidePressureAltitude(value);

  unsigned enl;
  if (line.read_checked(enl)) {
    data.engine_noise_level = enl;
    data.engine_noise_level_available.update(data.Time);
  }

  return true;
}

bool
CAI302Device::ParseNMEA(const char *String, NMEA_INFO *GPS_INFO)
{
  if (!VerifyNMEAChecksum(String))
    return false;

  NMEAInputLine line(String);
  char type[16];
  line.read(type, 16);

  if (strcmp(type, "$PCAIB") == 0)
    return cai_PCAIB(line, GPS_INFO);

  if (strcmp(type, "$PCAID") == 0)
    return cai_PCAID(line, *GPS_INFO);

  if (strcmp(type, "!w") == 0)
    return cai_w(line, GPS_INFO);

  return false;
}

bool
CAI302Device::PutMacCready(fixed MacCready)
{
  unsigned mac_cready = uround(Units::ToUserUnit(MacCready * 10, unKnots));

  char szTmp[32];
  sprintf(szTmp, "!g,m%u\r", mac_cready);
  port->Write(szTmp);

  return true;
}

bool
CAI302Device::PutBugs(fixed Bugs)
{
  unsigned bugs = uround(Bugs * 100);

  char szTmp[32];
  sprintf(szTmp, "!g,u%u\r", bugs);
  port->Write(szTmp);

  return true;
}

bool
CAI302Device::PutBallast(fixed Ballast)
{
  unsigned ballast = uround(Ballast * 100);

  char szTmp[32];
  sprintf(szTmp, "!g,b%u\r", ballast);
  port->Write(szTmp);

  return true;
}

bool
CAI302Device::Open(OperationEnvironment &env)
{
  port->Write('\x03');
  port->Write("LOG 0\r");

  return true;
}

static int DeclIndex = 128;

static void
convert_string(char *dest, size_t size, const TCHAR *src)
{
#ifdef _UNICODE
  size_t length = _tcslen(src);
  if (length >= size)
    length = size - 1;

  int length2 = ::WideCharToMultiByte(CP_ACP, 0, src, length, dest, size,
                                      NULL, NULL);
  if (length2 < 0)
    length2 = 0;
  dest[length2] = '\0';
#else
  strncpy(dest, src, size - 1);
  dest[size - 1] = '\0';
#endif
}

static bool
cai302DeclAddWayPoint(Port *port, const Waypoint &way_point)
{
  int DegLat, DegLon;
  double tmp, MinLat, MinLon;
  char NoS, EoW;

  tmp = way_point.Location.Latitude.value_degrees();
  NoS = 'N';
  if (tmp < 0) {
    NoS = 'S';
    tmp = -tmp;
  }
  DegLat = (int)tmp;
  MinLat = (tmp - DegLat) * 60;

  tmp = way_point.Location.Longitude.value_degrees();
  EoW = 'E';
  if (tmp < 0) {
    EoW = 'W';
    tmp = -tmp;
  }
  DegLon = (int)tmp;
  MinLon = (tmp - DegLon) * 60;

  char Name[13];
  convert_string(Name, sizeof(Name), way_point.Name.c_str());

  char szTmp[128];
  sprintf(szTmp, "D,%d,%02d%07.4f%c,%03d%07.4f%c,%s,%d\r",
          DeclIndex,
          DegLat, MinLat, NoS,
          DegLon, MinLon, EoW,
          Name,
          (int)way_point.Altitude);

  DeclIndex++;

  port->Write(szTmp);

  return port->ExpectString("dn>");
}

static bool
ReadBlock(Port &port, void *dest, size_t max_length, size_t length)
{
  assert(dest != NULL);
  assert(max_length > 0);

  if (length > max_length)
    length = max_length;

  if (length == 0)
    return true;

  return port.FullRead(dest, length, 3000);
}

static bool
DeclareInner(Port *port, const Declaration *decl,
             OperationEnvironment &env)
{
  port->SetRxTimeout(500);

  port->Flush();
  port->Write('\x03');

  port->GetChar();

  /* empty rx buffer */
  port->SetRxTimeout(0);
  while (port->GetChar() != EOF) {}

  port->SetRxTimeout(500);
  port->Write('\x03');
  if (!port->ExpectString("cmd>"))
    return false;

  port->Write("upl 1\r");
  if (!port->ExpectString("up>"))
    return false;

  port->Flush();

  port->Write("O\r");

  port->SetRxTimeout(1500);

  cai302_OdataNoArgs_t cai302_OdataNoArgs;
  if (!ReadBlock(*port, &cai302_OdataNoArgs, sizeof(cai302_OdataNoArgs),
                 sizeof(cai302_OdataNoArgs)) ||
      !port->ExpectString("up>"))
    return false;

  port->Write("O 0\r"); // 0=active pilot

  cai302_OdataPilot_t cai302_OdataPilot;
  if (!ReadBlock(*port, &cai302_OdataPilot, sizeof(cai302_OdataPilot),
                 cai302_OdataNoArgs.PilotRecordSize + 3) ||
      !port->ExpectString("up>"))
    return false;

  swap(cai302_OdataPilot.ApproachRadius);
  swap(cai302_OdataPilot.ArrivalRadius);
  swap(cai302_OdataPilot.EnrouteLoggingInterval);
  swap(cai302_OdataPilot.CloseTpLoggingInterval);
  swap(cai302_OdataPilot.TimeBetweenFlightLogs);
  swap(cai302_OdataPilot.MinimumSpeedToForceFlightLogging);
  swap(cai302_OdataPilot.UnitWord);
  swap(cai302_OdataPilot.MarginHeight);

  port->Write("G\r");

  cai302_GdataNoArgs_t cai302_GdataNoArgs;
  if (!ReadBlock(*port, &cai302_GdataNoArgs, sizeof(cai302_GdataNoArgs),
                 sizeof(cai302_GdataNoArgs)) ||
      !port->ExpectString("up>"))
    return false;

  port->Write("G 0\r");

  cai302_Gdata_t cai302_Gdata;
  if (!ReadBlock(*port, &cai302_Gdata, sizeof(cai302_Gdata),
                 cai302_GdataNoArgs.GliderRecordSize + 3) ||
      !port->ExpectString("up>"))
    return false;

  swap(cai302_Gdata.WeightInLiters);
  swap(cai302_Gdata.BallastCapacity);
  swap(cai302_Gdata.ConfigWord);
  swap(cai302_Gdata.WingArea);

  port->Write('\x03');
  if (!port->ExpectString("cmd>"))
    return false;

  port->Write("dow 1\r");
  if (!port->ExpectString("dn>"))
    return false;

  char PilotName[25], GliderType[13], GliderID[13];
  convert_string(PilotName, sizeof(PilotName), decl->PilotName);
  convert_string(GliderType, sizeof(GliderType), decl->AircraftType);
  convert_string(GliderID, sizeof(GliderID), decl->AircraftReg);

  char szTmp[255];
  sprintf(szTmp, "O,%-24s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r",
          PilotName,
          cai302_OdataPilot.OldUnit,
          cai302_OdataPilot.OldTemperaturUnit,
          cai302_OdataPilot.SinkTone,
          cai302_OdataPilot.TotalEnergyFinalGlide,
          cai302_OdataPilot.ShowFinalGlideAltitude,
          cai302_OdataPilot.MapDatum,
          cai302_OdataPilot.ApproachRadius,
          cai302_OdataPilot.ArrivalRadius,
          cai302_OdataPilot.EnrouteLoggingInterval,
          cai302_OdataPilot.CloseTpLoggingInterval,
          cai302_OdataPilot.TimeBetweenFlightLogs,
          cai302_OdataPilot.MinimumSpeedToForceFlightLogging,
          cai302_OdataPilot.StfDeadBand,
          cai302_OdataPilot.ReservedVario,
          cai302_OdataPilot.UnitWord,
          cai302_OdataPilot.MarginHeight);

  port->Write(szTmp);
  if (!port->ExpectString("dn>"))
    return false;

  sprintf(szTmp, "G,%-12s,%-12s,%d,%d,%d,%d,%d,%d,%d,%d\r",
          GliderType,
          GliderID,
          cai302_Gdata.bestLD,
          cai302_Gdata.BestGlideSpeed,
          cai302_Gdata.TwoMeterSinkAtSpeed,
          cai302_Gdata.WeightInLiters,
          cai302_Gdata.BallastCapacity,

          0,
          cai302_Gdata.ConfigWord,
          cai302_Gdata.WingArea);

  port->Write(szTmp);
  if (!port->ExpectString("dn>"))
    return false;

  DeclIndex = 128;

  for (unsigned i = 0; i < decl->size(); ++i)
    if (!cai302DeclAddWayPoint(port, decl->get_waypoint(i)))
      return false;

  port->Write("D,255\r");
  port->SetRxTimeout(1500); // D,255 takes more than 800ms
  return port->ExpectString("dn>");
}

bool
CAI302Device::Declare(const Declaration *decl, OperationEnvironment &env)
{
  port->StopRxThread();

  bool success = DeclareInner(port, decl, env);

  port->SetRxTimeout(500);

  port->Write('\x03');
  port->ExpectString("cmd>");

  port->Write("LOG 0\r");

  port->SetRxTimeout(0);
  port->StartRxThread();

  return success;
}

static Device *
CAI302CreateOnPort(Port *com_port)
{
  return new CAI302Device(com_port);
}

const struct DeviceRegister cai302Device = {
  _T("CAI 302"),
  drfBaroAlt | drfLogger,
  CAI302CreateOnPort,
};

static bool
ReadSpeedVector(NMEAInputLine &line, SpeedVector &value_r)
{
  fixed bearing, norm;

  bool bearing_valid = line.read_checked(bearing);
  bool norm_valid = line.read_checked(norm);

  if (bearing_valid && norm_valid) {
    value_r.bearing = Angle::degrees(bearing);
    value_r.norm = norm / 10;
    return true;
  }

  return false;
}

/*
!w,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>,<9>,<10>,<11>,<12>,<13>*hh<CR><LF>
<1>  Vector wind direction in degrees
<2>  Vector wind speed in 10ths of meters per second
<3>  Vector wind age in seconds
<4>  Component wind in 10ths of Meters per second + 500 (500 = 0, 495 = 0.5 m/s tailwind)
<5>  True altitude in Meters + 1000
<6>  Instrument QNH setting
<7>  True airspeed in 100ths of Meters per second
<8>  Variometer reading in 10ths of knots + 200
<9>  Averager reading in 10ths of knots + 200
<10> Relative variometer reading in 10ths of knots + 200
<11> Instrument MacCready setting in 10ths of knots
<12> Instrument Ballast setting in percent of capacity
<13> Instrument Bug setting
*hh  Checksum, XOR of all bytes
*/

bool
CAI302Device::cai_w(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  SpeedVector wind;
  if (ReadSpeedVector(line, wind))
    GPS_INFO->ProvideExternalWind(wind.Reciprocal());

  line.skip(2);

  fixed value;
  if (line.read_checked(value))
    GPS_INFO->ProvideBaroAltitudeTrue(value - fixed(1000));

  if (line.read_checked(value))
    GPS_INFO->settings.ProvideQNH(value, GPS_INFO->Time);

  if (line.read_checked(value))
    GPS_INFO->ProvideTrueAirspeed(value / 100);

  if (line.read_checked(value))
    GPS_INFO->ProvideTotalEnergyVario(Units::ToSysUnit((value - fixed(200)) / 10,
                                                       unKnots));

  line.skip(2);

  int i;

  if (line.read_checked(i))
    GPS_INFO->settings.ProvideMacCready(Units::ToSysUnit(fixed(i) / 10, unKnots),
                                        GPS_INFO->Time);

  if (line.read_checked(i))
    GPS_INFO->settings.ProvideBallast(value / 100, GPS_INFO->Time);

  if (line.read_checked(i))
    GPS_INFO->settings.ProvideBugs(value / 100, GPS_INFO->Time);

  return true;
}
