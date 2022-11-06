/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "Atmosphere/Pressure.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Operation/Operation.hpp"
#include "Language/Language.hpp"
#include "LogFile.hpp"
#include "system/Path.hpp"
#include "io/FileLineReader.hpp"
#include "io/ZipArchive.hpp"
#include "io/ZipLineReader.hpp"
#include "io/MapFile.hpp"
#include "util/RuntimeError.hxx"
#include "Profile/Profile.hpp"

#include <string.h>

static bool
ParseAirspaceFile(Airspaces &airspaces, Path path,
                  OperationEnvironment &operation)
try {
  FileLineReader reader(path, Charset::AUTO);

  try {
    ParseAirspaceFile(airspaces, reader, operation);
  } catch (...) {
    // TODO translate this?
    std::throw_with_nested(FormatRuntimeError("Error in file %s",
                                              path.ToUTF8().c_str()));
  }

  return true;
} catch (...) {
  LogError(std::current_exception());
  operation.SetError(std::current_exception());
  return false;
}

static bool
ParseAirspaceFile(Airspaces &airspaces,
                  struct zzip_dir *dir, const char *path,
                  OperationEnvironment &operation)
try {
  ZipLineReader reader(dir, path, Charset::AUTO);

  try {
    ParseAirspaceFile(airspaces, reader, operation);
  } catch (...) {
    // TODO translate this?
    std::throw_with_nested(FormatRuntimeError("Error in file %s",
                                              path));
  }

  return true;
} catch (...) {
  LogError(std::current_exception());
  operation.SetError(std::current_exception());
  return false;
}

void
ReadAirspace(Airspaces &airspaces,
             AtmosphericPressure press,
             OperationEnvironment &operation)
{
  LogFormat("ReadAirspace");
  operation.SetText(_("Loading Airspace File..."));

  bool airspace_ok = false;

  // Read the airspace filenames from the registry
  if (const auto path = Profile::GetPath(ProfileKeys::AirspaceFile);
      path != nullptr)
    airspace_ok |= ParseAirspaceFile(airspaces, path, operation);

  if (const auto path = Profile::GetPath(ProfileKeys::AdditionalAirspaceFile);
      path != nullptr)
    airspace_ok |= ParseAirspaceFile(airspaces, path, operation);

  try {
    if (auto archive = OpenMapFile();
        archive && archive->Exists("airspace.txt"))
      airspace_ok |= ParseAirspaceFile(airspaces, archive->get(),
                                       "airspace.txt", operation);
  } catch (...) {
    LogError(std::current_exception(),
             "Failed to load airspaces from map file");
  }

  if (airspace_ok) {
    airspaces.Optimise();
    airspaces.SetFlightLevels(press);
  } else
    // there was a problem
    airspaces.Clear();
}

void
SetAirspaceGroundLevels(Airspaces &airspaces,
                        const RasterTerrain &terrain) noexcept
{
  airspaces.SetGroundLevels(terrain);
}
