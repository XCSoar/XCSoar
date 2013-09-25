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

#include "Airspace/AirspaceGlue.hpp"
#include "Airspace/AirspaceParser.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Operation/Operation.hpp"
#include "Language/Language.hpp"
#include "LogFile.hpp"
#include "IO/TextFile.hpp"
#include "Profile/Profile.hpp"

#include <windef.h> /* for MAX_PATH */
#include <memory>

static bool
ParseAirspaceFile(AirspaceParser &parser, const TCHAR *path,
                  OperationEnvironment &operation)
{
  std::unique_ptr<TLineReader> reader(OpenTextFile(path, ConvertLineReader::AUTO));
  if (!reader) {
    LogFormat(_T("Failed to open airspace file: %s"), path);
    return false;
  }

  if (!parser.Parse(*reader, operation)) {
    LogFormat(_T("Failed to parse airspace file: %s"), path);
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
  TCHAR path[MAX_PATH];
  if (Profile::GetPath(ProfileKeys::AirspaceFile, path))
    airspace_ok |= ParseAirspaceFile(parser, path, operation);

  if (Profile::GetPath(ProfileKeys::AdditionalAirspaceFile, path))
    airspace_ok |= ParseAirspaceFile(parser, path, operation);

  if (Profile::GetPath(ProfileKeys::MapFile, path)) {
    _tcscat(path, _T("/airspace.txt"));
    airspace_ok |= ParseAirspaceFile(parser, path, operation);
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
