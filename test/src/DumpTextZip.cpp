// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "io/ZipArchive.hpp"
#include "io/ZipLineReader.hpp"
#include "system/Args.hpp"
#include "util/PrintException.hxx"

#include <zzip/zzip.h>

#include <stdio.h>

int main(int argc, char **argv)
try {
  Args args(argc, argv, "ZIPFILE FILENAME");
  const auto zip_path = args.ExpectNextPath();
  const char *filename = args.ExpectNext();
  args.ExpectEnd();

  ZipArchive archive(zip_path);

  ZipLineReader reader(archive.get(), filename);

  TCHAR *line;
  while ((line = reader.ReadLine()) != NULL)
    _putts(line);

  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
