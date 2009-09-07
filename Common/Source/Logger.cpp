/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "Logger.h"
#include "Version.hpp"
#include "Protection.hpp"
#include "Dialogs.h"
#include "Language.hpp"
#include "Device/Port.h"
#include "Settings.hpp"
#include "SettingsComputer.hpp"
#include "SettingsTask.hpp"
#include "Registry.hpp"
#include "Math/Earth.hpp"
#include "LogFile.hpp"
#include "Asset.hpp"
#include "UtilsText.hpp"
#include "UtilsSystem.hpp"
#include "LocalPath.hpp"
#include "Device/device.h"
#include "InputEvents.h"
#include "Compatibility/string.h"

//IGC Logger
bool LoggerActive = false;

TCHAR NumToIGCChar(int n) {
  if (n<10) {
    return _T('1') + (n-1);
  } else {
    return _T('A') + (n-10);
  }
}


int IGCCharToNum(TCHAR c) {
  if ((c >= _T('1')) && (c<= _T('9'))) {
    return c- _T('1') + 1;
  } else if ((c >= _T('A')) && (c<= _T('Z'))) {
    return c- _T('A') + 10;
  } else {
    return 0; // Error!
  }
}


/*

HFDTE141203  <- should be UTC, same as time in filename
HFFXA100
HFPLTPILOT:JOHN WHARINGTON
HFGTYGLIDERTYPE:LS 3
HFGIDGLIDERID:VH-WUE
HFDTM100GPSDATUM:WGS84
HFRFWFIRMWAREVERSION:3.6
HFRHWHARDWAREVERSION:3.4
HFFTYFR TYPE:GARRECHT INGENIEURGESELLSCHAFT,VOLKSLOGGER 1.0
HFCIDCOMPETITIONID:WUE
HFCCLCOMPETITIONCLASS:FAI
HFCIDCOMPETITIONID:WUE
HFCCLCOMPETITIONCLASS:15M
*/



static TCHAR szLoggerFileName[MAX_PATH] = TEXT("\0");
static TCHAR szFLoggerFileName[MAX_PATH] = TEXT("\0");
static TCHAR szFLoggerFileNameRoot[MAX_PATH] = TEXT("\0");
static double FRecordLastTime = 0;
static char szLastFRecord[MAX_IGC_BUFF];

void SetFRecordLastTime(double dTime)
{ FRecordLastTime=dTime; }

double GetFRecordLastTime(void)
{ return FRecordLastTime; }

void ResetFRecord_Internal(void)
{
    for (int iFirst = 0; iFirst < MAX_IGC_BUFF; iFirst++)
      szLastFRecord[iFirst]=0;
}
void ResetFRecord(void)
{
  SetFRecordLastTime(0);
  ResetFRecord_Internal();
}

int EW_count = 0;
int NumLoggerBuffered = 0;
void LoggerGInit();

#define MAX_LOGGER_BUFFER 60

typedef struct LoggerBuffer {
  double Latitude;
  double Longitude;
  double Altitude;
  double BaroAltitude;
  short Day;
  short Month;
  short Year;
  short Hour;
  short Minute;
  short Second;
  int SatelliteIDs[MAXSATELLITES];
} LoggerBuffer_T;

LoggerBuffer_T FirstPoint;
LoggerBuffer_T LoggerBuffer[MAX_LOGGER_BUFFER];

void LoggerGStop(TCHAR*);

void StopLogger(const NMEA_INFO &gps_info) {
  TCHAR szMessage[MAX_PATH] = TEXT("\0");
  int iLoggerError=0;  // see switch statement for error handler
  if (LoggerActive) {
    LoggerActive = false;
    if (LoggerClearFreeSpace(gps_info)) {

#ifndef _SIM_
    if (LoggerGActive())
	{
	  LoggerGStop(szLoggerFileName);
	}

#endif

      int imCount=0;
      const int imMax=3;
      for (imCount=0; imCount < imMax; imCount++) {
        // MoveFile() nonzero==Success
        if (0 != MoveFile( szLoggerFileName, szFLoggerFileName)) {
          iLoggerError=0;
          break; // success
        }
        Sleep(750); // wait for file system cache to fix itself?
      }
      if (imCount == imMax) { // MoveFile() failed all attempts

        if (0 == MoveFile( szLoggerFileName, szFLoggerFileNameRoot)) { // try rename it and leave in root
          iLoggerError=1; //Fail.  NoMoveNoRename
        }
        else {
          iLoggerError=2; //NoMoveYesRename
        }
      }

    }
    else { // Insufficient disk space.  // MoveFile() nonzero==Success
      if (0 == MoveFile( szLoggerFileName, szFLoggerFileNameRoot)) { // try rename it and leave in root
        iLoggerError=3; //Fail.  Insufficient Disk Space, NoRename
      }
      else {
        iLoggerError=4; //Success.  Insufficient Disk Space, YesRename
      }
    }

    switch (iLoggerError) { //0=Success 1=NoMoveNoRename 2=NoMoveYesRename 3=NoSpaceNoRename 4=NoSpaceYesRename
    case 0:
      StartupStore(TEXT("Logger file successfully moved\r\n"));
      break;

    case 1: // NoMoveNoRename
      _tcsncpy(szMessage,TEXT("Logger file not copied.  It is in the root folder of your device and called "),MAX_PATH);
      _tcsncat(szMessage,szLoggerFileName,MAX_PATH);

      MessageBoxX(gettext(szMessage),
		gettext(TEXT("Logger Error")), MB_OK| MB_ICONERROR);
      _tcsncat(szMessage,TEXT("\r\n"),MAX_PATH);
      StartupStore(szMessage);
      break;

    case 2: // NoMoveYesRename
      _tcsncpy(szMessage,TEXT("Logger file not copied.  It is in the root folder of your device"),MAX_PATH);

      MessageBoxX(gettext(szMessage),
		gettext(TEXT("Logger Error")), MB_OK| MB_ICONERROR);
      _tcsncat(szMessage,TEXT("\r\n"),MAX_PATH);
      StartupStore(szMessage);
      break;

    case 3: // Insufficient Storage.  NoRename
      _tcsncpy(szMessage,TEXT("Insuff. storage. Logger file in device's root folder, called "),MAX_PATH);
      _tcsncat(szMessage,szLoggerFileName,MAX_PATH);

      MessageBoxX(gettext(szMessage),
		gettext(TEXT("Logger Error")), MB_OK| MB_ICONERROR);
      _tcsncat(szMessage,TEXT("\r\n"),MAX_PATH);
      StartupStore(szMessage);
      break;

    case 4: // Insufficient Storage.  YesRename
      _tcsncpy(szMessage,TEXT("Insufficient storage.  Logger file is in the root folder of your device"),MAX_PATH);

      MessageBoxX(gettext(szMessage),
		gettext(TEXT("Logger Error")), MB_OK| MB_ICONERROR);
      _tcsncat(szMessage,TEXT("\r\n"),MAX_PATH);
      StartupStore(szMessage);
      break;
} // error handler

    NumLoggerBuffered = 0;
  }
}


static void
LogPointToBuffer(const NMEA_INFO &gps_info)
{
  if (NumLoggerBuffered== MAX_LOGGER_BUFFER) {
    for (int i= 0; i< NumLoggerBuffered-1; i++) {
      LoggerBuffer[i]= LoggerBuffer[i+1];
    }
  } else {
    NumLoggerBuffered++;
  }

  LoggerBuffer[NumLoggerBuffered-1].Latitude = gps_info.Latitude;
  LoggerBuffer[NumLoggerBuffered-1].Longitude = gps_info.Longitude;
  LoggerBuffer[NumLoggerBuffered-1].Altitude = gps_info.Altitude;
  LoggerBuffer[NumLoggerBuffered-1].BaroAltitude = gps_info.BaroAltitude;
  LoggerBuffer[NumLoggerBuffered-1].Hour = gps_info.Hour;
  LoggerBuffer[NumLoggerBuffered-1].Minute = gps_info.Minute;
  LoggerBuffer[NumLoggerBuffered-1].Second = gps_info.Second;
  LoggerBuffer[NumLoggerBuffered-1].Year = gps_info.Year;
  LoggerBuffer[NumLoggerBuffered-1].Month = gps_info.Month;
  LoggerBuffer[NumLoggerBuffered-1].Day = gps_info.Day;

  for (int iSat=0; iSat < MAXSATELLITES; iSat++)
    LoggerBuffer[NumLoggerBuffered-1].SatelliteIDs[iSat]=
      gps_info.SatelliteIDs[iSat];

  // This is the first point that will be output to file.
  // Declaration must happen before this, so must save this time.
  FirstPoint = LoggerBuffer[0];
}


void LogPointToFile(const NMEA_INFO& gps_info)
{
  char szBRecord[500];

  int DegLat, DegLon;
  double MinLat, MinLon;
  char NoS, EoW;

  if ((gps_info.Altitude<0) || (gps_info.BaroAltitude<0)) return;

  DegLat = (int)gps_info.Latitude;
  MinLat = gps_info.Latitude - DegLat;
  NoS = 'N';
  if((MinLat<0) || ((MinLat-DegLat==0) && (DegLat<0)))
    {
      NoS = 'S';
      DegLat *= -1; MinLat *= -1;
    }
  MinLat *= 60;
  MinLat *= 1000;

  DegLon = (int)gps_info.Longitude ;
  MinLon = gps_info.Longitude  - DegLon;
  EoW = 'E';
  if((MinLon<0) || ((MinLon-DegLon==0) && (DegLon<0)))
    {
      EoW = 'W';
      DegLon *= -1; MinLon *= -1;
    }
  MinLon *=60;
  MinLon *= 1000;

  sprintf(szBRecord,"B%02d%02d%02d%02d%05.0f%c%03d%05.0f%cA%05d%05d\r\n",
          gps_info.Hour, gps_info.Minute, gps_info.Second,
          DegLat, MinLat, NoS, DegLon, MinLon, EoW,
          (int)gps_info.BaroAltitude,(int)gps_info.Altitude);

  IGCWriteRecord(szBRecord, szLoggerFileName);
}


void LogPoint(const NMEA_INFO& gps_info) {
  if (!LoggerActive) {
    if (!gps_info.NAVWarning) {
      LogPointToBuffer(gps_info);
    }
  } else if (NumLoggerBuffered) {

    LogFRecordToFile(LoggerBuffer[0].SatelliteIDs,  // write FRec before cached BRecs
                   LoggerBuffer[0].Hour,
                   LoggerBuffer[0].Minute,
                   LoggerBuffer[0].Second,
                   true);

    for (int i=0; i<NumLoggerBuffered; i++) {
      NMEA_INFO tmp_info;
      tmp_info.Latitude = LoggerBuffer[i].Latitude;
      tmp_info.Longitude = LoggerBuffer[i].Longitude;
      tmp_info.Altitude = LoggerBuffer[i].Altitude;
      tmp_info.BaroAltitude = LoggerBuffer[i].BaroAltitude;
      tmp_info.Hour = LoggerBuffer[i].Hour;
      tmp_info.Minute = LoggerBuffer[i].Minute;
      tmp_info.Second = LoggerBuffer[i].Second;
      tmp_info.Year = LoggerBuffer[i].Year;
      tmp_info.Month = LoggerBuffer[i].Month;
      tmp_info.Day = LoggerBuffer[i].Day;

      for (int iSat=0; iSat < MAXSATELLITES; iSat++)
	tmp_info.SatelliteIDs[iSat] = LoggerBuffer[i].SatelliteIDs[iSat];
    
      LogPointToFile(tmp_info);
    }
    NumLoggerBuffered = 0;
  }
  if (LoggerActive) {
    LogPointToFile(gps_info);
  }
}

bool
LogFRecordToFile(const int SatelliteIDs[], short Hour, short Minute,
                 short Second, bool bAlways)
{ // bAlways forces write when completing header for restart
  // only writes record if constallation has changed unless bAlways set

#if !defined(_SIM_)

  char szFRecord[MAX_IGC_BUFF];
  static bool bFirst = true;
  int eof=0;
  int iNumberSatellites=0;
  bool bRetVal = false;

  if (bFirst)
  {
    bFirst = false;
    ResetFRecord_Internal();
  }


  sprintf(szFRecord,"F%02d%02d%02d", Hour, Minute, Second);
  eof=7;

  for (int i=0; i < MAXSATELLITES; i++)
  {
    if (SatelliteIDs[i] > 0)
    {
      sprintf(szFRecord+eof, "%02d",SatelliteIDs[i]);
      eof +=2;
      iNumberSatellites++;
    }
  }
  sprintf(szFRecord+ eof,"\r\n");

  // only write F Record if it has changed since last time
  // check every 4.5 minutes to see if it's changed.  Transient changes are not tracked.
  if (!bAlways
        && strcmp(szFRecord + 7, szLastFRecord + 7) == 0
        && strlen(szFRecord) == strlen(szLastFRecord) )
  { // constellation has not changed
      if (iNumberSatellites >=3)
        bRetVal=true;  // if the last FRecord had 3+ sats, then return true
                      //  and this causes 5-minute counter to reset
      else
        bRetVal=false;  // non-2d fix, don't reset 5-minute counter so
                        // we keep looking for changed constellations
  }
  else
  { // constellation has changed
    if (IGCWriteRecord(szFRecord, szLoggerFileName))
    {
      strcpy(szLastFRecord, szFRecord);
      if (iNumberSatellites >=3)
        bRetVal=true;  // if we log an FRecord with a 3+ sats, then return true
                      //  and this causes 5-minute counter to reset
      else
        bRetVal=false;  // non-2d fix, log it, and don't reset 5-minute counter so
                        // we keep looking for changed constellations
    }
    else
    {  // IGCwrite failed
      bRetVal = false;
    }

  }
  return bRetVal;
#else
  return true;
#endif
}

bool
LogFRecord(const NMEA_INFO &gps_info, bool bAlways)
{
  if (LoggerActive || bAlways)
    {
      return LogFRecordToFile(gps_info.SatelliteIDs,
			      gps_info.Hour, 
			      gps_info.Minute, 
			      gps_info.Second, bAlways);
    }
  else
    return false;  // track whether we succussfully write it, else we retry
}

bool IsAlphaNum (TCHAR c) {
  if (((c >= _T('A'))&&(c <= _T('Z')))
      ||((c >= _T('a'))&&(c <= _T('z')))
      ||((c >= _T('0'))&&(c <= _T('9')))) {
    return true;
  } else {
    return false;
  }
}

void 
StartLogger(const NMEA_INFO &gps_info, 
	    const SETTINGS_COMPUTER &settings,
	    const TCHAR *astrAssetNumber)
{
  HANDLE hFile;
  int i;
  TCHAR path[MAX_PATH];
  TCHAR cAsset[3];
  for (i=0; i < 3; i++) { // chars must be legal in file names
    cAsset[i] = IsAlphaNum(strAssetNumber[i]) ? strAssetNumber[i] : _T('A');
  }

// VENTA3 use logs subdirectory when not in main memory (true for FIVV and PNA)
#if defined(GNAV) || defined(FIVV) || defined(PNA)
  LocalPath(path,TEXT("logs"));
#else
  LocalPath(path);
#endif

  if (TaskModified) {
    SaveDefaultTask();
  }

  _stprintf(szLoggerFileName, TEXT("\\tmp.IGC"));
  DeleteFile(szLoggerFileName);

  LoggerGInit();

  for(i=1;i<99;i++)
    {
      // 2003-12-31-XXX-987-01.IGC
      // long filename form of IGC file.
      // XXX represents manufacturer code

      if (!settings.LoggerShortName) {
        // Long file name
        _stprintf(szFLoggerFileName,
                 TEXT("%s\\%04d-%02d-%02d-XCS-%c%c%c-%02d.IGC"),
                 path,
                 gps_info.Year,
                 gps_info.Month,
                 gps_info.Day,
                 cAsset[0],
                 cAsset[1],
                 cAsset[2],
                 i);

        _stprintf(szFLoggerFileNameRoot,
                 TEXT("%s\\%04d-%02d-%02d-XCS-%c%c%c-%02d.IGC"),
                 TEXT(""), // this creates it in root if MoveFile() fails
                 gps_info.Year,
                 gps_info.Month,
                 gps_info.Day,
                 cAsset[0],
                 cAsset[1],
                 cAsset[2],
                 i);
      } else {
        // Short file name
        TCHAR cyear, cmonth, cday, cflight;
        cyear = NumToIGCChar((int)gps_info.Year % 10);
        cmonth = NumToIGCChar(gps_info.Month);
        cday = NumToIGCChar(gps_info.Day);
        cflight = NumToIGCChar(i);
        _stprintf(szFLoggerFileName,
                 TEXT("%s\\%c%c%cX%c%c%c%c.IGC"),
                 path,
                 cyear,
                 cmonth,
                 cday,
                 cAsset[0],
                 cAsset[1],
                 cAsset[2],
                 cflight);

        _stprintf(szFLoggerFileNameRoot,
                 TEXT("%s\\%c%c%cX%c%c%c%c.IGC"),
                 TEXT(""), // this creates it in root if MoveFile() fails
                 cyear,
                 cmonth,
                 cday,
                 cAsset[0],
                 cAsset[1],
                 cAsset[2],
                 cflight);
      } // end if

      hFile = CreateFile(szFLoggerFileName, GENERIC_WRITE,
			 FILE_SHARE_WRITE, NULL, CREATE_NEW,
			 FILE_ATTRIBUTE_NORMAL, 0);
      if(hFile!=INVALID_HANDLE_VALUE )
	{
          // file already exists
      CloseHandle(hFile);
      DeleteFile(szFLoggerFileName);
      break;
	}
  } // end while

  TCHAR szMessage[MAX_PATH] = TEXT("\0");

  _tcsncpy(szMessage,TEXT("Logger Started: "),MAX_PATH);
  _tcsncat(szMessage,szFLoggerFileName,MAX_PATH);
  _tcsncat(szMessage,TEXT("\r\n"),MAX_PATH);
  StartupStore(szMessage);

  return;
}


void LoggerHeader(const NMEA_INFO &gps_info)
{
  char datum[]= "HFDTM100Datum: WGS-84\r\n";
  char temp[100];
  TCHAR PilotName[100];
  TCHAR AircraftType[100];
  TCHAR AircraftRego[100];

  // Flight recorder ID number MUST go first..
  sprintf(temp,
	  "AXCS%C%C%C\r\n",
	  strAssetNumber[0],
	  strAssetNumber[1],
	  strAssetNumber[2]);
  IGCWriteRecord(temp, szLoggerFileName);

  sprintf(temp,"HFDTE%02d%02d%02d\r\n",
	  gps_info.Day,
	  gps_info.Month,
	  gps_info.Year % 100);
  IGCWriteRecord(temp, szLoggerFileName);

  GetRegistryString(szRegistryPilotName, PilotName, 100);
  sprintf(temp,"HFPLTPILOT:%S\r\n", PilotName);
  IGCWriteRecord(temp, szLoggerFileName);

  GetRegistryString(szRegistryAircraftType, AircraftType, 100);
  sprintf(temp,"HFGTYGLIDERTYPE:%S\r\n", AircraftType);
  IGCWriteRecord(temp, szLoggerFileName);

  GetRegistryString(szRegistryAircraftRego, AircraftRego, 100);
  sprintf(temp,"HFGIDGLIDERID:%S\r\n", AircraftRego);
  IGCWriteRecord(temp, szLoggerFileName);

  sprintf(temp,"HFFTYFR TYPE:XCSOAR,XCSOAR %S\r\n", XCSoar_Version);
  IGCWriteRecord(temp, szLoggerFileName);

  IGCWriteRecord(datum, szLoggerFileName);

}


void StartDeclaration(const NMEA_INFO &gps_info,
		      int ntp)
{
  // TODO bug: this is causing problems with some analysis software
  // maybe it's because the date and location fields are bogus
  char start[] = "C0000000N00000000ETAKEOFF\r\n";
  char temp[100];

  if (NumLoggerBuffered==0) {
    FirstPoint.Year = gps_info.Year;
    FirstPoint.Month = gps_info.Month;
    FirstPoint.Day = gps_info.Day;
    FirstPoint.Hour = gps_info.Hour;
    FirstPoint.Minute = gps_info.Minute;
    FirstPoint.Second = gps_info.Second;
  }

  // JMW added task start declaration line

  // LGCSTKF013945TAKEOFF DETECTED

  // IGC GNSS specification 3.6.1
  sprintf(temp,
	  "C%02d%02d%02d%02d%02d%02d0000000000%02d\r\n",
	  // DD  MM  YY  HH  MM  SS  DD  MM  YY IIII TT
	  FirstPoint.Day,
	  FirstPoint.Month,
	  FirstPoint.Year % 100,
	  FirstPoint.Hour,
	  FirstPoint.Minute,
	  FirstPoint.Second,
	  ntp-2);

  IGCWriteRecord(temp, szLoggerFileName);
  // takeoff line
  // IGC GNSS specification 3.6.3
  IGCWriteRecord(start, szLoggerFileName);

}


void EndDeclaration(void)
{
  // TODO bug: this is causing problems with some analysis software
  // maybe it's because the date and location fields are bogus
  const char start[] = "C0000000N00000000ELANDING\r\n";
  IGCWriteRecord(start, szLoggerFileName);
}

void AddDeclaration(double Latitude, double Longitude, const TCHAR *ID)
{
  char szCRecord[500];

  char IDString[MAX_PATH];
  int i;

  int DegLat, DegLon;
  double MinLat, MinLon;
  char NoS, EoW;

  TCHAR tmpstring[MAX_PATH];
  _tcscpy(tmpstring, ID);
  _tcsupr(tmpstring);
  for(i=0;i<(int)_tcslen(tmpstring);i++)
    {
      IDString[i] = (char)tmpstring[i];
    }
  IDString[i] = '\0';

  DegLat = (int)Latitude;
  MinLat = Latitude - DegLat;
  NoS = 'N';
  if((MinLat<0) || ((MinLat-DegLat==0) && (DegLat<0)))
    {
      NoS = 'S';
      DegLat *= -1; MinLat *= -1;
    }
  MinLat *= 60;
  MinLat *= 1000;

  DegLon = (int)Longitude ;
  MinLon = Longitude  - DegLon;
  EoW = 'E';
  if((MinLon<0) || ((MinLon-DegLon==0) && (DegLon<0)))
    {
      EoW = 'W';
      DegLon *= -1; MinLon *= -1;
    }
  MinLon *=60;
  MinLon *= 1000;

  sprintf(szCRecord,"C%02d%05.0f%c%03d%05.0f%c%s\r\n",
	  DegLat, MinLat, NoS, DegLon, MinLon, EoW, IDString);

  IGCWriteRecord(szCRecord, szLoggerFileName);
}


// TODO code: make this thread-safe, since it could happen in the middle
// of the calculations doing LogPoint or something else!

void LoggerNote(const TCHAR *text) {
  if (LoggerActive) {
    char fulltext[500];
    sprintf(fulltext, "LPLT%S\r\n", text);
    IGCWriteRecord(fulltext, szLoggerFileName);
  }
}

bool DeclaredToDevice = false;


static bool LoggerDeclare(PDeviceDescriptor_t dev, Declaration_t *decl)
{
  if (!devIsLogger(dev))
    return FALSE;

  if (MessageBoxX(gettext(TEXT("Declare Task?")),
                  dev->Name, MB_YESNO| MB_ICONQUESTION) == IDYES) {
    if (devDeclare(dev, decl)) {
      MessageBoxX(gettext(TEXT("Task Declared!")),
                  dev->Name, MB_OK| MB_ICONINFORMATION);
      DeclaredToDevice = true;
    } else {
      MessageBoxX(gettext(TEXT("Error occured,\r\nTask NOT Declared!")),
                  dev->Name, MB_OK| MB_ICONERROR);
      DeclaredToDevice = false;
    }
  }
  return TRUE;
}

void LoggerDeviceDeclare() {
  bool found_logger = false;
  Declaration_t Decl;
  int i;

  GetRegistryString(szRegistryPilotName, Decl.PilotName, 64);
  GetRegistryString(szRegistryAircraftType, Decl.AircraftType, 32);
  GetRegistryString(szRegistryAircraftRego, Decl.AircraftRego, 32);

  for (i = 0; i < MAXTASKPOINTS; i++) {
    if (Task[i].Index == -1)
      break;
    Decl.waypoint[i] = &WayPointList[Task[i].Index];
  }
  Decl.num_waypoints = i;

  DeclaredToDevice = false;

  if (LoggerDeclare(devA(), &Decl))
    found_logger = true;

  if (LoggerDeclare(devB(), &Decl))
    found_logger = true;

  if (!found_logger) {
    MessageBoxX(gettext(TEXT("No logger connected")),
		devB()->Name, MB_OK| MB_ICONINFORMATION);
    DeclaredToDevice = true; // testing only
  }

}


bool CheckDeclaration(void) {
  if (!DeclaredToDevice) {
    return true;
  } else {
    if(MessageBoxX(gettext(TEXT("OK to invalidate declaration?")),
		   gettext(TEXT("Task declared")),
		   MB_YESNO| MB_ICONQUESTION) == IDYES){
      DeclaredToDevice = false;
      return true;
    } else {
      return false;
    }
  }
}


//////


FILETIME LogFileDate(const NMEA_INFO &gps_info,
		     TCHAR* filename) {
  FILETIME ft;
  ft.dwLowDateTime = 0;
  ft.dwHighDateTime = 0;

  TCHAR asset[MAX_PATH];
  SYSTEMTIME st;
  unsigned short year, month, day, num;
  int matches;
  // scan for long filename
  matches = _stscanf(filename,
                    TEXT("%hu-%hu-%hu-%7s-%hu.IGC"),
                    &year,
                    &month,
                    &day,
                    asset,
                    &num);
  if (matches==5) {
    st.wYear = year;
    st.wMonth = month;
    st.wDay = day;
    st.wHour = num;
    st.wMinute = 0;
    st.wSecond = 0;
    st.wMilliseconds = 0;
    SystemTimeToFileTime(&st,&ft);
    return ft;
  }

  TCHAR cyear, cmonth, cday, cflight;
  // scan for short filename
  matches = _stscanf(filename,
		     TEXT("%c%c%c%4s%c.IGC"),
		     &cyear,
		     &cmonth,
		     &cday,
		     asset,
		     &cflight);
  if (matches==5) {
    int iyear = (int)gps_info.Year;
    int syear = iyear % 10;
    int yearzero = iyear - syear;
    int yearthis = IGCCharToNum(cyear) + yearzero;
    if (yearthis > iyear) {
      yearthis -= 10;
    }
    st.wYear = yearthis;
    st.wMonth = IGCCharToNum(cmonth);
    st.wDay = IGCCharToNum(cday);
    st.wHour = IGCCharToNum(cflight);
    st.wMinute = 0;
    st.wSecond = 0;
    st.wMilliseconds = 0;
    SystemTimeToFileTime(&st,&ft);
    return ft;
    /*
      YMDCXXXF.IGC
      Y: Year, 0 to 9 cycling every 10 years
      M: Month, 1 to 9 then A for 10, B=11, C=12
      D: Day, 1 to 9 then A for 10, B=....
      C: Manuf. code = X
      XXX: Logger ID Alphanum
      F: Flight of day, 1 to 9 then A through Z
    */
  }
  return ft;
}


bool LogFileIsOlder(const NMEA_INFO &gps_info,
		    TCHAR *oldestname, TCHAR *thisname) {
  FILETIME ftold = LogFileDate(gps_info, oldestname);
  FILETIME ftnew = LogFileDate(gps_info, thisname);
  return (CompareFileTime(&ftold, &ftnew)>0);
}


bool DeleteOldIGCFile(const NMEA_INFO &gps_info,
		     TCHAR *pathname) {
  HANDLE hFind;  // file handle
  WIN32_FIND_DATA FindFileData;
  TCHAR oldestname[MAX_PATH];
  TCHAR searchpath[MAX_PATH];
  TCHAR fullname[MAX_PATH];
  _stprintf(searchpath, TEXT("%s*"),pathname);

  hFind = FindFirstFile(searchpath, &FindFileData); // find the first file
  if(hFind == INVALID_HANDLE_VALUE) {
    return false;
  }
  if(!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
    if (MatchesExtension(FindFileData.cFileName, TEXT(".igc")) ||
	MatchesExtension(FindFileData.cFileName, TEXT(".IGC"))) {
      // do something...
      _tcscpy(oldestname, FindFileData.cFileName);
    } else {
      return false;
    }
  }
  bool bSearch = true;
  while(bSearch) { // until we finds an entry
    if(FindNextFile(hFind,&FindFileData)) {
      if(!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
	 (MatchesExtension(FindFileData.cFileName, TEXT(".igc")) ||
	  (MatchesExtension(FindFileData.cFileName, TEXT(".IGC"))))) {
	if (LogFileIsOlder(gps_info,oldestname,FindFileData.cFileName)) {
	  _tcscpy(oldestname, FindFileData.cFileName);
	  // we have a new oldest name
	}
      }
    } else {
      bSearch = false;
    }
  }
  FindClose(hFind);  // closing file handle

  // now, delete the file...
  _stprintf(fullname, TEXT("%s%s"),pathname,oldestname);
  DeleteFile(fullname);
  return true; // did delete one
}


#define LOGGER_MINFREESTORAGE (250+MINFREESTORAGE)
// JMW note: we want to clear up enough space to save the persistent
// data (85 kb approx) and a new log file

#ifdef DEBUG_IGCFILENAME
TCHAR testtext1[] = TEXT("2007-11-05-XXX-AAA-01.IGC");
TCHAR testtext2[] = TEXT("2007-11-05-XXX-AAA-02.IGC");
TCHAR testtext3[] = TEXT("3BOA1VX2.IGC");
TCHAR testtext4[] = TEXT("5BDX7B31.IGC");
TCHAR testtext5[] = TEXT("3BOA1VX2.IGC");
TCHAR testtext6[] = TEXT("9BDX7B31.IGC");
TCHAR testtext7[] = TEXT("2008-01-05-XXX-AAA-01.IGC");
#endif

bool LoggerClearFreeSpace(const NMEA_INFO &gps_info) {
  bool found = true;
  unsigned long kbfree=0;
  TCHAR pathname[MAX_PATH];
  TCHAR subpathname[MAX_PATH];
  int numtries = 0;

  LocalPath(pathname);
#ifdef GNAV
  LocalPath(subpathname,TEXT("logs"));
#else
  LocalPath(subpathname);
#endif

#ifdef DEBUG_IGCFILENAME
  bool retval;
  retval = LogFileIsOlder(testtext1,
                          testtext2);
  retval = LogFileIsOlder(testtext1,
                          testtext3);
  retval = LogFileIsOlder(testtext4,
                          testtext5);
  retval = LogFileIsOlder(testtext6,
                          testtext7);
#endif

  while (found && ((kbfree = FindFreeSpace(pathname))<LOGGER_MINFREESTORAGE)
	 && (numtries++ <100)) {
    /* JMW asking for deleting old files is disabled now --- system
       automatically deletes old files as required
    */

    // search for IGC files, and delete the oldest one
    found = DeleteOldIGCFile(gps_info, pathname);
    if (!found) {
      found = DeleteOldIGCFile(gps_info, subpathname);
    }
  }
  if (kbfree>=LOGGER_MINFREESTORAGE) {
    StartupStore(TEXT("LoggerFreeSpace returned: true\r\n"));
    return true;
  } else {
    StartupStore(TEXT("LoggerFreeSpace returned: false\r\n"));
    return false;
  }
}




#include "Interface.hpp"
#include "ReplayLogger.hpp"

void guiStartLogger(const NMEA_INFO& gps_info, 
		    const SETTINGS_COMPUTER& settings,
		    bool noAsk) {
  int i;
  if (!LoggerActive) {
    if (ReplayLogger::IsEnabled()) {
      if (LoggerActive)
        guiStopLogger(gps_info, true);
      return;
    }
    TCHAR TaskMessage[1024];
    _tcscpy(TaskMessage,TEXT("Start Logger With Declaration\r\n"));
    for(i=0;i<MAXTASKPOINTS;i++)
      {
	if(Task[i].Index == -1)
	  {
	    if(i==0)
	      _tcscat(TaskMessage,TEXT("None"));

	    break;
	  }
	_tcscat(TaskMessage,WayPointList[ Task[i].Index ].Name);
	_tcscat(TaskMessage,TEXT("\r\n"));
      }

    if(noAsk ||
       (MessageBoxX(TaskMessage,gettext(TEXT("Start Logger")),
		    MB_YESNO|MB_ICONQUESTION) == IDYES))
      {

	if (LoggerClearFreeSpace(gps_info)) {

	  StartLogger(gps_info, settings, strAssetNumber);
	  LoggerHeader(gps_info);
	  LoggerActive = true; // start logger after Header is completed.  Concurrency

	  int ntp=0;
	  for(i=0;i<MAXTASKPOINTS;i++)
	    {
	      if(Task[i].Index == -1) {
		break;
	      }
	      ntp++;
	    }
	  StartDeclaration(gps_info,ntp);
	  for(i=0;i<MAXTASKPOINTS;i++)
	    {
	      if(Task[i].Index == -1) {
		break;
	      }
	      AddDeclaration(WayPointList[Task[i].Index].Latitude,
			     WayPointList[Task[i].Index].Longitude,
			     WayPointList[Task[i].Index].Name );
	    }
	  EndDeclaration();
	  ResetFRecord(); // reset timer & lastRecord string so if
			  // logger is restarted, FRec appears at top
			  // of file
	} else {

	  MessageBoxX(
		      gettext(TEXT("Logger inactive, insufficient storage!")),
		      gettext(TEXT("Logger Error")), MB_OK| MB_ICONERROR);
	  StartupStore(TEXT("Logger not started: Insufficient Storage\r\n"));
	}
      }
  }
}


void guiStopLogger(const NMEA_INFO& gps_info, 
		   bool noAsk) {
  if (LoggerActive) {
    if(noAsk ||
       (MessageBoxX(gettext(TEXT("Stop Logger")),
		    gettext(TEXT("Stop Logger")),
		    MB_YESNO|MB_ICONQUESTION) == IDYES)) {
      StopLogger(gps_info);
    }
  }
}


void guiToggleLogger(const NMEA_INFO& gps_info, 
		     const SETTINGS_COMPUTER& settings,
		     bool noAsk) {
  if (LoggerActive) {
    guiStopLogger(gps_info, noAsk);
  } else {
    guiStartLogger(gps_info, settings, noAsk);
  }
}

