/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "IO/FileSource.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/TextWriter.hpp"
#include "Util/Macros.hpp"

#include <tchar.h>
#include <string.h>

void
GRecord::Initialize()
{
  // key #1 used w/ Vali 1.0.0
  // key #2 used w/ Vali 1.0.2
  // OLC uses key #2 since 9/1/2008
  return Initialize(2);
}

bool
GRecord::AppendRecordToBuffer(const char *in)
{
  if (!IncludeRecordInGCalc(in))
    return false;

  AppendStringToBuffer(in);
  return true;
}

static void
AppendIGCString(MD5 &md5, const char *s)
{
  while (*s != '\0') {
    const char ch = *s++;
    if (IsValidIGCChar(ch))
      md5.Append(ch);
  }
}

void
GRecord::AppendStringToBuffer(const char *in)
{
  for (int i = 0; i < 4; i++)
    AppendIGCString(md5[i], in);
}

void
GRecord::FinalizeBuffer()
{
  for (int i = 0; i < 4; i++)
    md5[i].Finalize();
}

void
GRecord::GetDigest(char *output) const
{
  for (int i = 0; i <= 3; i++, output += MD5::DIGEST_LENGTH)
    md5[i].GetDigest(output);
}

void
GRecord::Initialize(int key_id)
{
  // 4 different 512 bit keys
  switch (key_id)
  {
  case 2:
    // key 2
    md5[0].InitKey(0x1C80A301,0x9EB30b89,0x39CB2Afe,0x0D0FEA76);
    md5[1].InitKey(0x48327203,0x3948ebea,0x9a9b9c9e,0xb3bed89a);
    md5[2].InitKey(0x67452301,0xefcdab89,0x98badcfe,0x10325476);
    md5[3].InitKey(0xc8e899e8,0x9321c28a,0x438eba12,0x8cbe0aee);
    break;

  case 3:
    // key 3
    md5[0].InitKey(0x7894abde,0x9cb4e90a,0x0bc8f0ea,0x03a9e01a);
    md5[1].InitKey(0x3c4a4c93,0x9cbf7ae3,0xa9bcd0ea,0x9a8c2aaa);
    md5[2].InitKey(0x3c9ae1f1,0x9fe02a1f,0x3fc9a497,0x93cad3ef);
    md5[3].InitKey(0x41a0c8e8,0xf0e37acf,0xd8bcabe2,0x9bed015a);
    break;

  case 1:
  default:
    // key 1
    md5[0].InitKey(0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476);
    md5[1].InitKey(0x48327203, 0x3948ebea, 0x9a9b9c9e, 0xb3bed89a);
    md5[2].InitKey(0x67452301, 0xefcdab89,  0x98badcfe, 0x10325476);
    md5[3].InitKey( 0xc8e899e8, 0x9321c28a, 0x438eba12, 0x8cbe0aee);
    break;
  }
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

bool
GRecord::LoadFileToBuffer(const TCHAR *filename)
{
  FileLineReaderA reader(filename);
  if (reader.error())
    return false;

  char *line;

  while ((line = reader.ReadLine()) != NULL)
    AppendRecordToBuffer(line);

  return true;
}

void
GRecord::WriteTo(TextWriter &writer) const
{
  char digest[DIGEST_LENGTH + 1];
  GetDigest(digest);

  static constexpr size_t chars_per_line = 16;
  static_assert(DIGEST_LENGTH % chars_per_line == 0, "wrong digest length");

  for (const char *i = digest, *end = digest + DIGEST_LENGTH;
       i != end; i += chars_per_line) {
    writer.Write('G');
    writer.Write(i, chars_per_line);
    writer.NewLine();
  }
}

bool
GRecord::AppendGRecordToFile(const TCHAR *filename)
{
  TextWriter writer(filename, true);
  if (!writer.IsOpen())
    return false;

  WriteTo(writer);
  return true;
}

bool
GRecord::ReadGRecordFromFile(const TCHAR *filename,
                             char *output, size_t max_length)
{
  FileLineReaderA reader(filename);
  if (reader.error())
    return false;

  unsigned int digest_length = 0;
  char *data;
  while ((data = reader.ReadLine()) != NULL) {
    if (data[0] != 'G')
      continue;

    for (const char *p = data + 1; *p != '\0'; ++p) {
      output[digest_length++] = *p;
      if (digest_length >= max_length)
        /* G record too large */
        return false;
    }
  }

  output[digest_length] = '\0';
  return true;
}

bool
GRecord::VerifyGRecordInFile(const TCHAR *path)
{
  // assumes FileName member is set
  // Load File into Buffer (assume name is already set)
  LoadFileToBuffer(path);

  // load Existing Digest "old"
  char old_g_record[DIGEST_LENGTH + 1];
  if (!ReadGRecordFromFile(path, old_g_record, ARRAY_SIZE(old_g_record)))
    return false;

  // recalculate digest from buffer
  FinalizeBuffer();

  char new_g_record[DIGEST_LENGTH + 1];
  GetDigest(new_g_record);

  return strcmp(old_g_record, new_g_record) == 0;
}
