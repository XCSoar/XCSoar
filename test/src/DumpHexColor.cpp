// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Formatter/HexColor.hpp"
#include "ui/canvas/PortableColor.hpp"
#include "system/Args.hpp"
#include "util/Macros.hpp"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
  Args args(argc, argv, "COLOR ...");
  const char *s = args.ExpectNext();

  while (true) {
    RGB8Color color;
    if (!ParseHexColor(s, color)) {
      fprintf(stderr, "Failed to parse '%s'\n", s);
      return EXIT_FAILURE;
    }

    char buffer[32];
    FormatHexColor(buffer, ARRAY_SIZE(buffer), color);

    printf("%s -> %s\n", s, buffer);

    if (args.IsEmpty())
      break;

    s = args.ExpectNext();
  }

  return EXIT_SUCCESS;
}
