// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Profile/Profile.hpp"
#include "LocalPath.hpp"
#include "system/Args.hpp"

#include <stdio.h>

int main(int argc, char **argv) {
  Args args(argc, argv, "NAME");
  const char *name = args.ExpectNext();
  args.ExpectEnd();

  InitialiseDataPath();
  Profile::SetFiles(nullptr);
  Profile::Load();
  DeinitialiseDataPath();

  const char *value = Profile::Get(name);
  if (value != NULL) {
    puts(value);
    return 0;
  } else {
    fputs("No such value\n", stderr);
    return 2;
  }
}
