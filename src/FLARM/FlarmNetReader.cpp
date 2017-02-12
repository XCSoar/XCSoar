/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "FlarmNetReader.hpp"
#include "FlarmNetRecord.hpp"
#include "FlarmNetDatabase.hpp"
#include "Util/StringUtil.hpp"
#include "Util/CharUtil.hpp"
#include "IO/LineReader.hpp"
#include "IO/FileLineReader.hpp"

#ifndef _UNICODE
#include "Util/UTF8.hpp"
#endif

#include <stdio.h>
#include <stdlib.h>

/**
 * Decodes the FlarmNet.org file and puts the wanted
 * characters into the res pointer
 * @param file File handle
 * @param charCount Number of character to decode
 * @param res Pointer to be written in
 */
static void
LoadString(const char *bytes, size_t length, TCHAR *res, size_t res_size)
{
  const char *const end = bytes + length * 2;

#ifndef _UNICODE
  const char *const limit = res + res_size - 2;
#endif

  TCHAR *p = res;

  char tmp[3];
  tmp[2] = 0;

  while (bytes < end) {
    tmp[0] = *bytes++;
    tmp[1] = *bytes++;

    /* FLARMNet files are ISO-Latin-1, which is kind of short-sighted */

    const unsigned char ch = (unsigned char)strtoul(tmp, NULL, 16);
#ifdef _UNICODE
    /* Latin-1 can be converted to WIN32 wchar_t by casting */
    *p++ = ch;
#else
    /* convert to UTF-8 on all other platforms */

    if (p >= limit)
      break;

    p = Latin1ToUTF8(ch, p);
#endif
  }

  *p = 0;

#ifndef _UNICODE
  assert(ValidateUTF8(res));
#endif

  // Trim the string of any additional spaces
  StripRight(res);
}

template<size_t size>
static void
LoadString(const char *bytes, size_t length, StaticString<size> &dest)
{
  return LoadString(bytes, length, dest.buffer(), dest.capacity());
}

/**
 * Reads next FlarmNet.org file entry and returns the pointer to
 * a new FlarmNetRecord instance or NULL on error.
 *
 * The caller is responsible for deleting the object again!
 */
static bool
LoadRecord(FlarmNetRecord &record, const char *line)
{
  if (strlen(line) < 172)
    return false;

  LoadString(line, 6, record.id);
  LoadString(line + 12, 21, record.pilot);
  LoadString(line + 54, 21, record.airfield);
  LoadString(line + 96, 21, record.plane_type);
  LoadString(line + 138, 7, record.registration);
  LoadString(line + 152, 3, record.callsign);
  LoadString(line + 158, 7, record.frequency);

  // Terminate callsign string on first whitespace
  for (TCHAR *i = record.callsign.buffer(); *i != _T('\0'); ++i)
    if (IsWhitespaceFast(*i))
      *i = _T('\0');

  return true;
}

unsigned
FlarmNetReader::LoadFile(NLineReader &reader, FlarmNetDatabase &database)
{
  /* skip first line */
  const char *line = reader.ReadLine();
  if (line == NULL)
    return 0;

  int itemCount = 0;
  while ((line = reader.ReadLine()) != NULL) {
    FlarmNetRecord record;
    if (LoadRecord(record, line)) {
      database.Insert(record);
      itemCount++;
    }
  }

  return itemCount;
}

unsigned
FlarmNetReader::LoadFile(Path path, FlarmNetDatabase &database)
try {
  FileLineReaderA file(path);
  return LoadFile(file, database);
} catch (const std::runtime_error &e) {
  return 0;
}
