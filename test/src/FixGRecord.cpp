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
#include "OS/Args.hpp"
#include "IO/FileTransaction.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/FileOutputStream.hxx"
#include "IO/BufferedOutputStream.hxx"
#include "Util/PrintException.hxx"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static void
FixGRecord(NLineReader &reader, BufferedOutputStream &writer)
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

    writer.Write(line);
    writer.Write('\n');
  }

  grecord.FinalizeBuffer();
  grecord.WriteTo(writer);
}

static void
FixGRecord(NLineReader &reader, Path dest_path)
{
  FileOutputStream file(dest_path);
  BufferedOutputStream writer(file);
  FixGRecord(reader, writer);
  writer.Flush();
  file.Commit();
}

int
main(int argc, char **argv)
try {
  Args args(argc, argv, "FILE.igc");
  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  {
    GRecord grecord;
    grecord.Initialize();
    grecord.VerifyGRecordInFile(path);
  }

  printf("Valid G record found\n");

  FileTransaction transaction(path);

  {
    FileLineReaderA reader(path);

    FixGRecord(reader, transaction.GetTemporaryPath());
  }

  if (!transaction.Commit()) {
    fprintf(stderr, "Failed to commit output file\n");
    return EXIT_FAILURE;
  }

  printf("New G record written\n");
  return EXIT_SUCCESS;
} catch (const std::runtime_error &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
