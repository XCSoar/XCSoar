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

#include "NOAAStore.hpp"
#include "NOAADownloader.hpp"
#include "METARParser.hpp"
#include "ParsedMETAR.hpp"
#include "Util/Macros.hpp"

#ifdef _UNICODE
#include <windows.h>
#endif

#ifdef _UNICODE

const TCHAR *
NOAAStore::Item::GetCodeT() const
{
  static TCHAR code2[ARRAY_SIZE(Item::code)];
  if (MultiByteToWideChar(CP_UTF8, 0, code, -1, code2, ARRAY_SIZE(code2)) <= 0)
    return _T("");

  return code2;
}

#endif

bool
NOAAStore::Item::Update(JobRunner &runner)
{
  bool metar_downloaded = NOAADownloader::DownloadMETAR(code, metar, runner);
  if (metar_downloaded) {
    metar_available = true;

    if (METARParser::Parse(metar, parsed_metar))
      parsed_metar_available = true;
  }

  bool taf_downloaded = NOAADownloader::DownloadTAF(code, taf, runner);
  if (taf_downloaded)
    taf_available = true;

  return metar_downloaded && taf_downloaded;
}

bool
NOAAStore::IsValidCode(const char *code)
{
  for (unsigned i = 0; i < 4; ++i)
    if (! ((code[i] >= 'A' && code[i] <= 'Z') ||
           (code[i] >= '0' && code[i] <= '9')))
      return false;

  if (code[4] != 0)
    return false;

  return true;
}

#ifdef _UNICODE
bool
NOAAStore::IsValidCode(const TCHAR* code)
{
  for (unsigned i = 0; i < 4; ++i)
    if (! ((code[i] >= _T('A') && code[i] <= _T('Z')) ||
           (code[i] >= _T('0') && code[i] <= _T('9'))))
      return false;

  if (code[4] != _T('\0'))
    return false;

  return true;
}
#endif

NOAAStore::iterator
NOAAStore::AddStation(const char *code)
{
  assert(IsValidCode(code));

  Item item;

  // Copy station code
  strncpy(item.code, code, 4);
  item.code[4] = 0;

  // Reset available flags
  item.metar_available = false;
  item.parsed_metar_available = false;
  item.taf_available = false;

  stations.push_back(item);
  return --end();
}

#ifdef _UNICODE
NOAAStore::iterator
NOAAStore::AddStation(const TCHAR *code)
{
  assert(IsValidCode(code));

  size_t len = _tcslen(code);
  char code2[len * 4 + 1];
  ::WideCharToMultiByte(CP_UTF8, 0, code, len, code2, sizeof(code2), NULL, NULL);
  code2[4] = 0;

  return AddStation(code2);
}
#endif

bool
NOAAStore::Update(JobRunner &runner)
{
  bool result = true;
  for (auto i = begin(), e = end(); i != e; ++i)
    result = i->Update(runner) && result;

  return result;
}
