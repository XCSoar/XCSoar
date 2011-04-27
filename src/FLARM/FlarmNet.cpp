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

#include "FLARM/FlarmNet.hpp"
#include "Util/StringUtil.hpp"
#include "Util/CharUtil.hpp"
#include "IO/LineReader.hpp"
#include "IO/FileLineReader.hpp"

#include <stdio.h>
#include <stdlib.h>

FlarmNetDatabase::~FlarmNetDatabase()
{
  for (RecordMap::iterator i = record_map.begin(); i != record_map.end(); ++i)
    delete i->second;
}

/**
 * Decodes the FlarmNet.org file and puts the wanted
 * characters into the res pointer
 * @param file File handle
 * @param charCount Number of character to decode
 * @param res Pointer to be written in
 */
static void
LoadString(const char *bytes, int charCount, TCHAR *res)
{
  int bytesToRead = charCount * 2;

  TCHAR *curChar = res;

  char tmp[3];
  tmp[2] = 0;
  for (int z = 0; z < bytesToRead; z += 2) {
    tmp[0] = bytes[z];
    tmp[1] = bytes[z+1];

    *curChar = (unsigned char)strtoul(tmp, NULL, 16);
    curChar++;
  }

  *curChar = 0;

  // Trim the string of any additional spaces
  TrimRight(res);
}

/**
 * Reads next FlarmNet.org file entry and returns the pointer to
 * a new FlarmNetRecord instance or NULL on error.
 *
 * The caller is responsible for deleting the object again!
 */
static FlarmNetRecord *
LoadRecord(const char *line)
{
  if (strlen(line) < 172)
    return NULL;

  FlarmNetRecord *record = new FlarmNetRecord;
  LoadString(line, 6, record->id);
  LoadString(line + 12, 21, record->pilot);
  LoadString(line + 54, 21, record->airfield);
  LoadString(line + 96, 21, record->plane_type);
  LoadString(line + 138, 7, record->registration);
  LoadString(line + 152, 3, record->callsign);
  LoadString(line + 158, 7, record->frequency);

  // Terminate callsign string on first whitespace
  int maxSize = sizeof(record->callsign) / sizeof(TCHAR);
  for (int i = 0; record->callsign[i] != 0 && i < maxSize; i++)
    if (IsWhitespaceOrNull(record->callsign[i]))
      record->callsign[i] = 0;

  return record;
}

unsigned
FlarmNetDatabase::LoadFile(NLineReader &reader)
{
  /* skip first line */
  const char *line = reader.read();
  if (line == NULL)
    return 0;

  int itemCount = 0;
  while ((line = reader.read()) != NULL) {
    FlarmNetRecord *record = LoadRecord(line);
    if (record != NULL) {
      record_map[record->GetId()] = record;
      itemCount++;
    }
  }

  return itemCount;
}

unsigned
FlarmNetDatabase::LoadFile(const TCHAR *path)
{
  FileLineReaderA file(path);
  if (file.error())
    return 0;

  return LoadFile(file);
}

const FlarmNetRecord *
FlarmNetDatabase::Find(FlarmId id) const
{
  RecordMap::const_iterator i = record_map.find(id);
  if (i != record_map.end())
    return i->second;

  return NULL;
}

const FlarmNetRecord *
FlarmNetDatabase::FindFirst(const TCHAR *cn) const
{
  RecordMap::const_iterator i = record_map.begin();
  while (i != record_map.end()) {
    const FlarmNetRecord *record = (const FlarmNetRecord *)(i->second);
    if (_tcscmp(record->callsign, cn) == 0)
      return record;

    i++;
  }

  return NULL;
}

unsigned
FlarmNetDatabase::Find(const TCHAR *cn, const FlarmNetRecord *array[], unsigned size) const
{
  unsigned count = 0;

  RecordMap::const_iterator i = record_map.begin();
  while (i != record_map.end() && count < size) {
    const FlarmNetRecord *record = (const FlarmNetRecord *)(i->second);
    if (_tcscmp(record->callsign, cn) == 0) {
      array[count] = record;
      count++;
    }

    i++;
  }

  return count;
}

FlarmId
FlarmNetRecord::GetId() const
{
  FlarmId id;
  id.parse(this->id, NULL);
  return id;
};
