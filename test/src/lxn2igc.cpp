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

/* Convert LXN files to IGC */

#include "Device/Driver/LX/Convert.hpp"
#include "OS/Args.hpp"

#include <stdio.h>
#include <stdlib.h>

static const long MAX_LXN_SIZE = 1024 * 1024;

int
main(int argc, char **argv)
{
  Args args(argc, argv, "FILE.lxn");
  const char *lxn_path = args.ExpectNext();
  args.ExpectEnd();

  FILE *file = fopen(lxn_path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Failed to open file %s\n", lxn_path);
    return EXIT_FAILURE;
  }

  long size;
  if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) <= 0 ||
      fseek(file, 0, SEEK_SET) != 0 || size > MAX_LXN_SIZE)  {
    fprintf(stderr, "Failed to seek file %s\n", lxn_path);
    fclose(file);
    return EXIT_FAILURE;
  }

  void *data = malloc(size);
  size_t n = fread(data, 1, size, file);
  fclose(file);
  if (n != (size_t)size) {
    free(data);
    fprintf(stderr, "Failed to read from file %s\n", lxn_path);
    return EXIT_FAILURE;
  }

  bool success = LX::ConvertLXNToIGC(data, n, stdout);
  free(data);

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
