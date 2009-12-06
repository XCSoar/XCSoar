/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

//


// CAUTION!
// cai302ParseNMEA is called from com port read thread
// all other functions are called from windows message loop thread

#include "Device/Driver/CAI302.hpp"
#include "Device/Internal.hpp"
#include "Protection.hpp"
#include "Device/Parser.h"
#include "Device/Port.h"
#include "Math/Units.h"
#include "McReady.h"
#include "NMEA/Info.h"

#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>

#define  CtrlC  0x03
#define  swap(x)      x = ((((x<<8) & 0xff00) | ((x>>8) & 0x00ff)) & 0xffff)

#pragma pack(push, 1)                  // force byte allignement

typedef struct{
  unsigned char result[3];
  unsigned char reserved[15];
  unsigned char ID[3];
  unsigned char Type;
  unsigned char Version[5];
  unsigned char reserved2[6];
  unsigned char cai302ID;
  unsigned char reserved3;
}cai302_Wdata_t;

typedef struct{
  unsigned char result[3];
  unsigned char PilotCount;
  unsigned char PilotRecordSize;
}cai302_OdataNoArgs_t;

typedef struct{
  unsigned char  result[3];
  char           PilotName[24];
  unsigned char  OldUnit;                                       // old unit
  unsigned char  OldTemperaturUnit;                             //0=Celcius 1=Farenheight
  unsigned char  SinkTone;
  unsigned char  TotalEnergyFinalGlide;
  unsigned char  ShowFinalGlideAltitude;
  unsigned char  MapDatum;  // ignored on IGC version
  unsigned short ApproachRadius;
  unsigned short ArrivalRadius;
  unsigned short EnrouteLoggingInterval;
  unsigned short CloseTpLoggingInterval;
  unsigned short TimeBetweenFlightLogs;                     // [Minutes]
  unsigned short MinimumSpeedToForceFlightLogging;          // (Knots)
  unsigned char  StfDeadBand;                                // (10ths M/S)
  unsigned char  ReservedVario;                           // multiplexed w/ vario mode:  Tot Energy, SuperNeto, Netto
  unsigned short UnitWord;
  unsigned short Reserved2;
  unsigned short MarginHeight;                              // (10ths of Meters)
  unsigned char  Spare[60];                                 // 302 expect more data than the documented filed
                                                            // be shure there is space to hold the data
}cai302_OdataPilot_t;

typedef struct{
  unsigned char result[3];
  unsigned char GliderRecordSize;
}cai302_GdataNoArgs_t;

typedef struct{
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
  unsigned short ConfigWord;                                //locked(1) = FF FE.  unlocked(0) = FF FF
  unsigned short WingArea;                                  // 100ths square meters
  unsigned char  Spare[60];                                 // 302 expect more data than the documented filed
                                                            // be shure there is space to hold the data
}cai302_Gdata_t;


#pragma pack(pop)

class CAI302Device : public AbstractDevice {
private:
  ComPort *port;

public:
  CAI302Device(ComPort *_port):port(_port) {}

public:
  virtual bool Open();
  virtual bool ParseNMEA(const TCHAR *line, struct NMEA_INFO *info,
                         bool enable_baro);
  virtual bool PutMcCready(double mc_cready);
  virtual bool PutBugs(double bugs);
  virtual bool PutBallast(double ballast);
#ifdef OLD_TASK
  virtual bool Declare(const struct Declaration *declaration);
#endif
};

//static cai302_Wdata_t cai302_Wdata;
static cai302_OdataNoArgs_t cai302_OdataNoArgs;
static cai302_OdataPilot_t cai302_OdataPilot;
static cai302_GdataNoArgs_t cai302_GdataNoArgs;
static cai302_Gdata_t cai302_Gdata;

// Additional sentance for CAI302 support
static bool
cai_w(const TCHAR *String, NMEA_INFO *GPS_INFO, bool enable_baro);

static bool
cai_PCAIB(const TCHAR *String, NMEA_INFO *GPS_INFO);

static bool
cai_PCAID(const TCHAR *String, NMEA_INFO *GPS_INFO);

static int  MacCreadyUpdateTimeout = 0;
static int  BugsUpdateTimeout = 0;
static int  BallastUpdateTimeout = 0;

bool
CAI302Device::ParseNMEA(const TCHAR *String, NMEA_INFO *GPS_INFO,
                        bool enable_baro)
{
  if (!NMEAParser::NMEAChecksum(String) || (GPS_INFO == NULL)){
    return false;
  }

  if(_tcsstr(String, _T("$PCAIB")) == String){
    return cai_PCAIB(&String[7], GPS_INFO);
  }

  if(_tcsstr(String, _T("$PCAID")) == String){
    return cai_PCAID(&String[7], GPS_INFO);
  }

  if(_tcsstr(String, _T("!w")) == String){
    return cai_w(&String[3], GPS_INFO, enable_baro);
  }

  return false;
}

bool
CAI302Device::PutMcCready(double MacCready)
{
  TCHAR  szTmp[32];

  _stprintf(szTmp, _T("!g,m%d\r\n"), int(((MacCready * 10) / KNOTSTOMETRESSECONDS) + 0.5));

  port->WriteString(szTmp);

  MacCreadyUpdateTimeout = 2;

  return true;
}

bool
CAI302Device::PutBugs(double Bugs)
{
  TCHAR  szTmp[32];

  _stprintf(szTmp, _T("!g,u%d\r\n"), int((Bugs * 100) + 0.5));

  port->WriteString(szTmp);

  BugsUpdateTimeout = 2;

  return true;
}

bool
CAI302Device::PutBallast(double Ballast)
{
  TCHAR  szTmp[32];

  _stprintf(szTmp, _T("!g,b%d\r\n"), int((Ballast * 10) + 0.5));

  port->WriteString(szTmp);

  BallastUpdateTimeout = 2;

  return true;
}

bool
CAI302Device::Open()
{
  port->WriteString(_T("\x03"));
  port->WriteString(_T("LOG 0\r"));

  return true;
}

static int DeclIndex = 128;
static int nDeclErrorCode;

#ifdef OLD_TASK
static bool
cai302DeclAddWayPoint(ComPort *port, const WAYPOINT &way_point);

bool
CAI302Device::Declare(const struct Declaration *decl)
{
  TCHAR PilotName[25];
  TCHAR GliderType[13];
  TCHAR GliderID[13];
  TCHAR szTmp[255];
  nDeclErrorCode = 0;

  port->StopRxThread();

  port->SetRxTimeout(500);
  port->WriteString(_T("\x03"));
  ExpectString(port, _T("$$$"));  // empty rx buffer (searching for
                                 // pattern that never occure)

  port->WriteString(_T("\x03"));
  if (!ExpectString(port, _T("cmd>"))){
    nDeclErrorCode = 1;
    return false;
  }

  port->WriteString(_T("upl 1\r"));
  if (!ExpectString(port, _T("up>"))){
    nDeclErrorCode = 1;
    return false;
  }

  ExpectString(port, _T("$$$"));

  port->WriteString(_T("O\r"));
  port->Read(&cai302_OdataNoArgs, sizeof(cai302_OdataNoArgs));
  if (!ExpectString(port, _T("up>"))){
    nDeclErrorCode = 1;
    return false;
  }

  port->WriteString(_T("O 0\r"));  // 0=active pilot
  Sleep(1000); // some params come up 0 if we don't wait!
  port->Read(&cai302_OdataPilot, min(sizeof(cai302_OdataPilot),
                                       (size_t)cai302_OdataNoArgs.PilotRecordSize+3));
  if (!ExpectString(port, _T("up>"))){
    nDeclErrorCode = 1;
    return false;
  }

  swap(cai302_OdataPilot.ApproachRadius);
  swap(cai302_OdataPilot.ArrivalRadius);
  swap(cai302_OdataPilot.EnrouteLoggingInterval);
  swap(cai302_OdataPilot.CloseTpLoggingInterval);
  swap(cai302_OdataPilot.TimeBetweenFlightLogs);
  swap(cai302_OdataPilot.MinimumSpeedToForceFlightLogging);
  swap(cai302_OdataPilot.UnitWord);
  swap(cai302_OdataPilot.MarginHeight);

  port->WriteString(_T("G\r"));
  port->Read(&cai302_GdataNoArgs, sizeof(cai302_GdataNoArgs));
  if (!ExpectString(port, _T("up>"))){
    nDeclErrorCode = 1;
    return false;
  }

  port->WriteString(_T("G 0\r"));
  Sleep(1000);
  port->Read(&cai302_Gdata, cai302_GdataNoArgs.GliderRecordSize + 3);
  if (!ExpectString(port, _T("up>"))){
    nDeclErrorCode = 1;
    return false;
  }

  swap(cai302_Gdata.WeightInLiters);
  swap(cai302_Gdata.BallastCapacity);
  swap(cai302_Gdata.ConfigWord);
  swap(cai302_Gdata.WingArea);

  port->SetRxTimeout(1500);

  port->WriteString(_T("\x03"));
  if (!ExpectString(port, _T("cmd>"))){
    nDeclErrorCode = 1;
    return false;
  }

  port->WriteString(_T("dow 1\r"));
  if (!ExpectString(port, _T("dn>"))){
    nDeclErrorCode = 1;
    return false;
  }

  _tcsncpy(PilotName, decl->PilotName, 24);
  PilotName[24] = '\0';
  _tcsncpy(GliderType, decl->AircraftType, 12);
  GliderType[12] = '\0';
  _tcsncpy(GliderID, decl->AircraftRego, 12);
  GliderID[12] = '\0';

  _stprintf(szTmp, _T("O,%-24s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r"),
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
    cai302_OdataPilot.MarginHeight
  );


  port->WriteString(szTmp);
  if (!ExpectString(port, _T("dn>"))){
    nDeclErrorCode = 1;
    return false;
  }

  _stprintf(szTmp, _T("G,%-12s,%-12s,%d,%d,%d,%d,%d,%d,%d,%d\r"),
    GliderType,
    GliderID,
    cai302_Gdata.bestLD,
    cai302_Gdata.BestGlideSpeed,
    cai302_Gdata.TwoMeterSinkAtSpeed,
    cai302_Gdata.WeightInLiters,
    cai302_Gdata.BallastCapacity,
    0,
    cai302_Gdata.ConfigWord,
    cai302_Gdata.WingArea
  );

  port->WriteString(szTmp);
  if (!ExpectString(port, _T("dn>"))){
    nDeclErrorCode = 1;
    return false;
  }

  DeclIndex = 128;

  for (int i = 0; i < decl->num_waypoints; i++)
    cai302DeclAddWayPoint(port, *decl->waypoint[i]);

  if (nDeclErrorCode == 0){

    _stprintf(szTmp, _T("D,%d\r"), 255 /* end of declaration */);
    port->WriteString(szTmp);

    port->SetRxTimeout(1500);            // D,255 takes more than 800ms

    if (!ExpectString(port, _T("dn>"))){
      nDeclErrorCode = 1;
    }

    // todo error checking
  }

  port->SetRxTimeout(500);

  port->WriteString(_T("\x03"));
  ExpectString(port, _T("cmd>"));

  port->WriteString(_T("LOG 0\r"));

  port->SetRxTimeout(0);
  port->StartRxThread();

  return(nDeclErrorCode == 0);

}

static bool
cai302DeclAddWayPoint(ComPort *port, const WAYPOINT &way_point)
{
  TCHAR Name[13];
  TCHAR  szTmp[128];
  int DegLat, DegLon;
  double tmp, MinLat, MinLon;
  char NoS, EoW;

  if (nDeclErrorCode != 0)
    return false;

  _tcsncpy(Name, way_point.Name, 12);
  Name[12] = '\0';

  tmp = way_point.Location.Latitude;
  NoS = 'N';
  if (tmp < 0)
    {
      NoS = 'S';
      tmp = -tmp;
    }
  DegLat = (int)tmp;
  MinLat = (tmp - DegLat) * 60;


  tmp = way_point.Location.Longitude;
  EoW = 'E';
  if (tmp < 0)
    {
      EoW = 'W';
      tmp = -tmp;
    }
  DegLon = (int)tmp;
  MinLon = (tmp - DegLon) * 60;

  _stprintf(szTmp, _T("D,%d,%02d%07.4f%c,%03d%07.4f%c,%s,%d\r"),
    DeclIndex,
    DegLat, MinLat, NoS,
    DegLon, MinLon, EoW,
    Name,
    (int)way_point.Altitude
  );

  DeclIndex++;

  port->WriteString(szTmp);

  if (!ExpectString(port, _T("dn>"))){
    nDeclErrorCode = 1;
    return false;
  }

  return true;
}

#endif

static Device *
CAI302CreateOnComPort(ComPort *com_port)
{
  return new CAI302Device(com_port);
}

const struct DeviceRegister cai302Device = {
  _T("CAI 302"),
  drfGPS | drfLogger | drfSpeed | drfVario | drfWind, /* XXX: drfBaroAlt? */
  CAI302CreateOnComPort,
};

// local stuff

/*
$PCAIB,<1>,<2>,<CR><LF>
<1> Destination Navpoint elevation in meters, format XXXXX (leading zeros will be transmitted)
<2> Destination Navpoint attribute word, format XXXXX (leading zeros will be transmitted)
*/

static bool
cai_PCAIB(const TCHAR *String, NMEA_INFO *GPS_INFO)
{
  (void)GPS_INFO;
  (void)String;
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
cai_PCAID(const TCHAR *String, NMEA_INFO *GPS_INFO)
{
	(void)GPS_INFO;
	(void)String;
  return true;
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

static bool
cai_w(const TCHAR *String, NMEA_INFO *GPS_INFO,
      bool enable_baro)
{

  TCHAR ctemp[80];


  NMEAParser::ExtractParameter(String,ctemp,1);
  GPS_INFO->ExternalWindAvailalbe = true;
  GPS_INFO->ExternalWindSpeed = _tcstod(ctemp, NULL) / 10.0;
  NMEAParser::ExtractParameter(String,ctemp,0);
  GPS_INFO->ExternalWindDirection = _tcstod(ctemp, NULL);


  NMEAParser::ExtractParameter(String,ctemp,4);

  if (enable_baro){

    GPS_INFO->BaroAltitudeAvailable = true;
    GPS_INFO->BaroAltitude = _tcstod(ctemp, NULL) - 1000;

  }

  // ExtractParameter(String,ctemp,5);
  // GPS_INFO->pressure.set_QNH(_tcstod(ctemp, NULL) - 1000); ?

  NMEAParser::ExtractParameter(String,ctemp,6);
  GPS_INFO->AirspeedAvailable = true;
  GPS_INFO->TrueAirspeed = _tcstod(ctemp, NULL) / 100.0;

  NMEAParser::ExtractParameter(String,ctemp,7);
  GPS_INFO->VarioAvailable = true;
  GPS_INFO->Vario = ((_tcstod(ctemp, NULL) - 200.0) / 10.0) * KNOTSTOMETRESSECONDS;

  NMEAParser::ExtractParameter(String,ctemp,10);
  GPS_INFO->MacReady = (_tcstod(ctemp, NULL) / 10.0) * KNOTSTOMETRESSECONDS;
  if (MacCreadyUpdateTimeout <= 0)
    GlidePolar::SetMacCready(GPS_INFO->MacReady);
  else
    MacCreadyUpdateTimeout--;

  NMEAParser::ExtractParameter(String,ctemp,11);
  GPS_INFO->Ballast = _tcstod(ctemp, NULL) / 100.0;
  if (BugsUpdateTimeout <= 0) {
    GlidePolar::SetBallast(GPS_INFO->Ballast);
  } else
    BallastUpdateTimeout--;

  NMEAParser::ExtractParameter(String,ctemp,12);
  GPS_INFO->Bugs = _tcstod(ctemp, NULL) / 100.0;
  if (BugsUpdateTimeout <= 0) {
    GlidePolar::SetBugs(GPS_INFO->Bugs);
  } else
    BugsUpdateTimeout--;

  // JMW update audio functions etc.
  TriggerVarioUpdate();

  return true;
}

