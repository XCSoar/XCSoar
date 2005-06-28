// $Id$

#include <windows.h>
#include <tchar.h>


#include "externs.h"
#include "utils.h"
#include "parser.h"
#include "port.h"

#include "devCai302.h"

#define  CtrlC  0x03

// Additional sentance for CAI302 support
static BOOL cai_w(TCHAR *String, NMEA_INFO *GPS_INFO);
static BOOL cai_PCAIB(TCHAR *String, NMEA_INFO *GPS_INFO);
static cai_PCAID(TCHAR *String, NMEA_INFO *GPS_INFO);

static int McReadyUpdateTimeout = 0;
static int BugsUpdateTimeout = 0;
static int BallastUpdateTimeout = 0;



BOOL cai302ParseNMEA(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *GPS_INFO){

  if (!NMEAChecksum(String) || (GPS_INFO == NULL)){
    return FALSE;
  }

  if(_tcsstr(String,TEXT("$PCAIB")) == String){
    return cai_PCAIB(&String[7], GPS_INFO);
  }

  if(_tcsstr(String,TEXT("$PCAID")) == String){
    return cai_PCAID(&String[7], GPS_INFO);
  }

  if(_tcsstr(String,TEXT("!w")) == String){
    return cai_w(&String[3], GPS_INFO);
  }

  return FALSE;

}


BOOL cai302PutMcReady(PDeviceDescriptor_t d, double McReady){

  TCHAR  szTmp[32];

  _stprintf(szTmp, TEXT("!g,m%d\r\n"), int(((McReady * 10) / KNOTSTOMETRESSECONDS) + 0.5));

  Port1WriteString(szTmp);

  McReadyUpdateTimeout = 2;

  return(TRUE);

}


BOOL cai302PutBugs(PDeviceDescriptor_t d, double Bugs){

  TCHAR  szTmp[32];

  _stprintf(szTmp, TEXT("!g,u%d\r\n"), int((Bugs * 100) + 0.5));

  Port1WriteString(szTmp);

  BugsUpdateTimeout = 2;

  return(TRUE);

}


BOOL cai302PutBallast(PDeviceDescriptor_t d, double Ballast){

  TCHAR  szTmp[32];

  _stprintf(szTmp, TEXT("!g,b%d\r\n"), int((Ballast * 10) + 0.5));

  Port1WriteString(szTmp);

  BallastUpdateTimeout = 2;

  return(TRUE);

}

BOOL cai302Open(PDeviceDescriptor_t d, int Port){

  d->Port = Port;

  TCHAR  szTmp[32];

  _stprintf(szTmp, TEXT("%cLOG %d\r\n"), CtrlC, 0);

  Port1WriteString(szTmp);
  return(TRUE);
}


static int DeclIndex = 128;
static int DeclInProgress = 0;

BOOL cai302DeclBegin(PDeviceDescriptor_t d, TCHAR *PilotsName, TCHAR *Class, TCHAR *ID){

  TCHAR  szTmp[32];

  _stprintf(szTmp, TEXT("%cDOW %d\r\n"), CtrlC, 1 /* no echo */);

  Port1WriteString(szTmp);

  DeclIndex = 128;
  DeclInProgress = 1;

  return(TRUE);

}


BOOL cai302DeclEnd(PDeviceDescriptor_t d){

  TCHAR  szTmp[32];

  _stprintf(szTmp, TEXT("D,%d\r\n"), 255 /* end of declaration */);
  Port1WriteString(szTmp);

  DeclInProgress = 0;
  _stprintf(szTmp, TEXT("%cLOG %d\r\n"), CtrlC, 0 /* nmea mode */);
  Port1WriteString(szTmp);

  return(TRUE);

}


BOOL cai302DeclAddWayPoint(PDeviceDescriptor_t d, WAYPOINT *wp){

  TCHAR Name[13];
  TCHAR  szTmp[128];
  int DegLat, DegLon;
  double MinLat, MinLon;
  char NoS, EoW;

  _tcsncpy(Name, wp->Name, 12);
  Name[12] = '\0';

  DegLat = (int)wp->Lattitude;
  MinLat = wp->Lattitude - DegLat;
  NoS = 'N';
  if(MinLat<0)
    {
      NoS = 'S';
      DegLat *= -1;
      MinLat *= -1;
    }
  MinLat *= 60;


  DegLon = (int)wp->Longditude ;
  MinLon = wp->Longditude  - DegLon;
  EoW = 'E';
  if(MinLon<0)
    {
      EoW = 'W';
      DegLon *= -1;
      MinLon *= -1;
    }
  MinLon *=60;

  _stprintf(szTmp, TEXT("D,%d,%02d%07.4f%c,%03d%07.4f%c,%s,%d\r\n"),
    DeclIndex,
    DegLat, MinLat, NoS,
    DegLon, MinLon, EoW,
    Name,
    (int)wp->Altitude
  );

  Port1WriteString(szTmp);

  DeclIndex++;

  return(TRUE);

}


BOOL cai302IsLogger(PDeviceDescriptor_t d){
  return(TRUE);
}


BOOL cai302IsGPSSource(PDeviceDescriptor_t d){
  return(TRUE);
}


BOOL cai302Install(PDeviceDescriptor_t d){
  _tcscpy(d->Name, TEXT("CAI 302"));
  d->ParseNMEA = cai302ParseNMEA;
  d->PutMcReady = cai302PutMcReady;
  d->PutBugs = cai302PutBugs;
  d->PutBallast = cai302PutBallast;
  d->Open = cai302Open;
  d->Close = NULL;
  d->Init = NULL;
  d->LinkTimeout = NULL;
  d->DeclBegin = cai302DeclBegin;
  d->DeclEnd = cai302DeclEnd;
  d->DeclAddWayPoint = cai302DeclAddWayPoint;
  d->IsLogger = cai302IsLogger;
  d->IsGPSSource = cai302IsGPSSource;

  return(TRUE);

}


BOOL cai302Register(void){
  return(devRegister(TEXT("CAI 302"),
      1l << dfGPS
    | 1l << dfLogger
    | 1l << dfSpeed
    | 1l << dfVario
    | 1l << dfBaroAlt
    | 1l << dfWind
  ));
}

// local stuff

/*
$PCAIB,<1>,<2>,<CR><LF>
<1> Destination Navpoint elevation in meters, format XXXXX (leading zeros will be transmitted)
<2> Destination Navpoint attribute word, format XXXXX (leading zeros will be transmitted)
*/

BOOL cai_PCAIB(TCHAR *String, NMEA_INFO *GPS_INFO){
  return TRUE;
}


/*
$PCAID,<1>,<2>,<3>,<4>*hh<CR><LF>
<1> Logged 'L' Last point Logged 'N' Last Point not logged
<2> Barometer Altitude in meters (Leading zeros will be transmitted)
<3> Engine Noise Level
<4> Log Flags
*hh Checksum, XOR of all bytes of the sentence after the ‘$’ and before the ‘*’
*/

BOOL cai_PCAID(TCHAR *String, NMEA_INFO *GPS_INFO){
  return TRUE;
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

BOOL cai_w(TCHAR *String, NMEA_INFO *GPS_INFO){

  TCHAR ctemp[80];


  ExtractParameter(String,ctemp,1);
  GPS_INFO->ExternalWindAvailalbe = TRUE;
  GPS_INFO->ExternalWindSpeed = (StrToDouble(ctemp,NULL) / 10.0);
  ExtractParameter(String,ctemp,0);
  GPS_INFO->ExternalWindDirection = StrToDouble(ctemp,NULL);


  ExtractParameter(String,ctemp,4);
  GPS_INFO->BaroAltitudeAvailable = TRUE;
  GPS_INFO->BaroAltitude = StrToDouble(ctemp, NULL) - 1000;

//  ExtractParameter(String,ctemp,5);
//  GPS_INFO->QNH = StrToDouble(ctemp, NULL) - 1000;

  ExtractParameter(String,ctemp,6);
  GPS_INFO->AirspeedAvailable = TRUE;
  GPS_INFO->Airspeed = (StrToDouble(ctemp,NULL) / 100.0);

  ExtractParameter(String,ctemp,7);
  GPS_INFO->VarioAvailable = TRUE;
  GPS_INFO->Vario = ((StrToDouble(ctemp,NULL) - 200.0) / 10.0) * KNOTSTOMETRESSECONDS;


  ExtractParameter(String,ctemp,10);
  GPS_INFO->MacReady = (StrToDouble(ctemp,NULL) / 10.0) * KNOTSTOMETRESSECONDS;
  if (McReadyUpdateTimeout <= 0)
    MACREADY = GPS_INFO->MacReady;
  else
    McReadyUpdateTimeout--;


  ExtractParameter(String,ctemp,11);
  GPS_INFO->Ballast = StrToDouble(ctemp,NULL) / 100.0;
  if (BugsUpdateTimeout <= 0)
    BALLAST = GPS_INFO->Ballast;
  else
    BallastUpdateTimeout--;


  ExtractParameter(String,ctemp,12);
  GPS_INFO->Bugs = StrToDouble(ctemp,NULL) / 100.0;
  if (BugsUpdateTimeout <= 0)
    BUGS = GPS_INFO->Bugs;
  else
    BugsUpdateTimeout--;

  return TRUE;

}

