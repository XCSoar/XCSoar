// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifdef HAVE_POSIX
#include "Device/Port/TTYEnumerator.hpp"
#endif

#include <stdio.h>
#include <stdlib.h>

int main()
{
  bool implemented = false, success = false;

#ifdef HAVE_POSIX
  implemented = true;

  TTYEnumerator te;
  if (!te.HasFailed()) {
    success = true;

    const char *path;
    while ((path = te.Next()) != nullptr)
      printf("%s\n", path);
  } else
    fprintf(stderr, "Failed to enumerate TTY ports\n");
#endif

  if (!implemented)
    fprintf(stderr, "Port enumeration not implemented on this target\n");

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
