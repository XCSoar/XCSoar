// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NOAADownloader.hpp"
#include "METAR.hpp"
#include "TAF.hpp"
#include "net/http/Progress.hpp"
#include "lib/curl/Easy.hxx"
#include "lib/curl/CoRequest.hxx"
#include "lib/curl/Setup.hxx"
#include "co/Task.hxx"
#include "util/StringStrip.hxx"

#include <cstdlib>

static Co::Task<Curl::CoResponse>
CoGet(CurlGlobal &curl, const char *url, ProgressListener &progress)
{
  CurlEasy easy{url};
  Curl::Setup(easy);
  const Net::ProgressAdapter progress_adapter{easy, progress};
  easy.SetFailOnError();

  // TODO limit the response body size
  co_return co_await Curl::CoRequest(curl, std::move(easy));
}

namespace NOAADownloader {

/**
 * Tries to parse a date and time from the buffer
 * @param buffer Buffer to parse
 * @param dest BrokenDateTime to write the parsed results in
 * @return Same as buffer if parsing failed,
 * otherwise the pointer to the next character after the parsed string portion
 */
static const char *
ParseDateTime(const char *buffer, BrokenDateTime &dest);

static bool
ParseDecodedDateTime(const char *buffer, BrokenDateTime &dest);

} // namespace NOAADownloader

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

Co::Task<METAR>
NOAADownloader::DownloadMETAR(const char *code, CurlGlobal &curl,
                              ProgressListener &progress)
{
#ifndef NDEBUG
  assert(strlen(code) == 4);
  for (unsigned i = 0; i < 4; i++)
    assert((code[i] >= 'A' && code[i] <= 'Z') ||
           (code[i] >= '0' && code[i] <= '9'));
#endif

  // Build file url
  char url[256];
  snprintf(url, sizeof(url),
           "https://tgftp.nws.noaa.gov/data/observations/metar/decoded/%s.TXT",
           code);

  // Request the file
  auto response = co_await CoGet(curl, url, progress);

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

  char *p = response.body.data();

  // Skip characters until line feed or string end
  while (*p != '\n' && *p != 0)
    p++;

  if (*p == 0)
    throw std::runtime_error{"Malformed METAR text"};

  // Skip characters until slash or string end
  while (*p != '/' && *p != 0)
    p++;

  if (*p == 0)
    throw std::runtime_error{"Malformed METAR text"};

  p++;

  METAR metar;

  if (*p == 0 || !ParseDecodedDateTime(p, metar.last_update))
    throw std::runtime_error{"Malformed METAR time stamp"};

  if (BrokenDateTime::NowUTC() - metar.last_update > std::chrono::hours{24})
    throw std::runtime_error{"METAR is too old"};

  // Search for line feed followed by "ob:"
  char *ob = strstr(p, "\nob:");
  if (ob == NULL)
    throw std::runtime_error{"Malformed METAR text"};

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

  metar.content.SetASCII(ob);
  metar.decoded.SetASCII(response.body);

  // Trim the content strings
  StripRight(metar.content.buffer());
  StripRight(metar.decoded.buffer());

  co_return metar;
}

Co::Task<TAF>
NOAADownloader::DownloadTAF(const char *code, CurlGlobal &curl,
                            ProgressListener &progress)
{
#ifndef NDEBUG
  assert(strlen(code) == 4);
  for (unsigned i = 0; i < 4; i++)
    assert((code[i] >= 'A' && code[i] <= 'Z') ||
           (code[i] >= '0' && code[i] <= '9'));
#endif

  // Build file url
  char url[256];
  snprintf(url, sizeof(url),
           "https://tgftp.nws.noaa.gov/data/forecasts/taf/stations/%s.TXT",
           code);

  // Request the file
  auto response = co_await CoGet(curl, url, progress);

  const char *p = response.body.c_str();

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

  TAF taf;

  // Parse date and time of last update
  const char *q = ParseDateTime(p, taf.last_update);
  if (q == p)
    throw std::runtime_error{"Malformed TAF time stamp"};

  p = q;

  if (BrokenDateTime::NowUTC() - taf.last_update > std::chrono::hours{2*24})
    throw std::runtime_error{"TAF is too old"};

  // Skip characters until line feed or string end
  while (*p != '\n' && *p != 0)
    p++;

  if (*p == 0)
    throw std::runtime_error{"Malformed TAF text"};

  // p is now at the first character after the line feed
  p++;

  if (*p == 0)
    throw std::runtime_error{"Malformed TAF text"};

  // Read rest of the response into the content string
  taf.content.SetASCII(p);

  // Trim the content string
  StripRight(taf.content.buffer());

  co_return taf;
}
