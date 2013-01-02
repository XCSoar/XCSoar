/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "IO/ZipLineReader.hpp"
#include "Util/ConvertString.hpp"

#include <zzip/zzip.h>

#include <windef.h> /* for MAX_PATH */

/**
 * Load topography from the map file (ZIP), load the other files from
 * the same ZIP file.
 */
static bool
LoadConfiguredTopographyZip(TopographyStore &store,
                            OperationEnvironment &operation)
{
  TCHAR path[MAX_PATH];
  if (!Profile::GetPath(ProfileKeys::MapFile, path))
    return false;

  const WideToACPConverter narrow_path(path);
  ZZIP_DIR *dir = zzip_dir_open(narrow_path, NULL);
  if (dir == NULL)
    return false;

  ZipLineReaderA reader(dir, "topology.tpl");
  if (reader.error()) {
    zzip_dir_close(dir);
    LogStartUp(_T("No topography in map file: %s"), path);
    return false;
  }

  store.Load(operation, reader, NULL, dir);
  zzip_dir_close(dir);
  return true;
}

bool
LoadConfiguredTopography(TopographyStore &store,
                         OperationEnvironment &operation)
{
  LogStartUp(_T("Loading Topography File..."));
  operation.SetText(_("Loading Topography File..."));

  return LoadConfiguredTopographyZip(store, operation);
}
