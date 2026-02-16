// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlarmNetReader.hpp"
#include "FlarmNetRecord.hpp"
#include "FlarmNetDatabase.hpp"
#include "util/CharUtil.hxx"
#include "util/StringStrip.hxx"
#include "io/LineReader.hpp"
#include "io/FileLineReader.hpp"
#include "util/UTF8.hpp"

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
LoadString(const char *bytes, size_t length, char *res, [[maybe_unused]] size_t res_size)
{
  const char *const end = bytes + length * 2;
  const char *const limit = res + res_size - 2;

  char *p = res;

  char tmp[3];
  tmp[2] = 0;

  while (bytes < end) {
    tmp[0] = *bytes++;
    tmp[1] = *bytes++;

    /* FLARMNet files are ISO-Latin-1, which is kind of short-sighted */

    const unsigned char ch = (unsigned char)strtoul(tmp, NULL, 16);

    /* convert to UTF-8 */

    if (p >= limit)
      break;

    p = Latin1ToUTF8(ch, p);
  }

  *p = 0;

  assert(ValidateUTF8(res));

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

  char id_buf[16];
  LoadString(line, 6, id_buf, sizeof(id_buf));
  record.id = FlarmId::Parse(id_buf, nullptr);

  LoadString(line + 12, 21, record.pilot);
  LoadString(line + 54, 21, record.airfield);
  LoadString(line + 96, 21, record.plane_type);
  LoadString(line + 138, 7, record.registration);
  LoadString(line + 152, 3, record.callsign);

  StaticString<LatinBufferSize(8)> freq_text;
  LoadString(line + 158, 7, freq_text);
  char freq_ascii[16];
  char *freq_end = CopyASCII(freq_ascii, sizeof(freq_ascii) - 1, freq_text);
  *freq_end = '\0';
  record.frequency = RadioFrequency::Parse(std::string_view(freq_ascii));

  // Terminate callsign string on first whitespace
  for (char *i = record.callsign.buffer(); *i != _T('\0'); ++i)
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
} catch (...) {
  return 0;
}
