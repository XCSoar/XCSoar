/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "OS/Args.hpp"
#include "OS/ConvertPathName.hpp"
#include "Operation/Operation.hpp"

#include <stdio.h>
#include <string.h>
#include <tchar.h>

int main(int argc, char **argv)
{
  Args args(argc, argv, "PATH");
  const auto map_path = args.ExpectNextT();
  args.ExpectEnd();

  TCHAR jp2_path[4096];
  _tcscpy(jp2_path, map_path.c_str());
  _tcscat(jp2_path, _T("/terrain.jp2"));

  TCHAR j2w_path[4096];
  _tcscpy(j2w_path, map_path.c_str());
  _tcscat(j2w_path, _T("/terrain.j2w"));

  NullOperationEnvironment operation;
  RasterTileCache rtc;
  if (!rtc.LoadOverview(jp2_path, j2w_path, operation)) {
    fprintf(stderr, "LoadOverview failed\n");
    return EXIT_FAILURE;
  }

  GeoBounds bounds = rtc.GetBounds();
  printf("bounds = %f|%f - %f|%f\n",
         (double)bounds.GetWest().Degrees(),
         (double)bounds.GetNorth().Degrees(),
         (double)bounds.GetEast().Degrees(),
         (double)bounds.GetSouth().Degrees());

  do {
    rtc.UpdateTiles(jp2_path, rtc.GetWidth() / 2, rtc.GetHeight() / 2,
                    1000);
  } while (rtc.IsDirty());

  return EXIT_SUCCESS;
}
