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
#include "Net/Request.hpp"
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
}

bool
NOAADownloader::DownloadMETAR(const char *code, METAR &metar)
{
  // Build file url
  char url[256] = "ftp://tgftp.nws.noaa.gov/data/observations/metar/stations/";
  strcat(url, code);
  strcat(url, ".TXT");
  PathName path(url);

  // Open download session
  Net::Session session;
  if (session.Error())
    return false;

  // Request the file
  Net::Request request(session, path);
  if (!request.Created())
    return false;

  /*
   * Example:
   *
   * 2011/07/01 10:20
   * EDDL 011020Z 31004KT 270V340 9999 SCT032TCU SCT050 17/09 Q1022 TEMPO SHRA
   */

  // Read first 256 bytes of the response
  char buffer[256];
  if (!request.Read(buffer, sizeof(buffer)))
    return false;

  // Parse date and time of last update
  const char *p = ParseDateTime(buffer, metar.last_update);
  if (p == buffer)
    return false;

  // Skip characters until line feed or string end
  while (*p != '\n' && *p != 0)
    p++;

  if (*p == 0)
    return false;

  // p is now at the first character after the line feed
  *p++;

  if (*p == 0)
    return false;

  // Read rest of the response into the content string
  metar.content.clear();
  AppendToContentString(p, metar.content);
  while (request.Read(buffer, sizeof(buffer)))
    AppendToContentString(buffer, metar.content);

  // Trim the content string
  TrimRight(metar.content.buffer());

  return true;
}

bool
NOAADownloader::DownloadTAF(const char *code, TAF &taf)
{
  // Build file url
  char url[256] = "ftp://tgftp.nws.noaa.gov/data/forecasts/taf/stations/";
  strcat(url, code);
  strcat(url, ".TXT");
  PathName path(url);

  // Open download session
  Net::Session session;
  if (session.Error())
    return false;

  // Request the file
  Net::Request request(session, path);
  if (!request.Created())
    return false;

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

  // Read first 256 bytes of the response
  char buffer[256];
  if (!request.Read(buffer, sizeof(buffer)))
    return false;

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
  *p++;

  if (*p == 0)
    return false;

  // Read rest of the response into the content string
  taf.content.clear();
  AppendToContentString(p, taf.content);
  while (request.Read(buffer, sizeof(buffer)))
    AppendToContentString(buffer, taf.content);

  // Trim the content string
  TrimRight(taf.content.buffer());

  return true;
}
