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
 * This program loads the topography from a map file and exits.  Useful
 * for valgrind and profiling.
 */

#include "Topography/TopographyStore.hpp"
#include "OS/PathName.hpp"
#include "IO/ZipLineReader.hpp"
#include "Projection/WindowProjection.hpp"
#include "Operation.hpp"

#include <zzip/zzip.h>

#include <stdio.h>
#include <tchar.h>

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Triangulate.hpp"

unsigned
PolygonToTriangles(const RasterPoint *points, unsigned num_points,
                   GLushort *triangles, unsigned min_distance)
{
  return 0;
}

#if RASTER_POINT_SIZE != SHAPE_POINT_SIZE
unsigned
PolygonToTriangles(const ShapePoint *points, unsigned num_points,
                   GLushort *triangles, unsigned min_distance)
{
  return 0;
}
#endif

unsigned
TriangleToStrip(GLushort *triangles, unsigned index_count,
                unsigned vertex_count, unsigned polygon_count)
{
  return 0;
}

#endif /* OpenGL */

class TestProjection : public WindowProjection {
public:
  TestProjection() {
    SetScreenOrigin(0, 0);
    SetScale(fixed(640) / (fixed(100) * 2));
    SetGeoLocation(GeoPoint(Angle::Degrees(fixed(7.7061111111111114)),
                            Angle::Degrees(fixed(51.051944444444445))));
    SetScreenSize(640, 480);
    UpdateScreenBounds();
  }
};

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

  TestProjection projection;

  topography.ScanVisibility(projection);

  return EXIT_SUCCESS;
}
