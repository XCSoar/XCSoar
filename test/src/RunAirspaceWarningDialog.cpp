/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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
#include "Interface.hpp"
#include "Dialogs.h"
#include "MapWindow.hpp"
#include "UtilsSystem.hpp"
#include "Profile/Profile.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Airspace/AirspaceParser.hpp"
#include "Airspace/AirspaceWarningManager.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Waypoint/Waypoints.hpp"
#include "GlideComputerInterface.hpp"
#include "Task/TaskManager.hpp"
#include "Screen/Blank.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Screen/Layout.hpp"
#include "ResourceLoader.hpp"
#include "Appearance.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/ConfiguredFile.hpp"

#include <tchar.h>
#include <stdio.h>

#ifdef HAVE_BLANK
int DisplayTimeOut = 0;
#endif

unsigned InfoBoxLayout::ControlWidth = 100;

#ifndef ENABLE_SDL
bool
MapWindow::identify(HWND hWnd)
{
  return false;
}
#endif /* !ENABLE_SDL */

static Waypoints way_points;
static TaskBehaviour task_behaviour;
static TaskEvents task_events;
static TaskManager task_manager(task_events, way_points);

static Airspaces airspace_database;
static AIRCRAFT_STATE ac_state; // dummy
static AirspaceWarningManager airspace_warning(airspace_database,
                                               ac_state, task_manager);

ProtectedAirspaceWarningManager airspace_warnings(airspace_warning);

static void
LoadFiles()
{
  TLineReader *reader = OpenConfiguredTextFile(szProfileAirspaceFile);
  if (reader != NULL) {
    ReadAirspace(airspace_database, *reader);
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
#ifdef WIN32
  ResourceLoader::Init(hInstance);
  CommonInterface::hInst = hInstance;
  PaintWindow::register_class(hInstance);
#endif

  LoadFiles();

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
  if (Appearance.UseCustomFonts)
    Fonts::LoadCustom();

  dlgAirspaceWarningInit(main_window);
  dlgAirspaceWarningShowDlg();
  dlgAirspaceWarningDeInit();

  return 0;
}
