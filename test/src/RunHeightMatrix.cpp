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

#include "Terrain/RasterMap.hpp"
#include "Terrain/HeightMatrix.hpp"
#include "Terrain/Loader.hpp"
#include "Projection/WindowProjection.hpp"
#include "Screen/Layout.hpp"
#include "OS/Args.hpp"
#include "Operation/Operation.hpp"

#include <zzip/zzip.h>

#include <stdio.h>
#include <string.h>
#include <tchar.h>

unsigned Layout::scale_1024 = 1024;

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

  RasterMap map;

  NullOperationEnvironment operation;
  if (!LoadTerrainOverview(dir, map.GetTileCache(),
                           operation)) {
    fprintf(stderr, "failed to load map\n");
    zzip_dir_close(dir);
    return EXIT_FAILURE;
  }

  SharedMutex mutex;
  do {
    UpdateTerrainTiles(dir, map.GetTileCache(), mutex,
                       map.GetProjection(),
                       map.GetMapCenter(), fixed(50000));
  } while (map.IsDirty());
  zzip_dir_close(dir);

  fixed radius = fixed(50000);
  WindowProjection projection;
  projection.SetScreenSize({640, 480});
  projection.SetScaleFromRadius(radius);
  projection.SetGeoLocation(map.GetMapCenter());
  projection.SetScreenOrigin(320, 240);
  projection.UpdateScreenBounds();

  HeightMatrix matrix;
#ifdef ENABLE_OPENGL
  matrix.Fill(map, projection.GetScreenBounds(),
              projection.GetScreenWidth(), projection.GetScreenHeight(),
              false);
#else
  matrix.Fill(map, projection, 1, false);
#endif

  return EXIT_SUCCESS;
}
