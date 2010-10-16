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
#include "LocalPath.hpp"
#include "WayPointParser.h"
#include "Airspace/AirspaceClientUI.hpp"
#include "TaskClientUI.hpp"
#include "Task/TaskManager.hpp"
#include "Screen/Blank.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Screen/Layout.hpp"
#include "Interface.hpp"
#include "DeviceBlackboard.hpp"
#include "Logger/Logger.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"

#include <tchar.h>
#include <stdio.h>

#ifdef HAVE_BLANK
int DisplayTimeOut = 0;
#endif

unsigned InfoBoxLayout::ControlWidth = 100;

void
DeviceBlackboard::SetStartupLocation(const GeoPoint &loc,
                                     const double alt)
{
}

void dlgStartPointShowModal() {}

Projection::Projection() {}
fixed Projection::GetMapScaleUser() const { return fixed_one; }

MapWindowProjection::MapWindowProjection() {}
fixed MapWindowProjection::GetMapScaleUser() const { return fixed_one; }

SettingsComputerBlackboard::SettingsComputerBlackboard() {}
SettingsMapBlackboard::SettingsMapBlackboard() {}

DeviceBlackboard device_blackboard;
static RasterTerrain terrain;
Logger logger;

InterfaceBlackboard CommonInterface::blackboard;

Logger::Logger() {}
Logger::~Logger() {}
void Logger::LoggerDeviceDeclare() {}
bool Logger::CheckDeclaration() { return true; }

void RasterTerrain::Lock(void) {}
void RasterTerrain::Unlock(void) {}

#ifndef ENABLE_SDL
bool
MapWindow::identify(HWND hWnd)
{
  return false;
}
#endif /* !ENABLE_SDL */

void dlgAnalysisShowModal(void) {}
void dlgTaskCalculatorShowModal(SingleWindow &parent) {}

Waypoints way_points;
static TaskBehaviour task_behaviour;
static TaskEvents task_events;
static TaskManager task_manager(task_events, task_behaviour, way_points);

static Airspaces airspace_database;

static AIRCRAFT_STATE ac_state; // dummy

static AirspaceWarningManager airspace_warning(airspace_database, ac_state,
                                               task_manager);

AirspaceClientUI airspace_ui(airspace_database, airspace_warning);

static void
LoadFiles()
{
  WayPointParser::ReadWaypoints(way_points, &terrain);
}

static void
CreateDefaultTask(TaskManager &task_manager, const Waypoints &way_points)
{
  const TCHAR start_name[] = _T("Bergneustadt");

  task_manager.set_factory(OrderedTask::FACTORY_MIXED);
  AbstractTaskFactory &factory = task_manager.get_factory();

  const Waypoint *wp;
  OrderedTaskPoint *tp;

  wp = way_points.lookup_name(start_name);
  if (wp != NULL) {
    tp = factory.createStart(AbstractTaskFactory::START_LINE, *wp);
    if (!factory.append(tp, false)) {
      fprintf(stderr, "Failed to create start point\n");
    }
  } else {
    fprintf(stderr, "No start waypoint\n");
  }

  wp = way_points.lookup_name(_T("Uslar"));
  if (wp != NULL) {
    tp = factory.createIntermediate(AbstractTaskFactory::AST_CYLINDER, *wp);
    if (!factory.append(tp, false)) {
      fprintf(stderr, "Failed to create turn point\n");
    }
  } else {
    fprintf(stderr, "No turn point\n");
  }

  wp = way_points.lookup_name(_T("Suhl Goldlaut"));
  if (wp != NULL) {
    tp = factory.createIntermediate(AbstractTaskFactory::AST_CYLINDER, *wp);
    if (!factory.append(tp, false)) {
      fprintf(stderr, "Failed to create turn point\n");
    }
  } else {
    fprintf(stderr, "No turn point\n");
  }

  wp = way_points.lookup_name(start_name);
  if (wp != NULL) {
    tp = factory.createFinish(AbstractTaskFactory::FINISH_LINE, *wp);
    if (!factory.append(tp, false)) {
      fprintf(stderr, "Failed to create finish point\n");
    }
  } else {
    fprintf(stderr, "No finish waypoint\n");
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

  CreateDefaultTask(task_manager, way_points);

  SingleWindow main_window;
  main_window.set(_T("STATIC"), _T("RunTaskEditorDialog"),
                  0, 0, 640, 480);
  ((Window &)main_window).install_wndproc();
  main_window.show();

  Layout::Initialize(640, 480);
  Fonts::Initialize();

  dlgTaskOverviewShowModal(main_window);

  return 0;
}
