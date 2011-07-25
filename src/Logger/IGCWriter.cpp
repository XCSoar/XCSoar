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

#include "Logger/IGCWriter.hpp"
#include "IO/TextWriter.hpp"
#include "NMEA/Info.hpp"
#include "Version.hpp"
#include "Compatibility/string.h"

#include <assert.h>

#ifdef _UNICODE
#include <windows.h>
#endif

const IGCWriter::LogPoint_GPSPosition &
IGCWriter::LogPoint_GPSPosition::operator=(const NMEAInfo &gps_info)
{
  Location = gps_info.location;
  GPSAltitude = (int)gps_info.gps_altitude;
  Initialized = true;

  return *this;
}

static char *
igc_format_location(char *buffer, const GeoPoint &location)
{
  char latitude_suffix = negative(location.Latitude.value_native())
    ? 'S' : 'N';
  unsigned latitude =
    (unsigned)uround(fabs(location.Latitude.value_degrees() * 60000));

  char longitude_suffix = negative(location.Longitude.value_native())
    ? 'W' : 'E';
  unsigned longitude =
    (unsigned)uround(fabs(location.Longitude.value_degrees() * 60000));

  sprintf(buffer, "%02u%05u%c%03u%05u%c",
          latitude / 60000, latitude % 60000, latitude_suffix,
          longitude / 60000, longitude % 60000, longitude_suffix);

  return buffer + strlen(buffer);
}

IGCWriter::IGCWriter(const TCHAR *_path, const NMEAInfo &gps_info)
  :Simulator(gps_info.connected && !gps_info.gps.real)
{
  _tcscpy(path, _path);

  frecord.reset();
  LastValidPoint.Initialized = false;

  if (!Simulator)
    grecord.Init();
}

bool
IGCWriter::flush()
{
  if (buffer.empty())
    return true;

  TextWriter writer(path, true);
  if (writer.error())
    return false;

  for (unsigned i = 0; i < buffer.length(); ++i) {
    if (!writer.writeln(buffer[i]))
      return false;

    grecord.AppendRecordToBuffer(buffer[i]);
  }

  if (!writer.flush())
    return false;

  buffer.clear();
  return true;
}

void
IGCWriter::finish(const NMEAInfo &gps_info)
{
  if (gps_info.connected && !gps_info.gps.real)
    Simulator = true;

  flush();
}

static void
clean(char *p)
{
  for (; *p != 0; ++p)
    if (!GRecord::IsValidIGCChar(*p))
      *p = ' ';
}

bool
IGCWriter::writeln(const char *line)
{
  if (buffer.full() && !flush())
    return false;

  assert(!buffer.full());

  char *dest = buffer.append();
  strncpy(dest, line, MAX_IGC_BUFF);
  dest[MAX_IGC_BUFF - 1] = '\0';

  clean(dest);

  return true;
}

bool
IGCWriter::write_tstring(const char *a, const TCHAR *b)
{
  size_t a_length = strlen(a);
  size_t b_length = _tcslen(b);
  char buffer[a_length + b_length * 4 + 1];
  memcpy(buffer, a, a_length);

#ifdef _UNICODE
  if (b_length > 0) {
    int len = ::WideCharToMultiByte(CP_ACP, 0, b, b_length,
                                    buffer + a_length, b_length * 4,
                                    NULL, NULL);
    if (len <= 0)
      return false;

    a_length += len;
  }

  buffer[a_length] = 0;
#else
  memcpy(buffer + a_length, b, b_length + 1);
#endif

  return writeln(buffer);
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
IGCWriter::header(const BrokenDateTime &DateTime,
                  const TCHAR *pilot_name, const TCHAR *aircraft_model,
                  const TCHAR *aircraft_registration,
                  const TCHAR *strAssetNumber, const TCHAR *driver_name)
{
  assert(DateTime.Plausible());

  char datum[] = "HFDTM100Datum: WGS-84";
  char temp[100];

  // Flight recorder ID number MUST go first..
  sprintf(temp, "AXCS%c%c%c",
          (char)strAssetNumber[0],
          (char)strAssetNumber[1],
          (char)strAssetNumber[2]);
  writeln(temp);

  sprintf(temp, "HFDTE%02u%02u%02u",
          DateTime.day, DateTime.month, DateTime.year % 100);
  writeln(temp);

  if (!Simulator)
    writeln(GetHFFXARecord());

  write_tstring("HFPLTPILOT:", pilot_name);
  write_tstring("HFGTYGLIDERTYPE:", aircraft_model);
  write_tstring("HFGIDGLIDERID:", aircraft_registration);
  write_tstring("HFFTYFR TYPE:XCSOAR,XCSOAR ", XCSoar_VersionStringOld);
  write_tstring("HFGPS: ", driver_name);

  writeln(datum);

  if (!Simulator)
    writeln(GetIRecord());
}

void
IGCWriter::StartDeclaration(const BrokenDateTime &FirstDateTime, const int ntp)
{
  assert(FirstDateTime.Plausible());

  // TODO bug: this is causing problems with some analysis software
  // maybe it's because the date and location fields are bogus
  char start[] = "C0000000N00000000ETAKEOFF";
  char temp[100];

  // JMW added task start declaration line

  // LGCSTKF013945TAKEOFF DETECTED

  // IGC GNSS specification 3.6.1
  sprintf(temp, "C%02u%02u%02u%02u%02u%02u0000000000%02d",
          // DD  MM  YY  HH  MM  SS  DD  MM  YY IIII TT
          FirstDateTime.day,
          FirstDateTime.month,
          FirstDateTime.year % 100,
          FirstDateTime.hour,
          FirstDateTime.minute,
          FirstDateTime.second,
          ntp - 2);

  writeln(temp);
  // takeoff line
  // IGC GNSS specification 3.6.3
  writeln(start);
}

void
IGCWriter::EndDeclaration(void)
{
  // TODO bug: this is causing problems with some analysis software
  // maybe it's because the date and location fields are bogus
  const char start[] = "C0000000N00000000ELANDING";
  writeln(start);
}

void
IGCWriter::AddDeclaration(const GeoPoint &location, const TCHAR *ID)
{
  char szCRecord[500];
  char IDString[MAX_PATH];
  int i;

  TCHAR tmpstring[MAX_PATH];
  _tcscpy(tmpstring, ID);
  _tcsupr(tmpstring);
  for (i = 0; i < (int)_tcslen(tmpstring); i++)
    IDString[i] = (char)tmpstring[i];

  IDString[i] = '\0';

  char *p = szCRecord;
  *p++ = 'C';
  p = igc_format_location(p, location);
  strcpy(p, IDString);

  writeln(szCRecord);
}

void
IGCWriter::LoggerNote(const TCHAR *text)
{
  write_tstring("LPLT", text);
}

/**
 * Applies range checks to the specified altitude value and converts
 * it to an integer suitable for printing in the IGC file.
 */
static int
normalize_igc_altitude(int value)
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
IGCWriter::LogPoint(const NMEAInfo& gps_info)
{
  char szBRecord[500];
  int iSIU = GetSIU(gps_info);
  fixed dEPE = GetEPE(gps_info);
  LogPoint_GPSPosition p;

  char IsValidFix;

  // if at least one GPS fix comes from the simulator, disable signing
  if (gps_info.connected && !gps_info.gps.real)
    Simulator = true;

  if (!Simulator) {
    const char *p = frecord.update(gps_info.gps.satellite_ids,
                                   gps_info.date_time_utc, gps_info.time,
                                   !gps_info.location_available);
    if (p != NULL)
      writeln(p);
  }

  if (!LastValidPoint.Initialized &&
      ((gps_info.gps_altitude < fixed(-100))
       || (gps_info.baro_altitude < fixed(-100))
          || !gps_info.location_available))
    return;


  if (!gps_info.location_available) {
    IsValidFix = 'V'; // invalid
    p = LastValidPoint;
  } else {
    IsValidFix = 'A'; // Active
    // save last active fix location
    p = LastValidPoint = gps_info;
  }

  char *q = szBRecord;
  sprintf(q, "B%02d%02d%02d",
          gps_info.date_time_utc.hour, gps_info.date_time_utc.minute,
          gps_info.date_time_utc.second);
  q += strlen(q);

  q = igc_format_location(q, p.Location);

  sprintf(q, "%c%05d%05d%03d%02d",
          IsValidFix,
          normalize_igc_altitude(gps_info.baro_altitude_available
                                 ? gps_info.baro_altitude
                                 /* fall back to GPS altitude */
                                 : p.GPSAltitude),
          normalize_igc_altitude(p.GPSAltitude),
          (int)dEPE, iSIU);

  writeln(szBRecord);
}

void
IGCWriter::LogEvent(const NMEAInfo &gps_info, const char *event)
{
  char szBRecord[30];
  sprintf(szBRecord,"E%02d%02d%02d%s",
          gps_info.date_time_utc.hour, gps_info.date_time_utc.minute,
          gps_info.date_time_utc.second, event);

  writeln(szBRecord);
  // tech_spec_gnss.pdf says we need a B record immediately after an E record
  LogPoint(gps_info);
}

void
IGCWriter::sign()
{
  if (Simulator)
    return;

  // buffer is appended w/ each igc file write
  grecord.FinalizeBuffer();
  // read record built by individual file writes
  char OldGRecordBuff[MAX_IGC_BUFF];
  grecord.GetDigest(OldGRecordBuff);

  // now calc from whats in the igc file on disk
  grecord.Init();
  grecord.SetFileName(path);
  grecord.LoadFileToBuffer();
  grecord.FinalizeBuffer();
  char NewGRecordBuff[MAX_IGC_BUFF];
  grecord.GetDigest(NewGRecordBuff);

  bool bFileValid = strcmp(OldGRecordBuff, NewGRecordBuff) == 0;
  grecord.AppendGRecordToFile(bFileValid);
}
