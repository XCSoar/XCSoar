/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Logger/Settings.hpp"
#include "LogFile.hpp"
#include "UtilsSystem.hpp"
#include "LocalPath.hpp"
#include "Device/Declaration.hpp"
#include "Compatibility/dirent.h"
#include "NMEA/Info.hpp"
#include "Simulator.hpp"
#include "OS/FileUtil.hpp"
#include "OS/PathName.hpp"
#include "Formatter/IGCFilenameFormatter.hpp"
#include "Interface.hpp"

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

  satellites_used_available = src.gps.satellites_used_available;
  if (satellites_used_available)
    satellites_used = src.gps.satellites_used;

  hdop = src.gps.hdop;
  real = src.gps.real;

  satellite_ids_available = src.gps.satellite_ids_available;
  if (satellite_ids_available)
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

  if (gps_info.alive && !gps_info.gps.real)
    simulator = true;

  writer->Finish();

  if (!simulator)
    writer->Sign();

  // Logger off
  delete writer;
  writer = NULL;

  // Make space for logger file, if unsuccessful -> cancel
  if (!LoggerClearFreeSpace(gps_info.date_time_utc.year))
    return;

  pre_takeoff_buffer.clear();
}

void
LoggerImpl::LogPointToBuffer(const NMEAInfo &gps_info)
{
  if (!gps_info.alive && pre_takeoff_buffer.empty())
    return;

  PreTakeoffBuffer item;
  item = gps_info;
  pre_takeoff_buffer.push(item);
}

void
LoggerImpl::LogEvent(const NMEAInfo &gps_info, const char *event)
{
  if (gps_info.alive && !gps_info.gps.real)
    simulator = true;

  if (writer != NULL)
    writer->LogEvent(gps_info, simulator, event);
}

void
LoggerImpl::LogPoint(const NMEAInfo &gps_info)
{
  if (!gps_info.alive || !gps_info.time_available)
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

    // NOTE: clock is only used to set the validity of valid objects to true
    //       for which "1" is sufficient. This kludge needs to be rewritten.
    tmp_info.clock = fixed_one;

    if (src.nav_warning)
      tmp_info.location_available.Clear();
    else
      tmp_info.location_available.Update(tmp_info.clock);

    tmp_info.gps.fix_quality = src.fix_quality;

    if (!src.satellites_used_available)
      tmp_info.gps.satellites_used_available.Clear();
    else {
      tmp_info.gps.satellites_used_available.Update(tmp_info.clock);
      tmp_info.gps.satellites_used = src.satellites_used;
    }

    tmp_info.gps.hdop = src.hdop;
    tmp_info.gps.real = src.real;

    if (!src.satellite_ids_available)
      tmp_info.gps.satellite_ids_available.Clear();
    else {
      tmp_info.gps.satellite_ids_available.Update(tmp_info.clock);
      for (unsigned i = 0; i < GPSState::MAXSATELLITES; i++)
        tmp_info.gps.satellite_ids[i] = src.satellite_ids[i];
    }

    if (tmp_info.alive && !tmp_info.gps.real)
      simulator = true;

    writer->LogPoint(tmp_info, simulator);
  }

  if (gps_info.alive && !gps_info.gps.real)
    simulator = true;

  writer->LogPoint(gps_info, simulator);
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
                        const LoggerSettings &settings,
                        const char *logger_id)
{
  assert(logger_id != NULL);
  assert(strlen(logger_id) == 3);

  LocalPath(filename, _T("logs"));
  Directory::Create(filename);

  StaticString<64> name;
  for (int i = 1; i < 99; i++) {
    if (!settings.short_name)
      FormatIGCFilenameLong(name.buffer(), gps_info.date_time_utc,
                            "XCS", logger_id, i);
    else
      FormatIGCFilename(name.buffer(), gps_info.date_time_utc,
                        'X', logger_id, i);

    LocalPath(filename, _T("logs"), name);
    if (!File::Exists(filename))
      break;  // file not exist, we'll use this name
  }

  delete writer;

  simulator = gps_info.alive && !gps_info.gps.real;
  writer = new IGCWriter(filename, simulator);

  LogStartUp(_T("Logger Started: %s"), filename);
}

void
LoggerImpl::LoggerNote(const TCHAR *text)
{
  if (writer != NULL)
    writer->LoggerNote(text);
}

static time_t
LogFileDate(unsigned current_year, const TCHAR *filename)
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
    int iyear = (int)current_year;
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
LogFileIsOlder(unsigned current_year,
               const TCHAR *oldestname, const TCHAR *thisname)
{
  return LogFileDate(current_year, oldestname) >
         LogFileDate(current_year, thisname);
}

/**
 * Delete eldest IGC file in the given path
 * @param gps_info Current NMEA_INFO
 * @param pathname Path where to search for the IGC files
 * @return True if a file was found and deleted, False otherwise
 */
static bool
DeleteOldestIGCFile(unsigned current_year, const TCHAR *pathname)
{
  StaticString<MAX_PATH> oldest_name, full_name;

  _TDIR *dir = _topendir(pathname);
  if (dir == NULL)
    return false;

  _tdirent *ent;
  while ((ent = _treaddir(dir)) != NULL) {
    if (!MatchesExtension(ent->d_name, _T(".igc")))
      continue;

    full_name = pathname;
    full_name += ent->d_name;

    if (File::Exists(full_name) &&
        LogFileIsOlder(current_year, oldest_name, ent->d_name))
      // we have a new oldest name
      oldest_name = ent->d_name;
  }

  _tclosedir(dir);

  // now, delete the file...
  full_name.Format(_T("%s%s"), pathname, oldest_name.c_str());
  File::Delete(full_name);

  // did delete one
  return true;
}

bool
LoggerImpl::LoggerClearFreeSpace(unsigned current_year)
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
    found = DeleteOldestIGCFile(current_year, pathname);
    if (!found)
      found = DeleteOldestIGCFile(current_year, subpathname);
  }

  if (kbfree >= LOGGER_MINFREESTORAGE) {
    LogStartUp(_T("LoggerFreeSpace returned: true"));
    return true;
  } else {
    LogStartUp(_T("LoggerFreeSpace returned: false"));
    return false;
  }
}

static const TCHAR *
GetGPSDeviceName()
{
  if (is_simulator())
    return _T("Simulator");

  const DeviceConfig &device = CommonInterface::GetSystemSettings().devices[0];
  if (device.UsesDriver())
    return device.driver_name;

  if (device.IsAndroidInternalGPS())
    return _T("Internal GPS (Android)");

  return _T("Unknown");
}

// TODO: fix scope so only gui things can start it
void
LoggerImpl::StartLogger(const NMEAInfo &gps_info,
                        const LoggerSettings &settings,
                        const TCHAR *asset_number, const Declaration &decl)
{
  if (!settings.logger_id.empty())
    asset_number = settings.logger_id.c_str();

  // chars must be legal in file names
  char logger_id[4];
  unsigned asset_length = _tcslen(asset_number);
  for (unsigned i = 0; i < 3; i++)
    logger_id[i] = i < asset_length && IsAlphaNum(asset_number[i]) ?
                   asset_number[i] : _T('A');
  logger_id[3] = _T('\0');

  StartLogger(gps_info, settings, logger_id);

  writer->WriteHeader(gps_info.date_time_utc, decl.pilot_name,
                      decl.aircraft_type, decl.aircraft_registration,
                      decl.competition_id,
                      logger_id, GetGPSDeviceName(), simulator);

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
