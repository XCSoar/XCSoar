// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Airspace/AirspaceGlue.hpp"
#include "Airspace/AirspaceParser.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Language/Language.hpp"
#include "LogFile.hpp"
#include "Operation/Operation.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "io/BufferedReader.hxx"
#include "io/FileReader.hxx"
#include "io/MapFile.hpp"
#include "io/ProgressReader.hpp"
#include "io/ZipArchive.hpp"
#include "io/ZipLineReader.hpp"
#include "lib/fmt/PathFormatter.hpp"
#include "lib/fmt/RuntimeError.hxx"
#include "system/Path.hpp"

#include <string.h>

bool
ParseAirspaceFile(Airspaces &airspaces, Path path,
                  OperationEnvironment &operation) noexcept
try {
  FileReader file_reader{path};
  ProgressReader progress_reader{file_reader, file_reader.GetSize(), operation};
  BufferedReader buffered_reader{progress_reader};

  try {
    ParseAirspaceFile(airspaces, buffered_reader);
  } catch (...) {
    // TODO translate this?
    std::throw_with_nested(FmtRuntimeError("Error in file {}", path));
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
  ZipReader zip_reader{dir, path};
  ProgressReader progress_reader{zip_reader, zip_reader.GetSize(), operation};
  BufferedReader buffered_reader{progress_reader};

  try {
    ParseAirspaceFile(airspaces, buffered_reader);
  } catch (...) {
    // TODO translate this?
    std::throw_with_nested(FmtRuntimeError("Error in file {}", path));
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
  LogString("ReadAirspace");
  operation.SetText(_("Loading Airspace File..."));

  bool airspace_ok = false;

  // Read the airspace filenames from the registry
  const auto paths = Profile::GetMultiplePaths(ProfileKeys::AirspaceFileList);
  for (const auto& path : paths) {
  airspace_ok |= ParseAirspaceFile(airspaces, path, operation);
  }

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
