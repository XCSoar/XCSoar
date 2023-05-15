// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Topography/TopographyGlue.hpp"
#include "Topography/TopographyStore.hpp"
#include "Language/Language.hpp"
#include "Profile/Profile.hpp"
#include "LogFile.hpp"
#include "Operation/Operation.hpp"
#include "io/MapFile.hpp"
#include "io/ZipArchive.hpp"
#include "io/ZipLineReader.hpp"
#include "system/Path.hpp"

/**
 * Load topography from the map file (ZIP), load the other files from
 * the same ZIP file.
 */
static bool
LoadConfiguredTopographyZip(TopographyStore &store,
                            OperationEnvironment &operation)
try {
  auto archive = OpenMapFile();
  if (!archive)
    return false;

  ZipLineReaderA reader(archive->get(), "topology.tpl");
  store.Load(operation, reader, nullptr, archive->get());
  return true;
} catch (...) {
  LogError(std::current_exception(), "No topography in map file");
  return false;
}

bool
LoadConfiguredTopography(TopographyStore &store,
                         OperationEnvironment &operation)
{
  LogString("Loading Topography File...");
  operation.SetText(_("Loading Topography File..."));

  return LoadConfiguredTopographyZip(store, operation);
}
