/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Screen/Init.hpp"
#include "Interface.hpp"
#include "Look/Fonts.hpp"
#include "Look/DialogLook.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/Airspace/Airspace.hpp"
#include "Dialogs/Airspace/AirspaceWarningDialog.hpp"
#include "Dialogs/DialogSettings.hpp"
#include "UIGlobals.hpp"
#include "Profile/Profile.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Airspace/AirspaceParser.hpp"
#include "Airspace/AirspaceWarningManager.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Screen/Layout.hpp"
#include "ResourceLoader.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/ConfiguredFile.hpp"
#include "LocalPath.hpp"
#include "Components.hpp"
#include "Operation/Operation.hpp"

#include <memory>
#include <tchar.h>
#include <stdio.h>

InterfaceBlackboard CommonInterface::Private::blackboard;

ProtectedAirspaceWarningManager *airspace_warnings;

static DialogSettings dialog_settings;
static DialogLook dialog_look;

const DialogSettings &
UIGlobals::GetDialogSettings()
{
  return dialog_settings;
}

const DialogLook &
UIGlobals::GetDialogLook()
{
  return dialog_look;
}

void
dlgAirspaceDetails(const AbstractAirspace &the_airspace,
                   ProtectedAirspaceWarningManager *airspace_warnings)
{
}

static void
LoadFiles(Airspaces &airspace_database)
{
  NullOperationEnvironment operation;

  std::unique_ptr<TLineReader> reader(OpenConfiguredTextFile(ProfileKeys::AirspaceFile,
                                                             ConvertLineReader::AUTO));
  if (reader) {
    AirspaceParser parser(airspace_database);
    parser.Parse(*reader, operation);
    airspace_database.Optimise();
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
  Airspaces airspace_database;
  AirspaceWarningManager airspace_warning(airspace_database);
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
    airspace_warning.GetWarning(*it->GetAirspace())
      .UpdateSolution((AirspaceWarning::State)i, ais);

  SingleWindow main_window;
  main_window.Create(_T("STATIC"), _T("RunAirspaceWarningDialog"),
                     PixelRect{0, 0, 640, 480});
  main_window.Show();

  Layout::Initialize(640, 480);

  Fonts::Initialize();

  dialog_settings.SetDefaults();

  dialog_look.Initialise(Fonts::map_bold, Fonts::map, Fonts::map_label,
                         Fonts::map_bold, Fonts::map_bold);
  SetXMLDialogLook(dialog_look);

  dlgAirspaceWarningsShowModal(main_window, *airspace_warnings);

  Fonts::Deinitialize();
  DeinitialiseDataPath();

  delete airspace_warnings;

  return 0;
}
