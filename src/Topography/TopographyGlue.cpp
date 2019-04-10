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

#include "Topography/TopographyGlue.hpp"
#include "Topography/TopographyStore.hpp"
#include "Language/Language.hpp"
#include "Profile/Profile.hpp"
#include "LogFile.hpp"
#include "Operation/Operation.hpp"
#include "IO/MapFile.hpp"
#include "IO/ZipArchive.hpp"
#include "IO/ZipLineReader.hpp"

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
  LogFormat("Loading Topography File...");
  operation.SetText(_("Loading Topography File..."));

  return LoadConfiguredTopographyZip(store, operation);
}
