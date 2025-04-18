// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * This program loads the terrain from a map file and exits.  Useful
 * for valgrind and profiling.
 */

#include "Terrain/RasterTileCache.hpp"
#include "Terrain/Loader.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "system/Args.hpp"
#include "system/ConvertPathName.hpp"
#include "io/ZipArchive.hpp"
#include "util/PrintException.hxx"

#include <stdio.h>
#include <string.h>
#include <tchar.h>

int main(int argc, char **argv)
try {
  Args args(argc, argv, "PATH");
  const auto map_path = args.ExpectNextPath();
  args.ExpectEnd();

  ZipArchive archive(map_path);

  RasterTileCache rtc;

  {
    ConsoleOperationEnvironment operation;
    LoadTerrainOverview(archive.get(), rtc, operation);
  }

  GeoBounds bounds = rtc.GetBounds();
  printf("bounds = %f|%f - %f|%f\n",
         (double)bounds.GetWest().Degrees(),
         (double)bounds.GetNorth().Degrees(),
         (double)bounds.GetEast().Degrees(),
         (double)bounds.GetSouth().Degrees());

  SharedMutex mutex;
  do {
    UpdateTerrainTiles(archive.get(), rtc, mutex,
                       SignedRasterLocation(rtc.GetSize().x / 2,
                                            rtc.GetSize().y / 2),
                       1000);
  } while (rtc.IsDirty());

  return EXIT_SUCCESS;
} catch (const std::runtime_error &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
