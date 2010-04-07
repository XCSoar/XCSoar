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
#include "Components.hpp"
#include "TaskClientUI.hpp"
#include "Version.hpp"
#include "Dialogs/Message.hpp"
#include "Language.hpp"
#include "Profile.hpp"
#include "LogFile.hpp"
#include "Asset.hpp"
#include "UtilsText.hpp"
#include "UtilsSystem.hpp"
#include "UtilsFile.hpp"
#include "LocalPath.hpp"
#include "Device/device.hpp"
#include "Device/Descriptor.hpp"
#include "Device/List.hpp"
#include "Compatibility/string.h"
#include "SettingsComputer.hpp"
#include "NMEA/Info.hpp"
#include "Simulator.hpp"

#ifdef HAVE_POSIX
#include <unistd.h>
#endif
#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <tchar.h>
#include <algorithm>

const struct LoggerImpl::LoggerPreTakeoffBuffer &
LoggerImpl::LoggerPreTakeoffBuffer::operator=(const NMEA_INFO &src)
{
  Location = src.Location;
  Altitude = src.GPSAltitude;
  BaroAltitude = src.GetAltitudeBaroPreferred();

  DateTime = src.DateTime;
  Time = src.Time;

  NAVWarning = src.gps.NAVWarning;

  std::copy(src.gps.SatelliteIDs, src.gps.SatelliteIDs + MAXSATELLITES,
            SatelliteIDs);

  return *this;
}

LoggerImpl::LoggerImpl():
  LoggerActive(false),
  DeclaredToDevice(false),
  Simulator(false),
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

static TCHAR
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

static int
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

/**
 * Stops the logger
 * @param gps_info NMEA_INFO struct holding the current date
 */
void
LoggerImpl::StopLogger(const NMEA_INFO &gps_info)
{
  // Logger can't be switched off if already off -> cancel
  if (!LoggerActive)
    return;

  // Logger off
  LoggerActive = false;

  if (gps_info.gps.Simulator)
    Simulator = true;

  // Make space for logger file, if unsuccessful -> cancel
  if (!LoggerClearFreeSpace(gps_info))
    return;

  // Write IGC File
  WriteLock();
  DiskBufferFlush();
  Unlock();

  // Write GRecord
  if (!Simulator && LoggerGActive())
    LoggerGStop(szLoggerFileName);

  NumLoggerPreTakeoffBuffered = 0;
}

void
LoggerImpl::LogPointToBuffer(const NMEA_INFO &gps_info)
{
  if (NumLoggerPreTakeoffBuffered == LOGGER_PRETAKEOFF_BUFFER_MAX) {
    for (int i = 0; i < NumLoggerPreTakeoffBuffered - 1; i++) {
      LoggerPreTakeoffBuffer[i] = LoggerPreTakeoffBuffer[i+1];
    }
  } else {
    NumLoggerPreTakeoffBuffered++;
  }

  LoggerPreTakeoffBuffer[NumLoggerPreTakeoffBuffered - 1] = gps_info;

  // This is the first point that will be output to file.
  // Declaration must happen before this, so must save this time.
  FirstPoint = LoggerPreTakeoffBuffer[0];
}


void
LoggerImpl::LogPointToFile(const NMEA_INFO& gps_info)
{
  char szBRecord[500];
  int iSIU=GetSIU(gps_info);
  double dEPE=GetEPE(gps_info);

  int DegLat, DegLon;
  double MinLat, MinLon;
  char NoS, EoW;

  if (gps_info.gps.Simulator || is_simulator())
    /* if at least one GPS fix comes from the simulator, disable
       signing */
    Simulator = true;
  else
    LogFRecordToFile(gps_info.gps.SatelliteIDs,
                     gps_info.DateTime, gps_info.Time,
                     gps_info.gps.NAVWarning);

  if ((gps_info.GPSAltitude < -100) || (gps_info.BaroAltitude < -100)
      || gps_info.gps.NAVWarning) {
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

  sprintf(szBRecord,"B%02d%02d%02d%02d%05.0f%c%03d%05.0f%cA%05d%05d%03d%02d\r\n",
          gps_info.DateTime.hour, gps_info.DateTime.minute,
          gps_info.DateTime.second,
          DegLat, MinLat, NoS, DegLon, MinLon, EoW,
          (int)gps_info.BaroAltitude,(int)gps_info.GPSAltitude,(int)dEPE,iSIU);

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
      tmp_info.Location = LoggerPreTakeoffBuffer[i].Location;
      tmp_info.GPSAltitude = LoggerPreTakeoffBuffer[i].Altitude;
      tmp_info.BaroAltitude = LoggerPreTakeoffBuffer[i].BaroAltitude;
      tmp_info.DateTime = LoggerPreTakeoffBuffer[i].DateTime;
      tmp_info.Time = LoggerPreTakeoffBuffer[i].Time;
      tmp_info.gps.NAVWarning = LoggerPreTakeoffBuffer[i].NAVWarning;

      for (int iSat = 0; iSat < MAXSATELLITES; iSat++)
        tmp_info.gps.SatelliteIDs[iSat] =
          LoggerPreTakeoffBuffer[i].SatelliteIDs[iSat];

      LogPointToFile(tmp_info);
    }

    NumLoggerPreTakeoffBuffered = 0;
  }

  if (LoggerActive) {
    LogPointToFile(gps_info);
  }
}

static bool
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
  int i;
  TCHAR path[MAX_PATH];

  for (i = 0; i < 3; i++) { // chars must be legal in file names
    strAssetNumber[i] = IsAlphaNum(strAssetNumber[i]) ? strAssetNumber[i] : _T('A');
  }

  // VENTA3 use logs subdirectory when not in main memory (true for FIVV and PNA)
  #if defined(GNAV) || defined(FIVV) || defined(PNA)
    LocalPath(path, _T("logs"));
  #else
    LocalPath(path);
  #endif

  WriteLock();
  DiskBufferReset();
  Unlock();

  Simulator = gps_info.gps.Simulator;
  if (!Simulator)
    LoggerGInit();

  ResetFRecord();

  for (i = 1; i < 99; i++) {
    // 2003-12-31-XXX-987-01.IGC
    // long filename form of IGC file.
    // XXX represents manufacturer code

    if (!settings.LoggerShortName) {
      // Long file name
      _stprintf(szLoggerFileName,
                _T("%s\\%04u-%02u-%02u-XCS-%c%c%c-%02d.IGC"),
          path,
                gps_info.DateTime.year,
                gps_info.DateTime.month,
                gps_info.DateTime.day,
          strAssetNumber[0],
          strAssetNumber[1],
          strAssetNumber[2],
          i);
    } else {
      // Short file name
      TCHAR cyear, cmonth, cday, cflight;
      cyear = NumToIGCChar((int)gps_info.DateTime.year % 10);
      cmonth = NumToIGCChar(gps_info.DateTime.month);
      cday = NumToIGCChar(gps_info.DateTime.day);
      cflight = NumToIGCChar(i);
      _stprintf(szLoggerFileName,
                _T("%s\\%c%c%cX%c%c%c%c.IGC"),
          path,
          cyear,
          cmonth,
          cday,
          strAssetNumber[0],
          strAssetNumber[1],
          strAssetNumber[2],
          cflight);

    }

    FILE *file = _tfopen(szLoggerFileName, _T("w"));
    if (file != NULL) {
      // file did not already exist, and could be created
      fclose(file);
#ifdef HAVE_POSIX
      _tunlink(szLoggerFileName);
#else
      DeleteFile(szLoggerFileName);
#endif
      break;
    }
  }

  LogStartUp(_T("Logger Started: %s"), szLoggerFileName);
}

void
LoggerImpl::LoggerHeader(const NMEA_INFO &gps_info, const Declaration &decl)
{
  char datum[] = "HFDTM100Datum: WGS-84\r\n";
  char temp[100];

  // Flight recorder ID number MUST go first..
  sprintf(temp, "AXCS%C%C%C\r\n",
      strAssetNumber[0],
      strAssetNumber[1],
      strAssetNumber[2]);
  IGCWriteRecord(temp, szLoggerFileName);

  sprintf(temp, "HFDTE%02u%02u%02u\r\n",
          gps_info.DateTime.day,
          gps_info.DateTime.month,
          gps_info.DateTime.year % 100);
  IGCWriteRecord(temp, szLoggerFileName);

  IGCWriteRecord(GetHFFXARecord(), szLoggerFileName);

  sprintf(temp, "HFPLTPILOT:%S\r\n", decl.PilotName);
  IGCWriteRecord(temp, szLoggerFileName);

  sprintf(temp, "HFGTYGLIDERTYPE:%S\r\n", decl.AircraftType);
  IGCWriteRecord(temp, szLoggerFileName);

  sprintf(temp, "HFGIDGLIDERID:%S\r\n", decl.AircraftRego);
  IGCWriteRecord(temp, szLoggerFileName);

  sprintf(temp, "HFFTYFR TYPE:XCSOAR,XCSOAR %S\r\n", XCSoar_VersionStringOld);
  IGCWriteRecord(temp, szLoggerFileName);

  DeviceConfig device_config;
  if (gps_info.gps.Simulator) {
    _tcscpy(device_config.driver_name, _T("Simulator"));
  } else {
    Profile::GetDeviceConfig(0, device_config);
  }

  sprintf(temp, "HFGPS: %S\r\n", device_config.driver_name);
  IGCWriteRecord(temp, szLoggerFileName);

  IGCWriteRecord(datum, szLoggerFileName);

  IGCWriteRecord(GetIRecord(), szLoggerFileName);
}

void
LoggerImpl::StartDeclaration(const NMEA_INFO &gps_info, const int ntp)
{
  // TODO bug: this is causing problems with some analysis software
  // maybe it's because the date and location fields are bogus
  char start[] = "C0000000N00000000ETAKEOFF\r\n";
  char temp[100];

  if (NumLoggerPreTakeoffBuffered == 0) {
    FirstPoint.DateTime = gps_info.DateTime;
  }

  // JMW added task start declaration line

  // LGCSTKF013945TAKEOFF DETECTED

  // IGC GNSS specification 3.6.1
  sprintf(temp, "C%02u%02u%02u%02u%02u%02u0000000000%02d\r\n",
      // DD  MM  YY  HH  MM  SS  DD  MM  YY IIII TT
          FirstPoint.DateTime.day,
          FirstPoint.DateTime.month,
          FirstPoint.DateTime.year % 100,
          FirstPoint.DateTime.hour,
          FirstPoint.DateTime.minute,
          FirstPoint.DateTime.second,
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
LoggerImpl::AddDeclaration(const GEOPOINT &location, const TCHAR *ID)
{
  const double Latitude = location.Latitude;
  const double Longitude = location.Longitude;

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
  if (!LoggerActive)
    return;

  char fulltext[500];
  sprintf(fulltext, "LPLT%S\r\n", text);
  IGCWriteRecord(fulltext, szLoggerFileName);
}

bool
LoggerImpl::LoggerDeclare(struct DeviceDescriptor *dev,
                          const Declaration &decl)
{
  if (!devIsLogger(*dev))
    return false;

  if (MessageBoxX(gettext(_T("Declare Task?")),
                  dev->GetName(), MB_YESNO| MB_ICONQUESTION) == IDYES) {
    if (devDeclare(*dev, &decl)) {
      MessageBoxX(gettext(_T("Task Declared!")),
                  dev->GetName(), MB_OK| MB_ICONINFORMATION);
      DeclaredToDevice = true;
    } else {
      MessageBoxX(gettext(_T("Error occured,\r\nTask NOT Declared!")),
                  dev->GetName(), MB_OK| MB_ICONERROR);
      DeclaredToDevice = false;
    }
  }

  return true;
}

void
LoggerImpl::LoggerDeviceDeclare()
{
  DeclaredToDevice = false;
  bool found_logger = false;
  OrderedTask* task = task_ui.task_clone();
  if (!task) return;

  const Declaration decl(*task);
  delete task;

  for (unsigned i = 0; i < NUMDEV; ++i)
    if (LoggerDeclare(&DeviceList[i], decl))
      found_logger = true;

  if (!found_logger) {
    MessageBoxX(gettext(_T("No logger connected")),
                gettext(_T("Declare task")), MB_OK | MB_ICONINFORMATION);
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
  // if (Task is not declared) -> return true;
  if (!isTaskDeclared())
    return true;

  if (MessageBoxX(gettext(_T("OK to invalidate declaration?")),
                  gettext(_T("Task declared")),
     MB_YESNO| MB_ICONQUESTION) == IDYES){
    DeclaredToDevice = false;
    return true;
  } else {
    return false;
  }
}

static time_t
LogFileDate(const NMEA_INFO &gps_info, const TCHAR *filename)
{
  TCHAR asset[MAX_PATH];
  unsigned short year, month, day, num;
  int matches;
  // scan for long filename
  matches = _stscanf(filename,
                     _T("%hu-%hu-%hu-%7s-%hu.IGC"),
                    &year,
                    &month,
                    &day,
                    asset,
                    &num);

  if (matches == 5) {
    struct tm tm;
    tm.tm_sec = 0;
    tm.tm_min = 0;
    tm.tm_hour = num;
    tm.tm_mday = day;
    tm.tm_mon = month - 1;
    tm.tm_year = year - 1900;
    tm.tm_isdst = -1;
    return mktime(&tm);
  }

  TCHAR cyear, cmonth, cday, cflight;
  // scan for short filename
  matches = _stscanf(filename,
                     _T("%c%c%c%4s%c.IGC"),
		     &cyear,
		     &cmonth,
		     &cday,
		     asset,
		     &cflight);

  if (matches == 5) {
    int iyear = (int)gps_info.DateTime.year;
    int syear = iyear % 10;
    int yearzero = iyear - syear;
    int yearthis = IGCCharToNum(cyear) + yearzero;
    if (yearthis > iyear) {
      yearthis -= 10;
    }

    struct tm tm;
    tm.tm_sec = 0;
    tm.tm_min = 0;
    tm.tm_hour = IGCCharToNum(cflight);
    tm.tm_mday = IGCCharToNum(cday);
    tm.tm_mon = IGCCharToNum(cmonth) - 1;
    tm.tm_year = yearthis - 1900;
    tm.tm_isdst = -1;
    return mktime(&tm);
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

  return 0;
}

static bool
LogFileIsOlder(const NMEA_INFO &gps_info,
               const TCHAR *oldestname, const TCHAR *thisname)
{
  return LogFileDate(gps_info, oldestname) > LogFileDate(gps_info, thisname);
}

/**
 * Delete eldest IGC file in the given path
 * @param gps_info Current NMEA_INFO
 * @param pathname Path where to search for the IGC files
 * @return True if a file was found and deleted, False otherwise
 */
static bool
DeleteOldestIGCFile(const NMEA_INFO &gps_info, const TCHAR *pathname)
{
  TCHAR oldestname[MAX_PATH];
  TCHAR fullname[MAX_PATH];

  _TDIR *dir = _topendir(pathname);
  if (dir == NULL)
    return false;

  _tdirent *ent;
  while ((ent = _treaddir(dir)) != NULL) {
    if (!MatchesExtension(ent->d_name, _T(".igc")) &&
        !MatchesExtension(ent->d_name, _T(".IGC")))
      continue;

    _tcscpy(fullname, pathname);
    _tcscpy(fullname, ent->d_name);

    if (FileExists(fullname) &&
        LogFileIsOlder(gps_info, oldestname, ent->d_name))
      // we have a new oldest name
      _tcscpy(oldestname, ent->d_name);
  }

  _tclosedir(dir);

  // now, delete the file...
  _stprintf(fullname, _T("%s%s"), pathname, oldestname);
  DeleteFile(fullname);

  // did delete one
  return true;
}

#define LOGGER_MINFREESTORAGE (250+MINFREESTORAGE)
// JMW note: we want to clear up enough space to save the persistent
// data (85 kb approx) and a new log file

#ifdef DEBUG_IGCFILENAME
static const TCHAR testtext1[] = _T("2007-11-05-XXX-AAA-01.IGC");
static const TCHAR testtext2[] = _T("2007-11-05-XXX-AAA-02.IGC");
static const TCHAR testtext3[] = _T("3BOA1VX2.IGC");
static const TCHAR testtext4[] = _T("5BDX7B31.IGC");
static const TCHAR testtext5[] = _T("3BOA1VX2.IGC");
static const TCHAR testtext6[] = _T("9BDX7B31.IGC");
static const TCHAR testtext7[] = _T("2008-01-05-XXX-AAA-01.IGC");
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
    LocalPath(subpathname, _T("logs"));
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
    LogStartUp(_T("LoggerFreeSpace returned: true"));
    return true;
  } else {
    LogStartUp(_T("LoggerFreeSpace returned: false"));
    return false;
  }
}

#include "Interface.hpp"

// TODO: fix scope so only gui things can start it
void
LoggerImpl::guiStartLogger(const NMEA_INFO& gps_info,
    const SETTINGS_COMPUTER& settings, bool noAsk)
{
  if (LoggerActive)
    return;

  if (gps_info.gps.Replay) {
    guiStopLogger(gps_info, true);
    return;
  }

  OrderedTask* task = task_ui.task_clone();
  if (!task) return;

  const Declaration decl(*task);
  delete task;

  TCHAR TaskMessage[1024];
  _tcscpy(TaskMessage, _T("Start Logger With Declaration\r\n"));

  if (decl.size()) {
    for (unsigned i = 0; i< decl.size(); ++i) {
      _tcscat(TaskMessage, decl.get_name(i));
      _tcscat(TaskMessage, _T("\r\n"));
    }
  } else {
    _tcscat(TaskMessage, _T("None"));
  }

  if (noAsk || (MessageBoxX(TaskMessage, gettext(_T("Start Logger")),
                           MB_YESNO | MB_ICONQUESTION) == IDYES)) {
    if (LoggerClearFreeSpace(gps_info)) {
      StartLogger(gps_info, settings, strAssetNumber);
      LoggerHeader(gps_info, decl);

      if (decl.size()) {
        StartDeclaration(gps_info, decl.size());
        for (unsigned i = 0; i< decl.size(); ++i) {
          AddDeclaration(decl.get_location(i), decl.get_name(i));
        }
        EndDeclaration();
      }
      LoggerActive = true; // start logger after Header is completed.  Concurrency
    } else {
      MessageBoxX(gettext(_T("Logger inactive, insufficient storage!")),
                  gettext(_T("Logger Error")), MB_OK| MB_ICONERROR);
      LogStartUp(_T("Logger not started: Insufficient Storage"));
    }
  }
}

void
LoggerImpl::guiStopLogger(const NMEA_INFO& gps_info, bool noAsk)
{
  if (!LoggerActive)
    return;

  if(noAsk || (MessageBoxX(gettext(_T("Stop Logger")),
                           gettext(_T("Stop Logger")),
                           MB_YESNO | MB_ICONQUESTION) == IDYES)) {
    StopLogger(gps_info);
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
