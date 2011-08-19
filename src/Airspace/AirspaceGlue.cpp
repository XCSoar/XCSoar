/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Terrain/RasterTerrain.hpp"
#include "Operation.hpp"
#include "Language/Language.hpp"
#include "LogFile.hpp"
#include "IO/ConfiguredFile.hpp"

void
ReadAirspace(Airspaces &airspaces,
             RasterTerrain *terrain,
             const AtmosphericPressure &press,
             OperationEnvironment &operation)
{
  LogStartUp(_T("ReadAirspace"));
  operation.SetText(_("Loading Airspace File..."));

  bool airspace_ok = false;

  AirspaceParser parser(airspaces);

  // Read the airspace filenames from the registry
  TLineReader *reader =
    OpenConfiguredTextFile(szProfileAirspaceFile, _T("airspace.txt"),
                           ConvertLineReader::AUTO);
  if (reader != NULL) {
    if (!parser.Parse(*reader, operation))
      LogStartUp(_T("No airspace file 1"));
    else
      airspace_ok =  true;

    delete reader;
  }

  reader = OpenConfiguredTextFile(szProfileAdditionalAirspaceFile,
                                  ConvertLineReader::AUTO);
  if (reader != NULL) {
    if (!parser.Parse(*reader, operation))
      LogStartUp(_T("No airspace file 2"));
    else
      airspace_ok = true;

    delete reader;
  }

  if (airspace_ok) {
    airspaces.optimise();
    airspaces.set_flight_levels(press);

    if (terrain != NULL)
      airspaces.set_ground_levels(*terrain);
  } else
    // there was a problem
    airspaces.clear();
}
