/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "NMEA/Info.hpp"
#include "Version.hpp"

#include <assert.h>
#include <windef.h> /* for MAX_PATH */

static char *
FormatIGCLocation(char *buffer, const GeoPoint &location)
{
  char latitude_suffix = negative(location.latitude.Native())
    ? 'S' : 'N';
  unsigned latitude =
    (unsigned)uround(fabs(location.latitude.Degrees() * 60000));

  char longitude_suffix = negative(location.longitude.Native())
    ? 'W' : 'E';
  unsigned longitude =
    (unsigned)uround(fabs(location.longitude.Degrees() * 60000));

  sprintf(buffer, "%02u%05u%c%03u%05u%c",
          latitude / 60000, latitude % 60000, latitude_suffix,
          longitude / 60000, longitude % 60000, longitude_suffix);

  return buffer + strlen(buffer);
}

IGCWriter::IGCWriter(const TCHAR *path)
  :file(path)
{
  fix.Clear();

  grecord.Initialize();
}

bool
IGCWriter::CommitLine(char *line)
{
  if (!file.WriteLine(line))
    return false;

  grecord.AppendRecordToBuffer(line);
  return true;
}

bool
IGCWriter::WriteLine(const char *line)
{
  assert(strchr(line, '\r') == NULL);
  assert(strchr(line, '\n') == NULL);

  char *const dest = BeginLine();
  if (dest == nullptr)
    return false;

  char *const end = dest + MAX_IGC_BUFF - 1, *p = dest;

  p = CopyIGCString(dest, end, line);
  *p = '\0';

  return CommitLine(dest);
}

bool
IGCWriter::WriteLine(const char *a, const TCHAR *b)
{
  size_t a_length = strlen(a);
  assert(a_length < MAX_IGC_BUFF);

  char *const dest = BeginLine();
  if (dest == nullptr)
    return false;

  char *const end = dest + MAX_IGC_BUFF - 1, *p = dest;

  p = std::copy(a, a + a_length, p);
  p = CopyIGCString(p, end, b);
  *p = '\0';

  return CommitLine(dest);
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
   * HFDTM100GPSDATUM:WGS84
   * HFRFWFIRMWAREVERSION:3.6
   * HFRHWHARDWAREVERSION:3.4
   * HFFTYFR TYPE:GARRECHT INGENIEURGESELLSCHAFT,VOLKSLOGGER 1.0
   * HFCIDCOMPETITIONID:WUE
   * HFCCLCOMPETITIONCLASS:FAI
   */

  assert(date_time.Plausible());
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

  WriteLine("HFPLTPILOT:", pilot_name);
  WriteLine("HFGTYGLIDERTYPE:", aircraft_model);
  WriteLine("HFGIDGLIDERID:", aircraft_registration);
  WriteLine("HFCIDCOMPETITIONID:", competition_id);
  WriteLine("HFFTYFRTYPE:XCSOAR,XCSOAR ", XCSoar_VersionStringOld);
  WriteLine("HFGPS:", driver_name);

  WriteLine("HFDTM100DATUM:WGS-84");

  WriteLine(GetIRecord());
}

void
IGCWriter::StartDeclaration(const BrokenDateTime &date_time,
                            const int number_of_turnpoints)
{
  assert(date_time.Plausible());

  // IGC GNSS specification 3.6.1
  char buffer[100];
  sprintf(buffer, "C%02u%02u%02u%02u%02u%02u0000000000%02d",
          // DD  MM  YY  HH  MM  SS  DD  MM  YY IIII TT
          date_time.day,
          date_time.month,
          date_time.year % 100,
          date_time.hour,
          date_time.minute,
          date_time.second,
          number_of_turnpoints - 2);

  WriteLine(buffer);

  // takeoff line
  // IGC GNSS specification 3.6.3
  WriteLine("C0000000N00000000ETAKEOFF");
}

void
IGCWriter::EndDeclaration()
{
  // TODO bug: this is causing problems with some analysis software
  // maybe it's because the date and location fields are bogus
  WriteLine("C0000000N00000000ELANDING");
}

void
IGCWriter::AddDeclaration(const GeoPoint &location, const TCHAR *id)
{
  char c_record[500];

  char *p = c_record;
  *p++ = 'C';
  p = FormatIGCLocation(p, location);
  CopyASCIIUppper(p, id);

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
  char b_record[500];
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
  char f_record[64];
  sprintf(f_record, "F%02u%02u%02u", time.hour, time.minute, time.second);
  WriteLine(f_record);
}

void
IGCWriter::LogFRecord(const BrokenTime &time, const int *satellite_ids)
{
  char f_record[64];
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
  assert(file.IsOpen());

  grecord.FinalizeBuffer();
  grecord.WriteTo(file);
}
