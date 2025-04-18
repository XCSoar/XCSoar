// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "io/FileReader.hxx"
#include "io/BufferedReader.hxx"
#include "lib/zlib/GunzipReader.hxx"
#include "system/Args.hpp"
#include "util/PrintException.hxx"

#include <stdio.h>

int main(int argc, char **argv)
try {
  Args args(argc, argv, "FILE");
  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  FileReader file(path);
  GunzipReader gunzip(file);
  BufferedReader reader(gunzip);

  char *line;
  while ((line = reader.ReadLine()) != nullptr)
    puts(line);

  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
