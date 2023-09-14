// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "io/FileLineReader.hpp"
#include "system/Args.hpp"
#include "util/PrintException.hxx"

#include <stdio.h>

int main(int argc, char **argv)
try {
  Args args(argc, argv, "FILE");
  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  FileLineReader reader(path);

  TCHAR *line;
  while ((line = reader.ReadLine()) != NULL)
    _putts(line);

  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
