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

#include "Logger/LoggerImpl.hpp"
#include "Version.hpp"
#include "Profile/DeviceConfig.hpp"
#include "LogFile.hpp"
#include "Asset.hpp"
#include "UtilsSystem.hpp"
#include "UtilsFile.hpp"
#include "LocalPath.hpp"
#include "Device/Declaration.hpp"
#include "Compatibility/path.h"
#include "Compatibility/dirent.h"
#include "SettingsComputer.hpp"
#include "NMEA/Info.hpp"
#include "Simulator.hpp"
#include "Interface.hpp"
#include "OS/FileUtil.hpp"

#ifdef HAVE_POSIX
#include <unistd.h>
#endif
#include <time.h>
#include <sys/types.h>
#include <tchar.h>
#include <stdio.h>
#include <algorithm>

// JMW note: we want to clear up enough space to save the persistent
// data (85 kb approx) and a new log file
#define LOGGER_MINFREESTORAGE 750

const struct LoggerImpl::PreTakeoffBuffer &
LoggerImpl::PreTakeoffBuffer::operator=(const NMEAInfo &src)
{
  location = src.location;
  altitude_gps = src.gps_altitude;
  altitude_baro = src.GetAltitudeBaroPreferred();

  date_time_utc = src.date_time_utc;
  time = src.time;

  nav_warning = !src.location_available;
  fix_quality = src.gps.fix_quality;
  satellites_used = src.gps.satellites_used;
  hdop = src.gps.hdop;
  real = src.gps.real;

  std::copy(src.gps.satellite_ids,
            src.gps.satellite_ids + GPSState::MAXSATELLITES,
            satellite_ids);

  return *this;
}

LoggerImpl::LoggerImpl()
  :writer(NULL)
{
  filename[0] = 0;
}

LoggerImpl::~LoggerImpl()
{
  delete writer;
}

static TCHAR
NumToIGCChar(int num)
{
  if (num < 10)
    return _T('1') + (num - 1);

  return _T('A') + (num - 10);
}

static int
IGCCharToNum(TCHAR c)
{
  if ((c >= _T('1')) && (c <= _T('9')))
    return c - _T('1') + 1;

  if ((c >= _T('A')) && (c <= _T('Z')))
    return c - _T('A') + 10;

  return 0; // Error!
}

void
LoggerImpl::StopLogger(const NMEAInfo &gps_info)
{
  // Logger can't be switched off if already off -> cancel
  if (writer == NULL)
    return;

  writer->Finish(gps_info);
  writer->Sign();

  // Logger off
  delete writer;
  writer = NULL;

  // Make space for logger file, if unsuccessful -> cancel
  if (!LoggerClearFreeSpace(gps_info))
    return;

  pre_takeoff_buffer.clear();
}

void
LoggerImpl::LogPointToBuffer(const NMEAInfo &gps_info)
{
  if (!gps_info.connected && pre_takeoff_buffer.empty())
    return;

  PreTakeoffBuffer item;
  item = gps_info;
  pre_takeoff_buffer.push(item);
}

void
LoggerImpl::LogEvent(const NMEAInfo &gps_info, const char *event)
{
  if (writer != NULL)
    writer->LogEvent(gps_info, event);
}

void
LoggerImpl::LogPoint(const NMEAInfo &gps_info)
{
  if (!gps_info.connected || !gps_info.time_available)
    return;

  if (writer == NULL) {
    LogPointToBuffer(gps_info);
    return;
  }

  while (!pre_takeoff_buffer.empty()) {
    const struct PreTakeoffBuffer &src = pre_takeoff_buffer.shift();
    NMEAInfo tmp_info;
    tmp_info.location = src.location;
    tmp_info.gps_altitude = src.altitude_gps;
    tmp_info.baro_altitude = src.altitude_baro;
    tmp_info.date_time_utc = src.date_time_utc;
    tmp_info.time = src.time;

    if (src.nav_warning)
      tmp_info.location_available.Clear();
    else
      tmp_info.location_available.Update(tmp_info.clock);

    tmp_info.gps.fix_quality = src.fix_quality;
    tmp_info.gps.satellites_used = src.satellites_used;
    tmp_info.gps.hdop = src.hdop;
    tmp_info.gps.real = src.real;

    for (unsigned iSat = 0; iSat < GPSState::MAXSATELLITES; iSat++)
      tmp_info.gps.satellite_ids[iSat] = src.satellite_ids[iSat];

    writer->LogPoint(tmp_info);
  }

  writer->LogPoint(gps_info);
}

static bool
IsAlphaNum (TCHAR c)
{
  if ((c >= _T('A') && c <= _T('Z')) ||
      (c >= _T('a') && c <= _T('z')) ||
      (c >= _T('0') && c <= _T('9')))
    return true;

  return false;
}

void
LoggerImpl::StartLogger(const NMEAInfo &gps_info,
    const SETTINGS_COMPUTER &settings, const TCHAR *asset_number)
{
  int i;

  // chars must be legal in file names
  for (i = 0; i < 3; i++)
    strAssetNumber[i] = IsAlphaNum(strAssetNumber[i]) ?
                        strAssetNumber[i] : _T('A');

  LocalPath(filename, _T("logs"));
  Directory::Create(filename);

  TCHAR name[64];
  for (i = 1; i < 99; i++) {
    // 2003-12-31-XXX-987-01.igc
    // long filename form of IGC file.
    // XXX represents manufacturer code

    if (!settings.logger_short_name) {
      // Long file name
      _stprintf(name,
                _T("%04u-%02u-%02u-XCS-%c%c%c-%02d.igc"),
                gps_info.date_time_utc.year,
                gps_info.date_time_utc.month,
                gps_info.date_time_utc.day,
          strAssetNumber[0],
          strAssetNumber[1],
          strAssetNumber[2],
          i);
    } else {
      // Short file name
      TCHAR cyear, cmonth, cday, cflight;
      cyear = NumToIGCChar((int)gps_info.date_time_utc.year % 10);
      cmonth = NumToIGCChar(gps_info.date_time_utc.month);
      cday = NumToIGCChar(gps_info.date_time_utc.day);
      cflight = NumToIGCChar(i);
      _stprintf(name,
                _T("%c%c%cX%c%c%c%c.igc"),
          cyear,
          cmonth,
          cday,
          strAssetNumber[0],
          strAssetNumber[1],
          strAssetNumber[2],
          cflight);

    }

    LocalPath(filename, _T("logs"), name);
    if (!File::Exists(filename))
      break;  // file not exist, we'll use this name
  }

  delete writer;
  writer = new IGCWriter(filename, gps_info);

  LogStartUp(_T("Logger Started: %s"), filename);
}

void
LoggerImpl::LoggerNote(const TCHAR *text)
{
  if (writer != NULL)
    writer->LoggerNote(text);
}

static time_t
LogFileDate(const NMEAInfo &gps_info, const TCHAR *filename)
{
  TCHAR asset[MAX_PATH];

  // scan for long filename
  unsigned short year, month, day, num;
  int matches = _stscanf(filename, _T("%hu-%hu-%hu-%7s-%hu.igc"),
                         &year, &month, &day, asset, &num);

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
  matches = _stscanf(filename, _T("%c%c%c%4s%c.igc"),
		                 &cyear, &cmonth, &cday, asset, &cflight);

  if (matches == 5) {
    int iyear = (int)gps_info.date_time_utc.year;
    int syear = iyear % 10;
    int yearzero = iyear - syear;
    int yearthis = IGCCharToNum(cyear) + yearzero;
    if (yearthis > iyear)
      yearthis -= 10;

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
      YMDCXXXF.igc
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
LogFileIsOlder(const NMEAInfo &gps_info,
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
DeleteOldestIGCFile(const NMEAInfo &gps_info, const TCHAR *pathname)
{
  TCHAR oldest_name[MAX_PATH];
  TCHAR full_name[MAX_PATH];

  _TDIR *dir = _topendir(pathname);
  if (dir == NULL)
    return false;

  _tdirent *ent;
  while ((ent = _treaddir(dir)) != NULL) {
    if (!MatchesExtension(ent->d_name, _T(".igc")))
      continue;

    _tcscpy(full_name, pathname);
    _tcscpy(full_name, ent->d_name);

    if (File::Exists(full_name) &&
        LogFileIsOlder(gps_info, oldest_name, ent->d_name))
      // we have a new oldest name
      _tcscpy(oldest_name, ent->d_name);
  }

  _tclosedir(dir);

  // now, delete the file...
  _stprintf(full_name, _T("%s%s"), pathname, oldest_name);
  File::Delete(full_name);

  // did delete one
  return true;
}

bool
LoggerImpl::LoggerClearFreeSpace(const NMEAInfo &gps_info)
{
  bool found = true;
  unsigned long kbfree = 0;
  const TCHAR *pathname = GetPrimaryDataPath();
  TCHAR subpathname[MAX_PATH];
  int numtries = 0;

  LocalPath(subpathname, _T("logs"));

  while (found && ((kbfree = FindFreeSpace(pathname)) < LOGGER_MINFREESTORAGE)
	       && (numtries++ < 100)) {
    /* JMW asking for deleting old files is disabled now --- system
       automatically deletes old files as required
    */

    // search for IGC files, and delete the oldest one
    found = DeleteOldestIGCFile(gps_info, pathname);
    if (!found)
      found = DeleteOldestIGCFile(gps_info, subpathname);
  }

  if (kbfree >= LOGGER_MINFREESTORAGE) {
    LogStartUp(_T("LoggerFreeSpace returned: true"));
    return true;
  } else {
    LogStartUp(_T("LoggerFreeSpace returned: false"));
    return false;
  }
}

// TODO: fix scope so only gui things can start it
void
LoggerImpl::StartLogger(const NMEAInfo &gps_info,
                        const SETTINGS_COMPUTER &settings,
                        const TCHAR *asset_number, const Declaration &decl)
{
  StartLogger(gps_info, settings, asset_number);

  DeviceConfig device_config;
  // this is only the XCSoar Simulator, not Condor etc, so don't use Simulator flag
  if (is_simulator())
    device_config.driver_name = _T("Simulator");
  else
    Profile::GetDeviceConfig(0, device_config);

  writer->WriteHeader(gps_info.date_time_utc, decl.pilot_name,
                      decl.aircraft_type, decl.aircraft_registration,
                      asset_number, device_config.driver_name);

  if (decl.Size()) {
    BrokenDateTime FirstDateTime = !pre_takeoff_buffer.empty()
      ? pre_takeoff_buffer.peek().date_time_utc
      : gps_info.date_time_utc;
    writer->StartDeclaration(FirstDateTime, decl.Size());

    for (unsigned i = 0; i< decl.Size(); ++i)
      writer->AddDeclaration(decl.GetLocation(i), decl.GetName(i));

    writer->EndDeclaration();
  }
}

void
LoggerImpl::ClearBuffer()
{
  pre_takeoff_buffer.clear();
}
