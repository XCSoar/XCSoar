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
#include "FLARM/FlarmId.hpp"
#include "Util/StringUtil.hpp"
#include "Util/CharUtil.hpp"
#include "IO/LineReader.hpp"
#include "IO/FileLineReader.hpp"

#include <stdio.h>
#include <stdlib.h>

namespace FlarmNet
{
  typedef std::map<FlarmId, Record*> RecordMap;
  RecordMap record_map;
}

void
FlarmNet::Destroy()
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
static FlarmNet::Record *
LoadRecord(const char *line)
{
  if (strlen(line) < 172)
    return NULL;

  FlarmNet::Record *record = new FlarmNet::Record;
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
FlarmNet::LoadFile(NLineReader &reader)
{
  // Clear database before adding new entries
  Destroy();

  /* skip first line */
  const char *line = reader.read();
  if (line == NULL)
    return 0;

  int itemCount = 0;
  while ((line = reader.read()) != NULL) {
    FlarmNet::Record *record = LoadRecord(line);
    if (record != NULL) {
      record_map[record->GetId()] = record;
      itemCount++;
    }
  }

  return itemCount;
}

unsigned
FlarmNet::LoadFile(const TCHAR *path)
{
  FileLineReaderA file(path);
  if (file.error())
    return 0;

  return LoadFile(file);
}

const FlarmNet::Record *
FlarmNet::FindRecordById(FlarmId id)
{
  RecordMap::const_iterator i = record_map.find(id);
  if (i != record_map.end())
    return i->second;

  return NULL;
}

const FlarmNet::Record *
FlarmNet::FindFirstRecordByCallSign(const TCHAR *cn)
{
  RecordMap::const_iterator i = record_map.begin();
  while (i != record_map.end()) {
    const Record *record = (const Record *)(i->second);
    if (_tcscmp(record->callsign, cn) == 0)
      return record;

    i++;
  }

  return NULL;
}

unsigned
FlarmNet::FindRecordsByCallSign(const TCHAR *cn, const Record *array[], unsigned size)
{
  unsigned count = 0;

  RecordMap::const_iterator i = record_map.begin();
  while (i != record_map.end() && count < size) {
    const Record *record = (const Record *)(i->second);
    if (_tcscmp(record->callsign, cn) == 0) {
      array[count] = record;
      count++;
    }

    i++;
  }

  return count;
}

unsigned
FlarmNet::FindIdsByCallSign(const TCHAR *cn, const FlarmId *array[], unsigned size)
{
  unsigned count = 0;

  RecordMap::const_iterator i = record_map.begin();
  while (i != record_map.end() && count < size) {
    const Record *record = (const Record *)(i->second);
    if (_tcscmp(record->callsign, cn) == 0) {
      array[count] = (const FlarmId *)(&(i->first));
      count++;
    }

    i++;
  }

  return count;
}

FlarmId
FlarmNet::Record::GetId() const
{
  FlarmId id;
  id.Parse(this->id, NULL);
  return id;
};
