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

#include "NOAAStore.hpp"
#include "NOAADownloader.hpp"
#include "METARParser.hpp"
#include "METAR.hpp"
#include "ParsedMETAR.hpp"
#include "TAF.hpp"
#include "Util/StaticArray.hpp"

#ifdef _UNICODE
#include <windows.h>
#endif

namespace NOAAStore
{
  struct Item
  {
    char code[5];

    bool metar_available;
    bool taf_available;

    METAR metar;
    TAF taf;
  };

  typedef StaticArray<Item, 20> StationArray;

  StationArray stations;

  /**
   * Searches the given station code in the array
   * @param code Four letter code of the station/airport (upper case)
   * @return If found index of the station, otherwise (unsigned)-1
   */
  int GetStationIndex(const char *code);
}

int
NOAAStore::GetStationIndex(const char *code)
{
#ifndef NDEBUG
  assert(strlen(code) == 4);
  for (unsigned i = 0; i < 4; i++)
    assert(code[i] >= 'A' && code[i] <= 'Z');
#endif

  unsigned len = stations.size();
  for (unsigned i = 0; i < len; i++)
    if (strcmp(code, stations[i].code) == 0)
      return i;

  return (unsigned)-1;
}

bool
NOAAStore::AddStation(const char *code)
{
#ifndef NDEBUG
  assert(strlen(code) == 4);
  for (unsigned i = 0; i < 4; i++)
    assert(code[i] >= 'A' && code[i] <= 'Z');
#endif

  if (stations.full())
    return false;

  Item &item = stations.append();

  // Copy station code
  strncpy(item.code, code, 4);
  item.code[4] = 0;

  // Reset available flags
  item.metar_available = false;
  item.taf_available = false;

  return true;
}

#ifdef _UNICODE
bool
NOAAStore::AddStation(const TCHAR *code)
{
#ifndef NDEBUG
  assert(_tcslen(code) == 4);
  for (unsigned i = 0; i < 4; i++)
    assert(code[i] >= _T('A') && code[i] <= _T('Z'));
#endif

  size_t len = _tcslen(code);
  char code2[len * 4 + 1];
  ::WideCharToMultiByte(CP_UTF8, 0, code, len, code2, sizeof(code2), NULL, NULL);
  code2[4] = 0;

  return AddStation(code2);
}
#endif

void
NOAAStore::RemoveStation(unsigned index)
{
  assert(index < stations.size());
  stations.quick_remove(index);
}

void
NOAAStore::RemoveStation(const char *code)
{
#ifndef NDEBUG
  assert(strlen(code) == 4);
  for (unsigned i = 0; i < 4; i++)
    assert(code[i] >= 'A' && code[i] <= 'Z');
#endif

  unsigned index = GetStationIndex(code);
  if (index == (unsigned)-1)
    return;

  RemoveStation(index);
}

const char *
NOAAStore::GetCode(unsigned index)
{
  assert(index < stations.size());
  return stations[index].code;
}

const TCHAR *
NOAAStore::GetCodeT(unsigned index)
{
#ifdef _UNICODE
  const char *code = GetCode(index);

  int length = strlen(code);
  TCHAR *code2 = new TCHAR[length + 1];
  length = MultiByteToWideChar(CP_UTF8, 0, code, length, code2, length);
  code2[length] = _T('\0');

  return code2;
#else
  return GetCode(index);
#endif
}

bool
NOAAStore::GetMETAR(unsigned index, METAR &metar)
{
  assert(index < stations.size());
  if (!stations[index].metar_available)
    return false;

  metar = stations[index].metar;
  return true;
}

bool
NOAAStore::GetMETAR(const char *code, METAR &metar)
{
#ifndef NDEBUG
  assert(strlen(code) == 4);
  for (unsigned i = 0; i < 4; i++)
    assert(code[i] >= 'A' && code[i] <= 'Z');
#endif

  unsigned index = GetStationIndex(code);
  if (index == (unsigned)-1)
    return false;

  return GetMETAR(index, metar);
}

bool
NOAAStore::GetTAF(unsigned index, TAF &taf)
{
  assert(index < stations.size());
  if (!stations[index].taf_available)
    return false;

  taf = stations[index].taf;
  return true;
}

bool
NOAAStore::GetTAF(const char *code, TAF &taf)
{
#ifndef NDEBUG
  assert(strlen(code) == 4);
  for (unsigned i = 0; i < 4; i++)
    assert(code[i] >= 'A' && code[i] <= 'Z');
#endif

  unsigned index = GetStationIndex(code);
  if (index == (unsigned)-1)
    return false;

  return GetTAF(index, taf);
}

bool
NOAAStore::UpdateStation(unsigned index, JobRunner &runner)
{
  assert(index < stations.size());
  const char *code = stations[index].code;

  bool metar_downloaded =
    NOAADownloader::DownloadMETAR(code, stations[index].metar, runner);
  if (metar_downloaded)
    stations[index].metar_available = true;

  bool taf_downloaded =
    NOAADownloader::DownloadTAF(code, stations[index].taf, runner);
  if (taf_downloaded)
    stations[index].taf_available = true;

  return metar_downloaded && taf_downloaded;
}

bool
NOAAStore::UpdateStation(const char *code, JobRunner &runner)
{
#ifndef NDEBUG
  assert(strlen(code) == 4);
  for (unsigned i = 0; i < 4; i++)
    assert(code[i] >= 'A' && code[i] <= 'Z');
#endif

  unsigned index = GetStationIndex(code);
  if (index == (unsigned)-1)
    return false;

  return UpdateStation(index, runner);
}

bool
NOAAStore::Update(JobRunner &runner)
{
  bool result = true;
  unsigned len = stations.size();
  for (unsigned i = 0; i < len; i++)
    result = UpdateStation(i, runner) && result;

  return result;
}

unsigned
NOAAStore::Count()
{
  return stations.size();
}

bool
NOAAStore::Full()
{
  return stations.full();
}
