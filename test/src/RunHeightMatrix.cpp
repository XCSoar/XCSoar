// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Terrain/RasterMap.hpp"
#include "Terrain/HeightMatrix.hpp"
#include "Terrain/Loader.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "Projection/WindowProjection.hpp"
#include "Screen/Layout.hpp"
#include "system/Args.hpp"
#include "io/ZipArchive.hpp"
#include "util/PrintException.hxx"

#include <stdio.h>
#include <string.h>
#include <tchar.h>

unsigned Layout::scale_1024 = 1024;

int main(int argc, char **argv)
try {
  Args args(argc, argv, "PATH");
  const auto map_path = args.ExpectNextPath();
  args.ExpectEnd();

  ZipArchive archive(map_path);

  RasterMap map;

  {
    ConsoleOperationEnvironment operation;
    LoadTerrainOverview(archive.get(), map.GetTileCache(), operation);
  }

  map.UpdateProjection();

  SharedMutex mutex;
  do {
    UpdateTerrainTiles(archive.get(), map.GetTileCache(), mutex,
                       map.GetProjection(),
                       map.GetMapCenter(), 50000);
  } while (map.IsDirty());

  double radius = 50000;
  WindowProjection projection;
  projection.SetScreenSize({640, 480});
  projection.SetScaleFromRadius(radius);
  projection.SetGeoLocation(map.GetMapCenter());
  projection.SetScreenOrigin(320, 240);
  projection.UpdateScreenBounds();

  HeightMatrix matrix;
#ifdef ENABLE_OPENGL
  matrix.Fill(map, projection.GetScreenBounds(),
              (UnsignedPoint2D)projection.GetScreenSize(),
              false);
#else
  matrix.Fill(map, projection, 1, false);
#endif

  return EXIT_SUCCESS;
} catch (const std::runtime_error &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
