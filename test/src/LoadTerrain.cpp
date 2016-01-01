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

/*
 * This program loads the terrain from a map file and exits.  Useful
 * for valgrind and profiling.
 */

#include "Terrain/RasterTileCache.hpp"
#include "Terrain/Loader.hpp"
#include "OS/Args.hpp"
#include "OS/ConvertPathName.hpp"
#include "Compatibility/path.h"
#include "Operation/Operation.hpp"

#include <zzip/zzip.h>

#include <stdio.h>
#include <string.h>
#include <tchar.h>

int main(int argc, char **argv)
{
  Args args(argc, argv, "PATH");
  const auto map_path = args.ExpectNext();
  args.ExpectEnd();

  ZZIP_DIR *dir = zzip_dir_open(map_path, nullptr);
  if (dir == nullptr) {
    fprintf(stderr, "Failed to open %s\n", map_path);
    return EXIT_FAILURE;
  }

  NullOperationEnvironment operation;
  RasterTileCache rtc;
  if (!LoadTerrainOverview(dir, rtc, operation)) {
    fprintf(stderr, "LoadOverview failed\n");
    zzip_dir_close(dir);
    return EXIT_FAILURE;
  }

  GeoBounds bounds = rtc.GetBounds();
  printf("bounds = %f|%f - %f|%f\n",
         (double)bounds.GetWest().Degrees(),
         (double)bounds.GetNorth().Degrees(),
         (double)bounds.GetEast().Degrees(),
         (double)bounds.GetSouth().Degrees());

  SharedMutex mutex;
  do {
    UpdateTerrainTiles(dir, rtc, mutex,
                       rtc.GetWidth() / 2, rtc.GetHeight() / 2, 1000);
  } while (rtc.IsDirty());
  zzip_dir_close(dir);

  return EXIT_SUCCESS;
}
