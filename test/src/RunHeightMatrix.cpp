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

#include "Terrain/RasterMap.hpp"
#include "Terrain/HeightMatrix.hpp"
#include "WindowProjection.hpp"
#include "Screen/Layout.hpp"
#include "OS/PathName.hpp"
#include "Compatibility/path.h"
#include "Operation.hpp"

#include <stdio.h>
#include <tchar.h>

unsigned Layout::scale_1024 = 1024;

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s PATH\n", argv[0]);
    return 1;
  }

  const char *map_path = argv[1];

  TCHAR jp2_path[4096];
  _tcscpy(jp2_path, PathName(map_path));
  _tcscat(jp2_path, _T(DIR_SEPARATOR_S) _T("terrain.jp2"));

  TCHAR j2w_path[4096];
  _tcscpy(j2w_path, PathName(map_path));
  _tcscat(j2w_path, _T(DIR_SEPARATOR_S) _T("terrain.j2w"));

  NullOperationEnvironment operation;
  RasterMap map(jp2_path, j2w_path, NULL, operation);
  if (!map.isMapLoaded()) {
    fprintf(stderr, "failed to load map\n");
    return EXIT_FAILURE;
  }

  do {
    map.SetViewCenter(map.GetMapCenter(), fixed(50000));
  } while (map.IsDirty());

  fixed radius = fixed(50000);
  WindowProjection projection;
  projection.SetScreenSize(640, 480);
  projection.SetScaleFromRadius(radius);
  projection.SetGeoLocation(map.GetMapCenter());
  projection.SetScreenOrigin(320, 240);
  projection.UpdateScreenBounds();

  HeightMatrix matrix;
  matrix.Fill(map, projection, 1, false);

  return EXIT_SUCCESS;
}
