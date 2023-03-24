// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Logger/GRecord.hpp"
#include "system/Args.hpp"
#include "io/FileTransaction.hpp"
#include "io/FileLineReader.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"
#include "util/PrintException.hxx"

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

  transaction.Commit();

  printf("New G record written\n");
  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
