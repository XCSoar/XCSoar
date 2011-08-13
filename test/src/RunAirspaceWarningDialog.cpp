/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Screen/SingleWindow.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Init.hpp"
#include "Interface.hpp"
#include "Dialogs/Airspace.hpp"
#include "Dialogs/AirspaceWarningDialog.hpp"
#include "UtilsSystem.hpp"
#include "Profile/Profile.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Airspace/AirspaceParser.hpp"
#include "Airspace/AirspaceWarningManager.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Computer/GlideComputerInterface.hpp"
#include "Task/TaskManager.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Screen/Layout.hpp"
#include "ResourceLoader.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/ConfiguredFile.hpp"
#include "LocalPath.hpp"
#include "Components.hpp"
#include "Operation.hpp"

#include <tchar.h>
#include <stdio.h>

ProtectedAirspaceWarningManager *airspace_warnings;

void dlgAirspaceDetails(const AbstractAirspace& the_airspace) {}

static void
LoadFiles(Airspaces &airspace_database)
{
  NullOperationEnvironment operation;

  TLineReader *reader = OpenConfiguredTextFile(szProfileAirspaceFile);
  if (reader != NULL) {
    ReadAirspace(airspace_database, *reader, operation);
    delete reader;

    airspace_database.optimise();
  }
}

#ifndef WIN32
int main(int argc, char **argv)
#else
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
#ifdef _WIN32_WCE
        LPWSTR lpCmdLine,
#else
        LPSTR lpCmdLine2,
#endif
        int nCmdShow)
#endif
{
  Waypoints way_points;
  TaskEvents task_events;
  TaskManager task_manager(task_events, way_points);

  Airspaces airspace_database;
  AirspaceWarningManager airspace_warning(airspace_database, task_manager);
  airspace_warnings = new ProtectedAirspaceWarningManager(airspace_warning);

  InitialiseDataPath();
  ScreenGlobalInit screen_init;

#ifdef WIN32
  ResourceLoader::Init(hInstance);
#endif

  LoadFiles(airspace_database);

  Fonts::Initialize();

  Airspaces::AirspaceTree::const_iterator it = airspace_database.begin();

  AirspaceInterceptSolution ais;
  for (unsigned i = 0; i < 5 && it != airspace_database.end(); ++i, ++it)
    airspace_warning.get_warning(*it->get_airspace())
      .update_solution((AirspaceWarning::AirspaceWarningState)i, ais);

  SingleWindow main_window;
  main_window.set(_T("STATIC"), _T("RunAirspaceWarningDialog"),
                  0, 0, 640, 480);
  main_window.show();

  Layout::Initialize(640, 480);

  Fonts::Initialize();

  dlgAirspaceWarningsShowModal(main_window);

  Fonts::Deinitialize();
  DeinitialiseDataPath();

  delete airspace_warnings;

  return 0;
}
