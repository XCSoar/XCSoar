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

#include "LoggerImpl.hpp"
#include "Version.hpp"
#include "Dialogs/Message.hpp"
#include "Language.hpp"
#include "Task.h"
#include "Registry.hpp"
#include "LogFile.hpp"
#include "Asset.hpp"
#include "UtilsText.hpp"
#include "UtilsSystem.hpp"
#include "LocalPath.hpp"
#include "Device/device.h"
#include "Compatibility/string.h"
#include "SettingsComputer.hpp"
#include "NMEA/Info.h"

LoggerImpl::LoggerImpl():
  LoggerActive(false),
  DeclaredToDevice(false),
  NumLoggerPreTakeoffBuffered(0),
  LoggerDiskBufferCount(0),
  frecord_clock(270.0) // 4.5 minutes)
{
  ResetFRecord();
  szLoggerFileName[0] = 0;
}

bool
LoggerImpl::isLoggerActive() const
{
  return LoggerActive;
}

TCHAR
NumToIGCChar(int n)
{
  if (n < 10) {
    return _T('1') + (n - 1);
  } else {
    return _T('A') + (n - 10);
  }
}

/**
 * Returns whether a task is declared to the device
 * @return True if a task is declared to the device, False otherwise
 */
bool
LoggerImpl::isTaskDeclared() const
{
  return DeclaredToDevice;
}

int
IGCCharToNum(TCHAR c)
{
  if ((c >= _T('1')) && (c <= _T('9'))) {
    return c - _T('1') + 1;
  } else if ((c >= _T('A')) && (c <= _T('Z'))) {
    return c - _T('A') + 10;
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

void
LoggerImpl::StopLogger(const NMEA_INFO &gps_info)
{
  if (LoggerActive) {
    LoggerActive = false;

    if (LoggerClearFreeSpace(gps_info)) {
      WriteLock();
      DiskBufferFlush();
      Unlock();

      if (!is_simulator() && LoggerGActive())
        LoggerGStop(szLoggerFileName);

      NumLoggerPreTakeoffBuffered = 0;
    }
  }
}

void
LoggerImpl::LogPointToBuffer(const NMEA_INFO &gps_info)
{
  if (NumLoggerPreTakeoffBuffered== LOGGER_PRETAKEOFF_BUFFER_MAX) {
    for (int i= 0; i< NumLoggerPreTakeoffBuffered-1; i++) {
      LoggerPreTakeoffBuffer[i]= LoggerPreTakeoffBuffer[i+1];
    }
  } else {
    NumLoggerPreTakeoffBuffered++;
  }

  LoggerPreTakeoffBuffer[NumLoggerPreTakeoffBuffered-1].Latitude =
      gps_info.Location.Latitude;
  LoggerPreTakeoffBuffer[NumLoggerPreTakeoffBuffered-1].Longitude =
      gps_info.Location.Longitude;

  LoggerPreTakeoffBuffer[NumLoggerPreTakeoffBuffered-1].Altitude =
      gps_info.Altitude;
  LoggerPreTakeoffBuffer[NumLoggerPreTakeoffBuffered-1].BaroAltitude =
      gps_info.BaroAltitude;

  if (!gps_info.BaroAltitudeAvailable) {
    LoggerPreTakeoffBuffer[NumLoggerPreTakeoffBuffered-1].BaroAltitude =
        gps_info.Altitude;
  }

  LoggerPreTakeoffBuffer[NumLoggerPreTakeoffBuffered-1].Hour =
      gps_info.Hour;
  LoggerPreTakeoffBuffer[NumLoggerPreTakeoffBuffered-1].Minute =
      gps_info.Minute;
  LoggerPreTakeoffBuffer[NumLoggerPreTakeoffBuffered-1].Second =
      gps_info.Second;
  LoggerPreTakeoffBuffer[NumLoggerPreTakeoffBuffered-1].Year =
      gps_info.Year;
  LoggerPreTakeoffBuffer[NumLoggerPreTakeoffBuffered-1].Month =
      gps_info.Month;
  LoggerPreTakeoffBuffer[NumLoggerPreTakeoffBuffered-1].Day =
      gps_info.Day;
  LoggerPreTakeoffBuffer[NumLoggerPreTakeoffBuffered-1].Time =
      gps_info.Time;

  LoggerPreTakeoffBuffer[NumLoggerPreTakeoffBuffered-1].NAVWarning =
      gps_info.NAVWarning;

  for (int iSat = 0; iSat < MAXSATELLITES; iSat++)
    LoggerPreTakeoffBuffer[NumLoggerPreTakeoffBuffered-1].SatelliteIDs[iSat] =
      gps_info.SatelliteIDs[iSat];

  // This is the first point that will be output to file.
  // Declaration must happen before this, so must save this time.
  FirstPoint = LoggerPreTakeoffBuffer[0];
}


void
LoggerImpl::LogPointToFile(const NMEA_INFO& gps_info)
{
  char szBRecord[500];

  int DegLat, DegLon;
  double MinLat, MinLon;
  char NoS, EoW;

  LogFRecordToFile(gps_info.SatelliteIDs, gps_info.Hour, gps_info.Minute,
      gps_info.Second, gps_info.Time, gps_info.NAVWarning);

  if ((gps_info.Altitude < -100) || (gps_info.BaroAltitude < -100)
      || gps_info.NAVWarning) {
    return;
  }

  DegLat = (int)gps_info.Location.Latitude;
  MinLat = gps_info.Location.Latitude - DegLat;
  NoS = 'N';
  if ((MinLat < 0) || ((MinLat - DegLat == 0) && (DegLat < 0))) {
    NoS = 'S';
    DegLat *= -1;
    MinLat *= -1;
  }
  MinLat *= 60;
  MinLat *= 1000;

  DegLon = (int)gps_info.Location.Longitude;
  MinLon = gps_info.Location.Longitude - DegLon;
  EoW = 'E';
  if ((MinLon < 0) || ((MinLon - DegLon == 0) && (DegLon < 0))) {
    EoW = 'W';
    DegLon *= -1;
    MinLon *= -1;
  }
  MinLon *= 60;
  MinLon *= 1000;

  sprintf(szBRecord,"B%02d%02d%02d%02d%05.0f%c%03d%05.0f%cA%05d%05d\r\n",
          gps_info.Hour, gps_info.Minute, gps_info.Second,
          DegLat, MinLat, NoS, DegLon, MinLon, EoW,
          (int)gps_info.BaroAltitude,(int)gps_info.Altitude);

  IGCWriteRecord(szBRecord, szLoggerFileName);
}

void
LoggerImpl::LogPoint(const NMEA_INFO& gps_info)
{
  if (!LoggerActive) {
    LogPointToBuffer(gps_info);
  } else if (NumLoggerPreTakeoffBuffered) {
    for (int i = 0; i < NumLoggerPreTakeoffBuffered; i++) {
      NMEA_INFO tmp_info;
      tmp_info.Location.Latitude = LoggerPreTakeoffBuffer[i].Latitude;
      tmp_info.Location.Longitude = LoggerPreTakeoffBuffer[i].Longitude;
      tmp_info.Altitude = LoggerPreTakeoffBuffer[i].Altitude;
      tmp_info.BaroAltitude = LoggerPreTakeoffBuffer[i].BaroAltitude;
      tmp_info.Hour = LoggerPreTakeoffBuffer[i].Hour;
      tmp_info.Minute = LoggerPreTakeoffBuffer[i].Minute;
      tmp_info.Second = LoggerPreTakeoffBuffer[i].Second;
      tmp_info.Year = LoggerPreTakeoffBuffer[i].Year;
      tmp_info.Month = LoggerPreTakeoffBuffer[i].Month;
      tmp_info.Day = LoggerPreTakeoffBuffer[i].Day;
      tmp_info.Time = LoggerPreTakeoffBuffer[i].Time;
      tmp_info.NAVWarning = LoggerPreTakeoffBuffer[i].NAVWarning;

      for (int iSat = 0; iSat < MAXSATELLITES; iSat++)
        tmp_info.SatelliteIDs[iSat] = LoggerPreTakeoffBuffer[i].SatelliteIDs[iSat];

      LogPointToFile(tmp_info);
    }

    NumLoggerPreTakeoffBuffered = 0;
  }

  if (LoggerActive) {
    LogPointToFile(gps_info);
  }
}


bool
IsAlphaNum (TCHAR c)
{
  if (((c >= _T('A')) && (c <= _T('Z')))
      || ((c >= _T('a')) && (c <= _T('z')))
      || ((c >= _T('0')) && (c <= _T('9')))) {
    return true;
  } else {
    return false;
  }
}

void
LoggerImpl::StartLogger(const NMEA_INFO &gps_info,
    const SETTINGS_COMPUTER &settings, const TCHAR *astrAssetNumber)
{
  HANDLE hFile;
  int i;
  TCHAR path[MAX_PATH];

  for (i = 0; i < 3; i++) { // chars must be legal in file names
    strAssetNumber[i] = IsAlphaNum(strAssetNumber[i]) ? strAssetNumber[i] : _T('A');
  }

  // VENTA3 use logs subdirectory when not in main memory (true for FIVV and PNA)
  #if defined(GNAV) || defined(FIVV) || defined(PNA)
    LocalPath(path,TEXT("logs"));
  #else
    LocalPath(path);
  #endif

  if (task.isTaskModified()) {
    task.SaveDefaultTask();
  }

  WriteLock();
  DiskBufferReset();
  Unlock();

  LoggerGInit();
  ResetFRecord();

  for (i = 1; i < 99; i++) {
    // 2003-12-31-XXX-987-01.IGC
    // long filename form of IGC file.
    // XXX represents manufacturer code

    if (!settings.LoggerShortName) {
      // Long file name
      _stprintf(szLoggerFileName,
          TEXT("%s\\%04d-%02d-%02d-XCS-%c%c%c-%02d.IGC"),
          path,
          gps_info.Year,
          gps_info.Month,
          gps_info.Day,
          strAssetNumber[0],
          strAssetNumber[1],
          strAssetNumber[2],
          i);
    } else {
      // Short file name
      TCHAR cyear, cmonth, cday, cflight;
      cyear = NumToIGCChar((int)gps_info.Year % 10);
      cmonth = NumToIGCChar(gps_info.Month);
      cday = NumToIGCChar(gps_info.Day);
      cflight = NumToIGCChar(i);
      _stprintf(szLoggerFileName,
          TEXT("%s\\%c%c%cX%c%c%c%c.IGC"),
          path,
          cyear,
          cmonth,
          cday,
          strAssetNumber[0],
          strAssetNumber[1],
          strAssetNumber[2],
          cflight);

    }

    hFile = CreateFile(szLoggerFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile != INVALID_HANDLE_VALUE) {
      // file did not already exist, and could be created
      CloseHandle(hFile);
      DeleteFile(szLoggerFileName);
      break;
    }
  }

  TCHAR szMessage[MAX_PATH] = TEXT("\0");

  _tcsncpy(szMessage, TEXT("Logger Started: "), MAX_PATH);
  _tcsncat(szMessage, szLoggerFileName, MAX_PATH);
  _tcsncat(szMessage, TEXT("\r\n"), MAX_PATH);
  StartupStore(szMessage);

  return;
}

void
LoggerImpl::LoggerHeader(const NMEA_INFO &gps_info)
{
  char datum[] = "HFDTM100Datum: WGS-84\r\n";
  char temp[100];
  TCHAR PilotName[100];
  TCHAR AircraftType[100];
  TCHAR AircraftRego[100];

  // Flight recorder ID number MUST go first..
  sprintf(temp, "AXCS%C%C%C\r\n",
      strAssetNumber[0],
      strAssetNumber[1],
      strAssetNumber[2]);
  IGCWriteRecord(temp, szLoggerFileName);

  sprintf(temp, "HFDTE%02d%02d%02d\r\n",
      gps_info.Day,
      gps_info.Month,
      gps_info.Year % 100);
  IGCWriteRecord(temp, szLoggerFileName);

  GetRegistryString(szRegistryPilotName, PilotName, 100);
  sprintf(temp, "HFPLTPILOT:%S\r\n", PilotName);
  IGCWriteRecord(temp, szLoggerFileName);

  GetRegistryString(szRegistryAircraftType, AircraftType, 100);
  sprintf(temp, "HFGTYGLIDERTYPE:%S\r\n", AircraftType);
  IGCWriteRecord(temp, szLoggerFileName);

  GetRegistryString(szRegistryAircraftRego, AircraftRego, 100);
  sprintf(temp, "HFGIDGLIDERID:%S\r\n", AircraftRego);
  IGCWriteRecord(temp, szLoggerFileName);

  sprintf(temp, "HFFTYFR TYPE:XCSOAR,XCSOAR %S\r\n", XCSoar_Version);
  IGCWriteRecord(temp, szLoggerFileName);

  TCHAR DeviceName[DEVNAMESIZE];
  if (is_simulator()) {
    _tcscpy(DeviceName, TEXT("Simulator"));
  } else {
    ReadDeviceSettings(0, DeviceName);
  }
  sprintf(temp, "HFGPS: %S\r\n", DeviceName);
  IGCWriteRecord(temp, szLoggerFileName);

  IGCWriteRecord(datum, szLoggerFileName);
}

void
LoggerImpl::StartDeclaration(const NMEA_INFO &gps_info, const int ntp)
{
  // TODO bug: this is causing problems with some analysis software
  // maybe it's because the date and location fields are bogus
  char start[] = "C0000000N00000000ETAKEOFF\r\n";
  char temp[100];

  if (NumLoggerPreTakeoffBuffered == 0) {
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
  sprintf(temp, "C%02d%02d%02d%02d%02d%02d0000000000%02d\r\n",
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

void
LoggerImpl::EndDeclaration(void)
{
  // TODO bug: this is causing problems with some analysis software
  // maybe it's because the date and location fields are bogus
  const char start[] = "C0000000N00000000ELANDING\r\n";
  IGCWriteRecord(start, szLoggerFileName);
}

void
LoggerImpl::AddDeclaration(double Latitude, double Longitude, const TCHAR *ID)
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
  for (i = 0; i < (int)_tcslen(tmpstring); i++) {
    IDString[i] = (char)tmpstring[i];
  }
  IDString[i] = '\0';

  DegLat = (int)Latitude;
  MinLat = Latitude - DegLat;
  NoS = 'N';
  if ((MinLat < 0) || ((MinLat - DegLat == 0) && (DegLat < 0))) {
    NoS = 'S';
    DegLat *= -1;
    MinLat *= -1;
  }
  MinLat *= 60;
  MinLat *= 1000;

  DegLon = (int)Longitude;
  MinLon = Longitude - DegLon;
  EoW = 'E';
  if ((MinLon < 0) || ((MinLon - DegLon == 0) && (DegLon < 0))) {
    EoW = 'W';
    DegLon *= -1;
    MinLon *= -1;
  }
  MinLon *= 60;
  MinLon *= 1000;

  sprintf(szCRecord, "C%02d%05.0f%c%03d%05.0f%c%s\r\n",
      DegLat, MinLat, NoS, DegLon, MinLon, EoW, IDString);

  IGCWriteRecord(szCRecord, szLoggerFileName);
}

void
LoggerImpl::LoggerNote(const TCHAR *text)
{
  if (LoggerActive) {
    char fulltext[500];
    sprintf(fulltext, "LPLT%S\r\n", text);
    IGCWriteRecord(fulltext, szLoggerFileName);
  }
}

bool
LoggerImpl::LoggerDeclare(struct DeviceDescriptor *dev,
    const struct Declaration *decl)
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

void
LoggerImpl::LoggerDeviceDeclare()
{
  bool found_logger = false;
  struct Declaration Decl;
  int i;

  GetRegistryString(szRegistryPilotName, Decl.PilotName, 64);
  GetRegistryString(szRegistryAircraftType, Decl.AircraftType, 32);
  GetRegistryString(szRegistryAircraftRego, Decl.AircraftRego, 32);

  for (i = 0; task.ValidTaskPoint(i); i++) {
    Decl.waypoint[i] = &task.getWaypoint(i);
  }
  Decl.num_waypoints = i;

  DeclaredToDevice = false;

  if (LoggerDeclare(devA(), &Decl))
    found_logger = true;

  if (LoggerDeclare(devB(), &Decl))
    found_logger = true;

  if (!found_logger) {
    MessageBoxX(gettext(TEXT("No logger connected")),
                devB()->Name, MB_OK | MB_ICONINFORMATION);
    DeclaredToDevice = true; // testing only
  }

}

/**
 * Checks whether a Task is declared to the Logger.
 * If so, asks whether to invalidate the declaration.
 * @return True if a Task is NOT declared to the Logger, False otherwise
 */
bool
LoggerImpl::CheckDeclaration(void)
{
  // if (Task is not declared)
  if (!isTaskDeclared()) {
    return true;

  // else (Task is declared)
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

FILETIME
LogFileDate(const NMEA_INFO &gps_info, TCHAR* filename)
{
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

  if (matches == 5) {
    st.wYear = year;
    st.wMonth = month;
    st.wDay = day;
    st.wHour = num;
    st.wMinute = 0;
    st.wSecond = 0;
    st.wMilliseconds = 0;
    SystemTimeToFileTime(&st, &ft);
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

  if (matches == 5) {
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
    SystemTimeToFileTime(&st, &ft);
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

bool
LogFileIsOlder(const NMEA_INFO &gps_info,
    TCHAR *oldestname, TCHAR *thisname)
{
  FILETIME ftold = LogFileDate(gps_info, oldestname);
  FILETIME ftnew = LogFileDate(gps_info, thisname);
  return (CompareFileTime(&ftold, &ftnew) > 0);
}

/**
 * Delete eldest IGC file in the given path
 * @param gps_info Current NMEA_INFO
 * @param pathname Path where to search for the IGC files
 * @return True if a file was found and deleted, False otherwise
 */
bool
DeleteOldestIGCFile(const NMEA_INFO &gps_info, TCHAR *pathname)
{
  HANDLE hFind; // file handle
  WIN32_FIND_DATA FindFileData;
  TCHAR oldestname[MAX_PATH];
  TCHAR searchpath[MAX_PATH];
  TCHAR fullname[MAX_PATH];
  _stprintf(searchpath, TEXT("%s*"), pathname);

  hFind = FindFirstFile(searchpath, &FindFileData); // find the first file
  if(hFind == INVALID_HANDLE_VALUE)
    return false;

  if(!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
    if (!MatchesExtension(FindFileData.cFileName, TEXT(".igc"))
        && !MatchesExtension(FindFileData.cFileName, TEXT(".IGC")))
      return false;

      // do something...
      _tcscpy(oldestname, FindFileData.cFileName);
  }

  bool bSearch = true;
  // until we scanned all files
  while (bSearch) {
    if (FindNextFile(hFind, &FindFileData)) {
      if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
          && (MatchesExtension(FindFileData.cFileName, TEXT(".igc"))
              || (MatchesExtension(FindFileData.cFileName, TEXT(".IGC"))))) {
        if (LogFileIsOlder(gps_info, oldestname, FindFileData.cFileName)) {
          _tcscpy(oldestname, FindFileData.cFileName);
          // we have a new oldest name
        }
      }
    } else {
      bSearch = false;
    }
  }
  FindClose(hFind); // closing file handle

  // now, delete the file...
  _stprintf(fullname, TEXT("%s%s"), pathname, oldestname);
  DeleteFile(fullname);

  // did delete one
  return true;
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

/**
 * Deletes old IGC files until at least LOGGER_MINFREESTORAGE KiB of space are
 * available
 * @param gps_info Current NMEA_INFO
 * @return True if enough space could be cleared, False otherwise
 */
bool
LoggerImpl::LoggerClearFreeSpace(const NMEA_INFO &gps_info)
{
  bool found = true;
  unsigned long kbfree = 0;
  TCHAR pathname[MAX_PATH];
  TCHAR subpathname[MAX_PATH];
  int numtries = 0;

  LocalPath(pathname);
  if (is_altair()) {
    LocalPath(subpathname, TEXT("logs"));
  } else {
    LocalPath(subpathname);
  }

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

  while (found && ((kbfree = FindFreeSpace(pathname)) < LOGGER_MINFREESTORAGE)
	 && (numtries++ < 100)) {
    /* JMW asking for deleting old files is disabled now --- system
       automatically deletes old files as required
    */

    // search for IGC files, and delete the oldest one
    found = DeleteOldestIGCFile(gps_info, pathname);
    if (!found) {
      found = DeleteOldestIGCFile(gps_info, subpathname);
    }
  }
  if (kbfree >= LOGGER_MINFREESTORAGE) {
    StartupStore(TEXT("LoggerFreeSpace returned: true\r\n"));
    return true;
  } else {
    StartupStore(TEXT("LoggerFreeSpace returned: false\r\n"));
    return false;
  }
}

#include "Interface.hpp"

// TODO: fix scope so only gui things can start it
void
LoggerImpl::guiStartLogger(const NMEA_INFO& gps_info,
    const SETTINGS_COMPUTER& settings, bool noAsk)
{
  int i;

  if (!LoggerActive) {
    if (gps_info.Replay) {
      if (LoggerActive)
        guiStopLogger(gps_info, true);
      return;
    }

    TCHAR TaskMessage[1024];
    _tcscpy(TaskMessage, TEXT("Start Logger With Declaration\r\n"));

    if (task.Valid()) {
      for (i = 0; task.ValidTaskPoint(i); i++) {
        _tcscat(TaskMessage, task.getWaypoint(i).Name);
        _tcscat(TaskMessage, TEXT("\r\n"));
      }
    } else {
      _tcscat(TaskMessage, TEXT("None"));
    }

    if(noAsk || (MessageBoxX(TaskMessage,gettext(TEXT("Start Logger")),
                             MB_YESNO | MB_ICONQUESTION) == IDYES)) {
      if (LoggerClearFreeSpace(gps_info)) {
        StartLogger(gps_info, settings, strAssetNumber);
        LoggerHeader(gps_info);

        if (task.Valid()) {
          int ntp = task.getFinalWaypoint();
          StartDeclaration(gps_info, ntp);
          for (i = 0; task.ValidTaskPoint(i); i++) {
            const WAYPOINT &way_point = task.getWaypoint(i);
            AddDeclaration(way_point.Location.Latitude,
                way_point.Location.Longitude, way_point.Name);
          }
          EndDeclaration();
        }
        LoggerActive = true; // start logger after Header is completed.  Concurrency
      } else {
        MessageBoxX(gettext(TEXT("Logger inactive, insufficient storage!")),
                    gettext(TEXT("Logger Error")), MB_OK| MB_ICONERROR);
        StartupStore(TEXT("Logger not started: Insufficient Storage\r\n"));
      }
    }
  }
}

void
LoggerImpl::guiStopLogger(const NMEA_INFO& gps_info, bool noAsk)
{
  if (LoggerActive) {
    if(noAsk || (MessageBoxX(gettext(TEXT("Stop Logger")),
                             gettext(TEXT("Stop Logger")),
                             MB_YESNO | MB_ICONQUESTION) == IDYES)) {
      StopLogger(gps_info);
    }
  }
}

void
LoggerImpl::guiToggleLogger(const NMEA_INFO& gps_info,
    const SETTINGS_COMPUTER& settings, bool noAsk)
{
  if (LoggerActive) {
    guiStopLogger(gps_info, noAsk);
  } else {
    guiStartLogger(gps_info, settings, noAsk);
  }
}

void
LoggerImpl::clearBuffer()
{
  NumLoggerPreTakeoffBuffered = 0;
}
