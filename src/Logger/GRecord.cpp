/* Copyright_License {

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

#include "Logger/GRecord.hpp"
#include "Logger/MD5.hpp"
#include "IGC/IGCString.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/FileOutputStream.hxx"
#include "IO/BufferedOutputStream.hxx"
#include "OS/Path.hpp"
#include "Util/Macros.hpp"

#include <stdexcept>

#include <string.h>

/**
 * Security theater.
 */
static constexpr MD5::State g_key[GRecord::N_MD5] = {
  { 0x1C80A301,0x9EB30b89,0x39CB2Afe,0x0D0FEA76 },
  { 0x48327203,0x3948ebea,0x9a9b9c9e,0xb3bed89a },
  { 0x67452301,0xefcdab89,0x98badcfe,0x10325476 },
  { 0xc8e899e8,0x9321c28a,0x438eba12,0x8cbe0aee },
};

void
GRecord::Initialize()
{
  ignore_comma = true;

  for (unsigned i = 0; i < N_MD5; ++i)
    md5[i].Initialise(g_key[i]);
}

bool
GRecord::AppendRecordToBuffer(const char *in)
{
  if (!IncludeRecordInGCalc(in))
    return false;

  if (memcmp(in, "HFFTYFRTYPE:XCSOAR,XCSOAR ", 26) == 0 &&
      strstr(in + 25, " 6.5 ") != nullptr)
    /* this is XCSoar 6.5: enable the G record workaround */
    ignore_comma = false;

  AppendStringToBuffer(in);
  return true;
}

/**
 * @param ignore_comma if true, then the comma is ignored, even though
 * it's a valid IGC character
 */
static void
AppendIGCString(MD5 &md5, const char *s, bool ignore_comma)
{
  while (*s != '\0') {
    const char ch = *s++;
    if (ignore_comma && ch == ',')
      continue;

    if (IsValidIGCChar(ch))
      md5.Append(ch);
  }
}

void
GRecord::AppendStringToBuffer(const char *in)
{
  for (auto &i : md5)
    AppendIGCString(i, in, ignore_comma);
}

void
GRecord::FinalizeBuffer()
{
  for (auto &i : md5)
    i.Finalize();
}

void
GRecord::GetDigest(char *output) const
{
  for (auto &i : md5)
    output = i.GetDigest(output);
}

bool
GRecord::IncludeRecordInGCalc(const char *in)
{
  bool valid = false;

  switch (in[0]) {
  case 'L':
    if (memcmp(in + 1, XCSOAR_IGC_CODE, 3) == 0)
      // only include L records made by XCS
      valid = true;
    break;

  case 'G':
    break;

  case 'H':
    if ((in[1] != 'O') && (in[1] != 'P'))
      valid = true;
    break;

  default:
    valid = true;
  }

  return valid;
}

void
GRecord::LoadFileToBuffer(Path path)
{
  FileLineReaderA reader(path);

  char *line;
  while ((line = reader.ReadLine()) != nullptr)
    AppendRecordToBuffer(line);
}

void
GRecord::WriteTo(BufferedOutputStream &writer) const
{
  char digest[DIGEST_LENGTH + 1];
  GetDigest(digest);

  static constexpr size_t chars_per_line = 16;
  static_assert(DIGEST_LENGTH % chars_per_line == 0, "wrong digest length");

  for (const char *i = digest, *end = digest + DIGEST_LENGTH;
       i != end; i += chars_per_line) {
    writer.Write('G');
    writer.Write(i, chars_per_line);
    writer.Write('\n');
  }
}

void
GRecord::AppendGRecordToFile(Path path)
{
  FileOutputStream file(path, FileOutputStream::Mode::APPEND_EXISTING);
  BufferedOutputStream writer(file);
  WriteTo(writer);
  writer.Flush();
  file.Commit();
}

void
GRecord::ReadGRecordFromFile(Path path,
                             char *output, size_t max_length)
{
  FileLineReaderA reader(path);

  unsigned int digest_length = 0;
  char *data;
  while ((data = reader.ReadLine()) != nullptr) {
    if (data[0] != 'G')
      continue;

    for (const char *p = data + 1; *p != '\0'; ++p) {
      output[digest_length++] = *p;
      if (digest_length >= max_length)
        throw std::runtime_error("G record too large");
    }
  }

  output[digest_length] = '\0';
}

void
GRecord::VerifyGRecordInFile(Path path)
{
  // assumes FileName member is set
  // Load File into Buffer (assume name is already set)
  LoadFileToBuffer(path);

  // load Existing Digest "old"
  char old_g_record[DIGEST_LENGTH + 1];
  ReadGRecordFromFile(path, old_g_record, ARRAY_SIZE(old_g_record));

  // recalculate digest from buffer
  FinalizeBuffer();

  char new_g_record[DIGEST_LENGTH + 1];
  GetDigest(new_g_record);

  if (strcmp(old_g_record, new_g_record) != 0)
    throw std::runtime_error("Invalid G record");
}
