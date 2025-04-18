// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Language/MOLoader.hpp"
#include "system/Args.hpp"

#include <stdio.h>

int main(int argc, char **argv) {
  Args args(argc, argv, "FILE.igc STRING");
  const char *narrow_path = args.PeekNext();
  const auto path = args.ExpectNextPath();
  const char *original = args.ExpectNext();
  args.ExpectEnd();

  const MOLoader mo(path);
  if (mo.error()) {
    fprintf(stderr, "Failed to load %s\n", narrow_path);
    return 2;
  }

  const char *translated = mo.get().lookup(original);
  if (translated == NULL) {
    fprintf(stderr, "No such string\n");
    return 3;
  }

  puts(translated);
  return 0;
}
