// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/window/SingleWindow.hpp"
#include "ui/window/Init.hpp"
#include "Dialogs/Dialogs.h"
#include "LocalPath.hpp"
#include "WayPointParser.h"
#include "Airspace/AirspaceClientUI.hpp"
#include "TaskClientUI.hpp"
#include "Task/TaskManager.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Interface.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Logger/Logger.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "LocalPath.hpp"
#include "Look/GlobalFonts.hpp"

#include <tchar.h>
#include <stdio.h>

void
DeviceBlackboard::SetStartupLocation(const GeoPoint &loc,
                                     const double alt)
{
}

Projection::Projection() {}

static RasterTerrain terrain;
Logger logger;

InterfaceBlackboard CommonInterface::blackboard;

Logger::Logger() {}
Logger::~Logger() {}
void Logger::LoggerDeviceDeclare() {}

void RasterTerrain::Lock() {}
void RasterTerrain::Unlock() {}

void dlgAnalysisShowModal() {}
void dlgTaskCalculatorShowModal(SingleWindow &parent) {}

Waypoints way_points;
static TaskBehaviour task_behaviour;
static TaskEvents task_events;
static TaskManager task_manager(task_events, task_behaviour, way_points);

static Airspaces airspace_database;

static AirspaceWarningManager airspace_warning(airspace_database,
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
  const char start_name[] = _T("Bergneustadt");

  task_manager.set_factory(OrderedTask::FactoryType::MIXED);
  AbstractTaskFactory &factory = task_manager.GetFactory();

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

#ifndef _WIN32
int main(int argc, char **argv)
#else
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPSTR lpCmdLine2,
        int nCmdShow)
#endif
{
  InitialiseDataPath();
  ScreenGlobalInit screen_init;

  LoadFiles();

  CreateDefaultTask(task_manager, way_points);

  SingleWindow main_window;
  main_window.set(_T("STATIC"), _T("RunTaskEditorDialog"),
                  0, 0, 640, 480);
  main_window.Show();

  Fonts::Initialize();

  dlgTaskOverviewShowModal(main_window);

  Fonts::Deinitialize();
  DeinitialiseDataPath();

  return 0;
}
