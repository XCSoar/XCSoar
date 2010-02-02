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
#include "InputEvents.h"
#include "UtilsSystem.hpp"
#include "LocalPath.hpp"
#include "wcecompat/ts_string.h"
#include "Registry.hpp"
#include "RasterTerrain.h"
#include "AirspaceParser.hpp"
#include "Airspace/AirspaceWarningManager.hpp"
#include "Waypoint/Waypoints.hpp"
#include "GlideComputerInterface.hpp"
#include "Task/TaskManager.hpp"
#include "Screen/Blank.hpp"
#include "InfoBoxLayout.hpp"
#include "Screen/Layout.hpp"

#include <tchar.h>
#include <stdio.h>

#if defined(PNA) || defined(FIVV)
int GlobalModelType = MODELTYPE_UNKNOWN;
bool needclipping = false;
#endif

#ifdef HAVE_BLANK
int DisplayTimeOut = 0;
#endif

int InfoBoxLayout::ControlWidth = 100;

bool
FileExistsA(const char *FileName)
{
  FILE *file = fopen(FileName, "r");
  if (file != NULL) {
    fclose(file);
    return true;
  }

  return false;
}

void
StartupStore(const TCHAR *Str, ...)
{
  va_list ap;

  va_start(ap, Str);
  _vftprintf(stderr, Str, ap);
  va_end(ap);
}

const TCHAR *
gettext(const TCHAR *text)
{
  return text;
}

int WINAPI
MessageBoxX(LPCTSTR lpText, LPCTSTR lpCaption, UINT uType)
{
  return -1;
}

bool XCSoarInterface::Debounce() { return false; }
void XCSoarInterface::InterfaceTimeoutReset(void) {}

void
XCSoarInterface::CreateProgressDialog(const TCHAR* text)
{
  _ftprintf(stderr, _T("%s\n"), text);
}

void XCSoarInterface::StepProgressDialog(void) {}
bool XCSoarInterface::SetProgressStepSize(int nSize) {
  return false;
}

pt2Event
InputEvents::findEvent(const TCHAR *)
{
  return NULL;
}

RasterTerrain terrain;

void RasterTerrain::Lock(void) {}
void RasterTerrain::Unlock(void) {}

#ifndef ENABLE_SDL
bool
MapWindow::identify(HWND hWnd)
{
  return false;
}
#endif /* !ENABLE_SDL */

HINSTANCE CommonInterface::hInst;
bool CommonInterface::EnableAnimation;

Waypoints way_points;
TaskBehaviour task_behaviour;
TaskEvents task_events;

TaskManager task_manager(task_events,
                         task_behaviour,
                         way_points);

Airspaces airspace_database;

AIRCRAFT_STATE ac_state; // dummy

AirspaceWarningManager airspace_warning(airspace_database,
                                        ac_state,
                                        task_manager);

static void
LoadFiles()
{
  TCHAR tpath[MAX_PATH];
  GetRegistryString(szRegistryAirspaceFile, tpath, MAX_PATH);
  if (tpath[0] != 0) {
    ExpandLocalPath(tpath);

    char path[MAX_PATH];
    unicode2ascii(tpath, path, sizeof(path));

    ReadAirspace(airspace_database, path);
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
  CommonInterface::hInst = hInstance;
  PaintWindow::register_class(hInstance);
#endif

  LoadFiles();

  Airspaces::AirspaceTree::const_iterator it = airspace_database.begin();

  AirspaceInterceptSolution ais;
  for (unsigned i = 0; i < 5 && it != airspace_database.end(); ++i, ++it)
    airspace_warning.get_warning(*it->get_airspace())
      .update_solution((AirspaceWarning::AirspaceWarningState)i, ais);

  SingleWindow main_window;
  main_window.set(_T("STATIC"), _T("RunAirspaceWarningDialog"),
                  0, 0, 640, 480);
  main_window.show();

  Layout::Initalize(640, 480);
  InitialiseFonts(main_window.get_client_rect());

  dlgAirspaceWarningInit(main_window);
  dlgAirspaceWarningShowDlg();
  dlgAirspaceWarningDeInit();

  return 0;
}
