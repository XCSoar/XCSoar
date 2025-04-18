// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * Reads input from stdin, calculates the NMEA checksum, and prints
 * the lines with a correct checksum to stdout.
 *
 */

#include "NMEA/Checksum.hpp"

#include <stdio.h>
#include <string.h>

int main()
{
  char buffer[1024];

  const char *start = buffer;
  while ((start = fgets(buffer, sizeof(buffer) - 3, stdin)) != NULL) {
    const char *end = strchr(start, '*');
    if (end == NULL) {
      end = start + strlen(start);
      while (end > start && (end[-1] == '\n' || end[-1] == '\r'))
        --end;
    }

    printf("%.*s*%02x\n", (int)(end - buffer), buffer,
#if defined(__APPLE__) && (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE)
           // MacOS workaround
           NMEAChecksum(start));
#else  // MacOS
           NMEAChecksum({start, end}));
#endif // MacOS
  }

  return 0;
}
