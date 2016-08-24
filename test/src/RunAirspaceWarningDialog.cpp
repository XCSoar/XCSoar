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

#define ENABLE_DIALOG
#define ENABLE_MAIN_WINDOW

#include "Main.hpp"
#include "Interface.hpp"
#include "Dialogs/Airspace/Airspace.hpp"
#include "Dialogs/Airspace/AirspaceWarningDialog.hpp"
#include "Dialogs/DialogSettings.hpp"
#include "UIGlobals.hpp"
#include "Profile/Profile.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Airspace/AirspaceParser.hpp"
#include "Airspace/AirspaceWarningManager.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "ResourceLoader.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/ConfiguredFile.hpp"
#include "LocalPath.hpp"
#include "Components.hpp"
#include "Operation/Operation.hpp"

#include <memory>
#include <tchar.h>
#include <stdio.h>

void VisitDataFiles(const TCHAR* filter, File::Visitor &visitor) {}

InterfaceBlackboard CommonInterface::Private::blackboard;

ProtectedAirspaceWarningManager *airspace_warnings;

void
dlgAirspaceDetails(const AbstractAirspace &the_airspace,
                   ProtectedAirspaceWarningManager *airspace_warnings)
{
}

static void
LoadFiles(Airspaces &airspace_database)
{
  NullOperationEnvironment operation;

  auto reader = OpenConfiguredTextFile(ProfileKeys::AirspaceFile,
                                       Charset::AUTO);
  if (reader) {
    AirspaceParser parser(airspace_database);
    parser.Parse(*reader, operation);
    airspace_database.Optimise();
  }
}

static void
Main()
{
  Airspaces airspace_database;

  AirspaceWarningConfig airspace_warning_config;
  airspace_warning_config.SetDefaults();

  AirspaceWarningManager airspace_warning(airspace_warning_config,
                                          airspace_database);
  airspace_warnings = new ProtectedAirspaceWarningManager(airspace_warning);

  LoadFiles(airspace_database);

  const auto range = airspace_database.QueryAll();
  auto it = range.begin();

  AirspaceInterceptSolution ais;
  for (unsigned i = 0; i < 5 && it != range.end(); ++i, ++it)
    airspace_warning.GetWarning(it->GetAirspace())
      .UpdateSolution((AirspaceWarning::State)i, ais);

  dlgAirspaceWarningsShowModal(*airspace_warnings);

  delete airspace_warnings;
}
