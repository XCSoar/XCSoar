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
#include "OS/Args.hpp"
#include "IO/FileTransaction.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/TextWriter.hpp"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static bool
FixGRecord(NLineReader &reader, TextWriter &writer)
{
  GRecord grecord;
  grecord.Initialize();

  char digest[GRecord::DIGEST_LENGTH + 1];
  grecord.GetDigest(digest);

  char *line;
  while ((line = reader.ReadLine()) != nullptr) {
    if (line[0] == 'G')
      break;

    if (memcmp(line, "HFFTYFRTYPE:XCSOAR,XCSOAR ", 26) == 0) {
      char *v = strstr(line + 25, " 6.5 ");
      if (v != nullptr) {
        static char buffer[1024], *p = buffer;

        size_t n = v + 4 - line;
        memcpy(p, line, n);
        p += n;

        memcpy(p, "fix", 3);
        p += 3;

        strcpy(p, v + 4);

        line = buffer;
      }
    }

    grecord.AppendRecordToBuffer(line);

    if (!writer.WriteLine(line))
      return false;
  }

  grecord.FinalizeBuffer();
  grecord.WriteTo(writer);
  return true;
}

static bool
FixGRecord(NLineReader &reader, const TCHAR *dest_path)
{
  TextWriter writer(dest_path);
  return writer.IsOpen() && FixGRecord(reader, writer) && writer.Flush();
}

int
main(int argc, char **argv)
{
  Args args(argc, argv, "FILE.igc");
  tstring path = args.ExpectNextT();
  args.ExpectEnd();

  {
    GRecord grecord;
    grecord.Initialize();

    if (!grecord.VerifyGRecordInFile(path.c_str())) {
      fprintf(stderr, "Invalid G record\n");
      return EXIT_FAILURE;
    }
  }

  printf("Valid G record found\n");

  FileTransaction transaction(path.c_str());

  {
    FileLineReaderA reader(path.c_str());
    if (reader.error()) {
      fprintf(stderr, "Failed to open input file\n");
      return EXIT_FAILURE;
    }

    if (!FixGRecord(reader, transaction.GetTemporaryPath())) {
      fprintf(stderr, "Failed to write output file\n");
      return EXIT_FAILURE;
    }
  }

  if (!transaction.Commit()) {
    fprintf(stderr, "Failed to commit output file\n");
    return EXIT_FAILURE;
  }

  printf("New G record written\n");
  return EXIT_SUCCESS;
}
