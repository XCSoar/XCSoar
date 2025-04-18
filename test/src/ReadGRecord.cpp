// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Logger/GRecord.hpp"
#include "system/Args.hpp"
#include "util/Macros.hpp"
#include "util/PrintException.hxx"

#include <stdio.h>

int
main(int argc, char **argv)
try {
  Args args(argc, argv, "FILE.igc");
  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  GRecord g;
  g.Initialize();

  char data[1024];
  g.ReadGRecordFromFile(path, data, ARRAY_SIZE(data));
  puts(data);
  return 0;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
