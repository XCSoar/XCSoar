// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Topography/TopographyGlue.hpp"
#include "Topography/TopographyStore.hpp"
#include "Language/Language.hpp"
#include "Profile/Profile.hpp"
#include "LogFile.hpp"
#include "io/MapFile.hpp"
#include "io/ZipArchive.hpp"
#include "io/ZipLineReader.hpp"
#include "system/Path.hpp"

/**
 * Load topography from the map file (ZIP), load the other files from
 * the same ZIP file.
 */
static bool
LoadConfiguredTopographyZip(TopographyStore &store)
try {
  auto archive = OpenMapFile();
  if (!archive)
    return false;

  ZipLineReaderA reader(archive->get(), "topology.tpl");
  store.Load(reader, nullptr, archive->get());
  return true;
} catch (...) {
  LogError(std::current_exception(), "No topography in map file");
  return false;
}

bool
LoadConfiguredTopography(TopographyStore &store)
{
  return LoadConfiguredTopographyZip(store);
}
