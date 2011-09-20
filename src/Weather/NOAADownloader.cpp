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

#include "NOAADownloader.hpp"
#include "METAR.hpp"
#include "TAF.hpp"
#include "Net/Session.hpp"
#include "Net/ToBuffer.hpp"
#include "OS/PathName.hpp"
#include "Util/StringUtil.hpp"

#include <cstdlib>

namespace NOAADownloader
{
  /**
   * Tries to parse a date and time from the buffer
   * @param buffer Buffer to parse
   * @param dest BrokenDateTime to write the parsed results in
   * @return Same as buffer if parsing failed,
   * otherwise the pointer to the next character after the parsed string portion
   */
  const char *ParseDateTime(const char *buffer, BrokenDateTime &dest);
  bool ParseDecodedDateTime(const char *buffer, BrokenDateTime &dest);

  void AppendToContentString(const char *buffer, METAR::ContentString &content);
}

const char *
NOAADownloader::ParseDateTime(const char *buffer, BrokenDateTime &dest)
{
  char *p_end, *p;
  BrokenDateTime dt;
  long unsigned tmp;

  // Parse year
  tmp = strtoul(buffer, &p_end, 10);
  if (buffer == p_end || *p_end != '/')
    return buffer;

  dt.year = tmp;
  p = p_end + 1;

  // Parse month
  tmp = strtoul(p, &p_end, 10);
  if (p == p_end || *p_end != '/')
    return buffer;

  dt.month = tmp;
  p = p_end + 1;

  // Parse day
  tmp = strtoul(p, &p_end, 10);
  if (p == p_end || *p_end != ' ')
    return buffer;

  dt.day = tmp;
  p = p_end + 1;

  // Parse hour
  tmp = strtoul(p, &p_end, 10);
  if (p == p_end || *p_end != ':')
    return buffer;

  dt.hour = tmp;
  p = p_end + 1;

  // Parse minute
  tmp = strtoul(p, &p_end, 10);
  if (p == p_end)
    return buffer;

  dt.minute = tmp;
  dt.second = 0;

  dest = dt;
  return p_end;
}

bool
NOAADownloader::ParseDecodedDateTime(const char *buffer, BrokenDateTime &dest)
{
  char *p_end, *p;
  BrokenDateTime dt;
  long unsigned tmp;

  // Parse year
  tmp = strtoul(buffer, &p_end, 10);
  if (buffer == p_end || *p_end != '.')
    return false;

  dt.year = tmp;
  p = p_end + 1;

  // Parse month
  tmp = strtoul(p, &p_end, 10);
  if (p == p_end || *p_end != '.')
    return false;

  dt.month = tmp;
  p = p_end + 1;

  // Parse day
  tmp = strtoul(p, &p_end, 10);
  if (p == p_end || *p_end != ' ')
    return false;

  dt.day = tmp;
  p = p_end + 1;

  // Parse hour and minute
  tmp = strtoul(p, &p_end, 10);
  if (p == p_end)
    return false;

  dt.second = 0;
  dt.minute = tmp % 100;
  dt.hour = (tmp - dt.minute) / 100;

  dest = dt;
  return true;
}

void
NOAADownloader::AppendToContentString(const char *buffer,
                                      METAR::ContentString &content)
{
#ifdef _UNICODE
  int length = strlen(buffer);
  TCHAR *buffer2 = new TCHAR[length + 1];
  length = MultiByteToWideChar(CP_UTF8, 0, buffer, length, buffer2, length);
  buffer2[length] = _T('\0');
#else
  const char *buffer2 = buffer;
#endif

  content += buffer2;

#ifdef _UNICODE
  delete[] buffer2;
#endif
}

bool
NOAADownloader::DownloadMETAR(const char *code, METAR &metar,
                              JobRunner &runner)
{
#ifndef NDEBUG
  assert(strlen(code) == 4);
  for (unsigned i = 0; i < 4; i++)
    assert(code[i] >= 'A' && code[i] <= 'Z');
#endif

  // Build file url
  char url[256] = "http://weather.noaa.gov/pub/data/observations/metar/decoded/";
  strcat(url, code);
  strcat(url, ".TXT");
  PathName path(url);

  // Open download session
  Net::Session session;
  if (session.Error())
    return false;

  // Request the file
  char buffer[4096];
  Net::DownloadToBufferJob job(session, path, buffer, sizeof(buffer) - 1);
  if (!runner.Run(job) || job.GetLength() < 0)
    return false;

  buffer[job.GetLength()] = 0;

  /*
   * Example:
   *
   * Duesseldorf, Germany (EDDL) 51-18N 006-46E 41M
   * Sep 20, 2011 - 03:50 PM EDT / 2011.09.20 1950 UTC
   * Wind: from the SW (220 degrees) at 10 MPH (9 KT):0
   * Visibility: greater than 7 mile(s):0
   * Sky conditions: mostly cloudy
   * Temperature: 60 F (16 C)
   * Dew Point: 51 F (11 C)
   * Relative Humidity: 72%
   * Pressure (altimeter): 30.21 in. Hg (1023 hPa)
   * ob: EDDL 201950Z 22009KT 9999 FEW035 BKN038 16/11 Q1023 NOSIG
   * cycle: 20
   */

  char *p = buffer;

  // Skip characters until line feed or string end
  while (*p != '\n' && *p != 0)
    p++;

  if (*p == 0)
    return false;

  // Skip characters until slash or string end
  while (*p != '/' && *p != 0)
    p++;

  if (*p == 0)
    return false;

  p++;

  if (*p == 0 || !ParseDecodedDateTime(p, metar.last_update))
    return false;

  // Search for line feed followed by "ob:"
  char *ob = strstr(p, "\nob:");
  if (ob == NULL)
    return false;

  *ob = 0;

  ob += 4;
  while (*ob == ' ' && *ob != 0)
    ob++;

  p = ob;
  // Skip characters until line feed or string end
  while (*p != '\n' && *p != '\r' && *p != 0)
    p++;

  if (*p != 0)
    *p = 0;

  metar.content.clear();
  AppendToContentString(ob, metar.content);

  metar.decoded.clear();
  AppendToContentString(buffer, metar.decoded);

  // Trim the content strings
  TrimRight(metar.content.buffer());
  TrimRight(metar.decoded.buffer());

  return true;
}

bool
NOAADownloader::DownloadTAF(const char *code, TAF &taf,
                            JobRunner &runner)
{
#ifndef NDEBUG
  assert(strlen(code) == 4);
  for (unsigned i = 0; i < 4; i++)
    assert(code[i] >= 'A' && code[i] <= 'Z');
#endif

  // Build file url
  char url[256] = "http://weather.noaa.gov/pub/data/forecasts/taf/stations/";
  strcat(url, code);
  strcat(url, ".TXT");
  PathName path(url);

  // Open download session
  Net::Session session;
  if (session.Error())
    return false;

  // Request the file
  char buffer[4096];
  Net::DownloadToBufferJob job(session, path, buffer, sizeof(buffer) - 1);
  if (!runner.Run(job) || job.GetLength() < 0)
    return false;

  buffer[job.GetLength()] = 0;

  /*
   * Example:
   *
   * 2011/07/01 12:27
   * TAF EDDL 011100Z 0112/0218 32010KT 9999 SCT040
   *       TEMPO 0112/0119 4000 SHRA SCT030CB PROB30
   *       TEMPO 0112/0118 32015G30KT TSRA
   *       BECMG 0118/0121 32005KT PROB30
   *       TEMPO 0202/0207 BKN012
   *       BECMG 0210/0213 31010KT
   */

  // Parse date and time of last update
  const char *p = ParseDateTime(buffer, taf.last_update);
  if (p == buffer)
    return false;

  // Skip characters until line feed or string end
  while (*p != '\n' && *p != 0)
    p++;

  if (*p == 0)
    return false;

  // p is now at the first character after the line feed
  p++;

  if (*p == 0)
    return false;

  // Read rest of the response into the content string
  taf.content.clear();
  AppendToContentString(p, taf.content);

  // Trim the content string
  TrimRight(taf.content.buffer());

  return true;
}
