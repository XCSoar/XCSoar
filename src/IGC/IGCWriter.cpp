/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "IGC/IGCWriter.hpp"
#include "IGCString.hpp"
#include "Generator.hpp"
#include "NMEA/Info.hpp"
#include "Version.hpp"
#include "OS/Path.hpp"

#include <assert.h>

IGCWriter::IGCWriter(Path path)
  :file(path,
        /* we use CREATE_VISIBLE here so the user can recover partial
           IGC files after a crash/battery failure/etc. */
        FileOutputStream::Mode::CREATE_VISIBLE),
   buffered(file)
{
  fix.Clear();

  grecord.Initialize();
}

void
IGCWriter::CommitLine(char *line)
{
  buffered.Write(line);
  buffered.Write('\n');

  grecord.AppendRecordToBuffer(line);
}

void
IGCWriter::WriteLine(const char *line)
{
  assert(strchr(line, '\r') == NULL);
  assert(strchr(line, '\n') == NULL);

  char *const dest = BeginLine();
  char *const end = dest + MAX_IGC_BUFF - 1;

  char *p = CopyIGCString(dest, end, line);
  *p = '\0';

  CommitLine(dest);
}

void
IGCWriter::WriteLine(const char *a, const TCHAR *b)
{
  size_t a_length = strlen(a);
  assert(a_length < MAX_IGC_BUFF);

  char *const dest = BeginLine();
  char *const end = dest + MAX_IGC_BUFF - 1, *p = dest;

  p = std::copy_n(a, a_length, p);
  p = CopyIGCString(p, end, b);
  *p = '\0';

  CommitLine(dest);
}

void
IGCWriter::WriteHeader(const BrokenDateTime &date_time,
                       const TCHAR *pilot_name, const TCHAR *aircraft_model,
                       const TCHAR *aircraft_registration,
                       const TCHAR *competition_id,
                       const char *logger_id, const TCHAR *driver_name,
                       bool simulator)
{
  /*
   * HFDTE141203  <- should be UTC, same as time in filename
   * HFFXA100
   * HFPLTPILOT:JOHN WHARINGTON
   * HFGTYGLIDERTYPE:LS 3
   * HFGIDGLIDERID:VH-WUE
   * HFDTM100GPSDATUM:WGS-1984
   * HFRFWFIRMWAREVERSION:3.6
   * HFRHWHARDWAREVERSION:3.4
   * HFFTYFR TYPE:GARRECHT INGENIEURGESELLSCHAFT,VOLKSLOGGER 1.0
   * HFCIDCOMPETITIONID:WUE
   * HFCCLCOMPETITIONCLASS:FAI
   */

  assert(date_time.IsPlausible());
  assert(logger_id != NULL);
  assert(strlen(logger_id) == 3);

  char buffer[100];

  // Flight recorder ID number MUST go first..
  sprintf(buffer, "AXCS%s", logger_id);
  WriteLine(buffer);

  sprintf(buffer, "HFDTE%02u%02u%02u",
          date_time.day, date_time.month, date_time.year % 100);
  WriteLine(buffer);

  if (!simulator)
    WriteLine(GetHFFXARecord());

  WriteLine("HFPLTPILOTINCHARGE:", pilot_name);
  WriteLine("HFGTYGLIDERTYPE:", aircraft_model);
  WriteLine("HFGIDGLIDERID:", aircraft_registration);
  WriteLine("HFCIDCOMPETITIONID:", competition_id);
  WriteLine("HFFTYFRTYPE:XCSOAR,XCSOAR ", XCSoar_VersionStringOld);
  WriteLine("HFGPS:", driver_name);

  WriteLine("HFDTM100DATUM:WGS-1984");

  WriteLine(GetIRecord());
}

void
IGCWriter::StartDeclaration(const BrokenDateTime &date_time,
                            const int number_of_turnpoints)
{
  char buffer[64];
  FormatIGCTaskTimestamp(buffer, date_time, number_of_turnpoints);
  WriteLine(buffer);

  WriteLine(IGCMakeTaskTakeoff());
}

void
IGCWriter::EndDeclaration()
{
  // TODO bug: this is causing problems with some analysis software
  // maybe it's because the date and location fields are bogus
  WriteLine(IGCMakeTaskLanding());
}

void
IGCWriter::AddDeclaration(const GeoPoint &location, const TCHAR *id)
{
  char c_record[64];
  FormatIGCTaskTurnPoint(c_record, location, id);
  WriteLine(c_record);
}

void
IGCWriter::LoggerNote(const TCHAR *text)
{
  WriteLine("LPLT", text);
}

/**
 * Applies range checks to the specified altitude value and converts
 * it to an integer suitable for printing in the IGC file.
 */
static int
NormalizeIGCAltitude(int value)
{
  if (value < -9999)
    /* for negative values, there are only 4 characters left (after
       the minus sign), and besides that, IGC does not support a
       journey towards the center of the earth */
    return -9999;

  if (value >= 99999)
    /* hooray, new world record! .. or just some invalid value; we
       have only 5 characters for the altitude, so we must clip it at
       99999 */
    return 99999;

  return value;
}

void
IGCWriter::LogPoint(const IGCFix &fix, int epe, int satellites)
{
  char b_record[128];
  char *p = b_record;

  sprintf(p, "B%02d%02d%02d", fix.time.hour, fix.time.minute, fix.time.second);
  p += strlen(p);

  p = FormatIGCLocation(p, fix.location);

  sprintf(p, "%c%05d%05d%03d%02d",
          fix.gps_valid ? 'A' : 'V',
          NormalizeIGCAltitude(fix.pressure_altitude),
          NormalizeIGCAltitude(fix.gps_altitude),
          epe, satellites);

  WriteLine(b_record);
  Flush();
}

void
IGCWriter::LogPoint(const NMEAInfo& gps_info)
{
  if (fix.Apply(gps_info))
    LogPoint(fix, (int)GetEPE(gps_info.gps), GetSIU(gps_info.gps));
}

void
IGCWriter::LogEvent(const BrokenTime &time, const char *event)
{
  char e_record[30];
  sprintf(e_record, "E%02d%02d%02d%s",
          time.hour, time.minute, time.second, event);

  WriteLine(e_record);
}

void
IGCWriter::LogEvent(const IGCFix &fix, int epe, int satellites,
                    const char *event)
{
  LogEvent(fix.time, event);

  // tech_spec_gnss.pdf says we need a B record immediately after an E record
  LogPoint(fix, epe, satellites);
}

void
IGCWriter::LogEvent(const NMEAInfo &gps_info, const char *event)
{
  LogEvent(gps_info.date_time_utc, event);

  // tech_spec_gnss.pdf says we need a B record immediately after an E record
  LogPoint(gps_info);
}

void
IGCWriter::LogEmptyFRecord(const BrokenTime &time)
{
  char f_record[32];
  sprintf(f_record, "F%02u%02u%02u", time.hour, time.minute, time.second);
  WriteLine(f_record);
}

void
IGCWriter::LogFRecord(const BrokenTime &time, const int *satellite_ids)
{
  char f_record[32];
  sprintf(f_record, "F%02u%02u%02u", time.hour, time.minute, time.second);

  for (unsigned i = 0, length = 7; i < GPSState::MAXSATELLITES; ++i) {
    if (satellite_ids[i] > 0) {
      sprintf(f_record + length, "%02d", satellite_ids[i]);
      length += 2;
    }
  }

  WriteLine(f_record);
}

void
IGCWriter::Sign()
{
  grecord.FinalizeBuffer();
  grecord.WriteTo(buffered);
}
