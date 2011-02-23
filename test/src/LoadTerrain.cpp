/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Terrain/RasterTile.hpp"
#include "OS/PathName.hpp"
#include "Compatibility/path.h"
#include "Operation.hpp"

#include <stdio.h>
#include <tchar.h>

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s PATH\n", argv[0]);
    return 1;
  }

  const char *map_path = argv[1];

  char jp2_path[4096];
  strcpy(jp2_path, map_path);
  strcat(jp2_path, DIR_SEPARATOR_S "terrain.jp2");

  TCHAR j2w_path[4096];
  _tcscpy(j2w_path, PathName(map_path));
  _tcscat(j2w_path, _T(DIR_SEPARATOR_S) _T("terrain.j2w"));

  OperationEnvironment operation;
  RasterTileCache rtc;
  if (!rtc.LoadOverview(jp2_path, j2w_path, operation)) {
    fprintf(stderr, "LoadOverview failed\n");
    return EXIT_FAILURE;
  }

  GeoBounds bounds = rtc.GetBounds();
  printf("bounds = %f|%f - %f|%f\n",
         (double)bounds.west.value_degrees(),
         (double)bounds.north.value_degrees(),
         (double)bounds.east.value_degrees(),
         (double)bounds.south.value_degrees());

  do {
    rtc.UpdateTiles(jp2_path, rtc.GetWidth() / 2, rtc.GetHeight() / 2,
                    fixed(50000));
  } while (rtc.IsDirty());

  return EXIT_SUCCESS;
}
