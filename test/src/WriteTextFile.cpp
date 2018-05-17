/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "IO/TextWriter.hpp"
#include "OS/Args.hpp"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
  Args args(argc, argv, "FILE");
  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  TextWriter writer(path);
  if (!writer.IsOpen()) {
    fprintf(stderr, "Failed to open file\n");
    return 1;
  }

  static char line[4096];
  while (fgets(line, sizeof(line), stdin)) {
    /* remove newline character */
    size_t length = strlen(line);
    while (length > 0 && (line[length - 1] == '\n' ||
                          line[length - 1] == '\r'))
      --length;
    line[length] = 0;

    writer.WriteLine(line);
  }

  return EXIT_SUCCESS;
}
