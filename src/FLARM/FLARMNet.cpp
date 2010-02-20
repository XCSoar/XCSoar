/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "UtilsText.hpp"

#include <stdio.h>
#include <stdlib.h>

static void
LoadRecord(FILE *file, FLARMNetRecord *record);

/**
 * Reads the FLARMnet.org file and fills the map
 *
 * @param path the path of the file
 * @return the number of records read from the file
 */
unsigned
FLARMNetDatabase::LoadFile(const TCHAR *path)
{
  FILE* hFile = _tfopen(path, _T("rt"));
  if (hFile == NULL)
    return 0;

  long fileLength;

  fseek (hFile , 0 , SEEK_END);
  fileLength = ftell (hFile);
  fseek (hFile , 7 , SEEK_SET);

  int itemCount = 0;
  while(fileLength - ftell(hFile) > 87) {
    FLARMNetRecord *record = new FLARMNetRecord;

    LoadRecord(hFile, record);

    insert(value_type(record->GetId(), record));

    itemCount++;
  };

  fclose(hFile);

  return itemCount;
}

static void
LoadString(FILE *file, int charCount, TCHAR *res);

/**
 * Reads next FLARMnet.org file entry and saves it
 * into the given record
 * @param file File handle
 * @param record Pointer to the FLARMNetRecord to be filled
 */
static void
LoadRecord(FILE *file, FLARMNetRecord *record)
{
  LoadString(file, 6, record->id);
  LoadString(file, 21, record->name);
  LoadString(file, 21, record->airfield);
  LoadString(file, 21, record->type);
  LoadString(file, 7, record->reg);
  LoadString(file, 3, record->cn);
  LoadString(file, 7, record->freq);

  int i = 0;
  int maxSize = sizeof(record->cn) / sizeof(TCHAR);
  while(record->cn[i] != 0 && i < maxSize) {
    if (record->cn[i] == 32)
      record->cn[i] = 0;

    i++;
  }

  fseek(file, 1, SEEK_CUR);
}

/**
 * Decodes the FLARMnet.org file and puts the wanted
 * characters into the res pointer
 * @param file File handle
 * @param charCount Number of character to decode
 * @param res Pointer to be written in
 */
static void
LoadString(FILE *file, int charCount, TCHAR *res)
{
  int bytesToRead = charCount * 2;
  char bytes[100];

  fread(bytes, 1, bytesToRead, file);

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
 * Finds a FLARMNetRecord object based on the given FLARM id
 * @param id FLARM id
 * @return FLARMNetRecord object
 */
const FLARMNetRecord *
FLARMNetDatabase::Find(long id) const
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

long
FLARMNetRecord::GetId() const
{
  return _tcstoul(id, NULL, 16);
};
