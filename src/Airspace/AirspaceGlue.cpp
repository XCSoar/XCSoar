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

#include "Airspace/AirspaceGlue.hpp"
#include "Airspace/AirspaceParser.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Operation/Operation.hpp"
#include "Language/Language.hpp"
#include "LogFile.hpp"
#include "OS/Path.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/ZipLineReader.hpp"
#include "IO/MapFile.hpp"
#include "Profile/Profile.hpp"
#include "Util/Error.hxx"

#include <zzip/zzip.h>

#include <string.h>

static bool
ParseAirspaceFile(AirspaceParser &parser, Path path,
                  OperationEnvironment &operation)
{
  Error error;
  FileLineReader reader(path, error, Charset::AUTO);
  if (reader.error()) {
    LogError("Failed to parse airspace file", error);
    return false;
  }

  if (!parser.Parse(reader, operation)) {
    LogFormat(_T("Failed to parse airspace file: %s"), path.c_str());
    return false;
  }

  return true;
}

static bool
ParseAirspaceFile(AirspaceParser &parser,
                  struct zzip_dir *dir, const char *path,
                  OperationEnvironment &operation)
{
  Error error;
  ZipLineReader reader(dir, path, error, Charset::AUTO);
  if (reader.error()) {
    LogError("Failed to parse airspace file", error);
    return false;
  }

  if (!parser.Parse(reader, operation)) {
    LogFormat("Failed to parse airspace file: %s", path);
    return false;
  }

  return true;
}

void
ReadAirspace(Airspaces &airspaces,
             RasterTerrain *terrain,
             const AtmosphericPressure &press,
             OperationEnvironment &operation)
{
  LogFormat("ReadAirspace");
  operation.SetText(_("Loading Airspace File..."));

  bool airspace_ok = false;

  AirspaceParser parser(airspaces);

  // Read the airspace filenames from the registry
  auto path = Profile::GetPath(ProfileKeys::AirspaceFile);
  if (!path.IsNull())
    airspace_ok |= ParseAirspaceFile(parser, path, operation);

  path = Profile::GetPath(ProfileKeys::AdditionalAirspaceFile);
  if (!path.IsNull())
    airspace_ok |= ParseAirspaceFile(parser, path, operation);

  auto dir = OpenMapFile();
  if (dir != nullptr) {
    airspace_ok |= ParseAirspaceFile(parser, dir, "airspace.txt", operation);
    zzip_dir_close(dir);
  }

  if (airspace_ok) {
    airspaces.Optimise();
    airspaces.SetFlightLevels(press);

    if (terrain != NULL)
      airspaces.SetGroundLevels(*terrain);
  } else
    // there was a problem
    airspaces.Clear();
}
