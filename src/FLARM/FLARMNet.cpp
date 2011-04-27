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

#include "FLARM/FLARMNet.hpp"
#include "Util/StringUtil.hpp"
#include "IO/LineReader.hpp"
#include "IO/FileLineReader.hpp"

#include <stdio.h>
#include <stdlib.h>

FLARMNetDatabase::~FLARMNetDatabase()
{
  for (iterator i = begin(); i != end(); ++i)
    delete i->second;
}

/**
 * Decodes the FLARMnet.org file and puts the wanted
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
  for (int z = 0; z < bytesToRead; z += 2) {
    char tmp[3];
    tmp[0] = bytes[z];
    tmp[1] = bytes[z+1];
    tmp[2] = 0;

    *curChar = (unsigned char)strtoul(tmp, NULL, 16);
    curChar++;
  }

  *curChar = 0;

  // Trim the string of any additional spaces
  TrimRight(res);
}

/**
 * Reads next FLARMnet.org file entry and saves it
 * into the given record
 * @param file File handle
 * @param record Pointer to the FLARMNetRecord to be filled
 */
static void
LoadRecord(const char *line, FLARMNetRecord *record)
{
  if (strlen(line) < 172)
    return;

  LoadString(line, 6, record->id);
  LoadString(line + 12, 21, record->name);
  LoadString(line + 54, 21, record->airfield);
  LoadString(line + 96, 21, record->type);
  LoadString(line + 138, 7, record->reg);
  LoadString(line + 152, 3, record->cn);
  LoadString(line + 158, 7, record->freq);

  int i = 0;
  int maxSize = sizeof(record->cn) / sizeof(TCHAR);
  while(record->cn[i] != 0 && i < maxSize) {
    if (record->cn[i] == 32)
      record->cn[i] = 0;

    i++;
  }
}

unsigned
FLARMNetDatabase::LoadFile(NLineReader &reader)
{
  /* skip first line */
  const char *line = reader.read();
  if (line == NULL)
    return 0;

  int itemCount = 0;
  while ((line = reader.read()) != NULL) {
    FLARMNetRecord *record = new FLARMNetRecord;

    LoadRecord(line, record);

    insert(value_type(record->GetId(), record));

    itemCount++;
  };

  return itemCount;
}

/**
 * Reads the FLARMnet.org file and fills the map
 *
 * @param path the path of the file
 * @return the number of records read from the file
 */
unsigned
FLARMNetDatabase::LoadFile(const TCHAR *path)
{
  FileLineReaderA file(path);
  if (file.error())
    return 0;

  return LoadFile(file);
}

/**
 * Finds a FLARMNetRecord object based on the given FLARM id
 * @param id FLARM id
 * @return FLARMNetRecord object
 */
const FLARMNetRecord *
FLARMNetDatabase::Find(FlarmId id) const
{
  const_iterator i = find(id);
  if (i != end())
    return i->second;

  return NULL;
}

/**
 * Finds a FLARMNetRecord object based on the given Callsign
 * @param cn Callsign
 * @return FLARMNetRecord object
 */
const FLARMNetRecord *
FLARMNetDatabase::Find(const TCHAR *cn) const
{
  const_iterator i = begin();
  while (i != end()) {
    const FLARMNetRecord *record = (const FLARMNetRecord *)(i->second);
    if (_tcscmp(record->cn, cn) == 0)
      return record;

    i++;
  }

  return NULL;
}

unsigned
FLARMNetDatabase::Find(const TCHAR *cn, const FLARMNetRecord *array[], unsigned size) const
{
  unsigned count = 0;

  const_iterator i = begin();
  while (i != end() && count < size) {
    const FLARMNetRecord *record = (const FLARMNetRecord *)(i->second);
    if (_tcscmp(record->cn, cn) == 0) {
      array[count] = record;
      count++;
    }

    i++;
  }

  return count;
}

FlarmId
FLARMNetRecord::GetId() const
{
  FlarmId id;
  id.parse(this->id, NULL);
  return id;
};
