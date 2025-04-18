// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Logger/GRecord.hpp"
#include "system/Args.hpp"
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
  g.LoadFileToBuffer(path);
  g.FinalizeBuffer();

  g.AppendGRecordToFile(path);

  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
