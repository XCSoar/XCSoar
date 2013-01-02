/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

/*
 * This program loads the topography from a map file and exits.  Useful
 * for valgrind and profiling.
 */

#include "Topography/TopographyStore.hpp"
#include "Topography/TopographyFile.hpp"
#include "Topography/XShape.hpp"
#include "OS/PathName.hpp"
#include "IO/ZipLineReader.hpp"
#include "Operation/Operation.hpp"

#include <zzip/zzip.h>

#include <stdio.h>
#include <tchar.h>

#ifdef ENABLE_OPENGL

static void
TriangulateAll(const TopographyFile &file)
{
  const unsigned short *count;
  for (const XShape &shape : file)
    if (shape.get_type() == MS_SHAPE_POLYGON)
      for (unsigned i = 0; i < 4; ++i)
        shape.get_indices(i, 1, count);
}

static void
TriangulateAll(const TopographyStore &store)
{
  for (unsigned i = 0; i < store.size(); ++i)
    TriangulateAll(store[i]);
}

#endif

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s PATH\n", argv[0]);
    return 1;
  }

  const char *path = argv[1];

  ZZIP_DIR *dir = zzip_dir_open(path, NULL);
  if (dir == NULL) {
    fprintf(stderr, "Failed to open %s\n", (const char *)path);
    return EXIT_FAILURE;
  }

  ZipLineReaderA reader(dir, "topology.tpl");
  if (reader.error()) {
    fprintf(stderr, "Failed to open %s\n", (const char *)path);
    return EXIT_FAILURE;
  }

  TopographyStore topography;
  NullOperationEnvironment operation;
  topography.Load(operation, reader, NULL, dir);
  zzip_dir_close(dir);

  topography.LoadAll();

#ifdef ENABLE_OPENGL
  TriangulateAll(topography);
#endif

  return EXIT_SUCCESS;
}
