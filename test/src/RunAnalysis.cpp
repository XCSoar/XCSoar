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
#include "Screen/Blank.hpp"
#include "Screen/BufferCanvas.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Init.hpp"
#include "ResourceLoader.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Logger/Logger.hpp"
#include "Terrain/RasterWeather.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Waypoint/WaypointGlue.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/dlgAnalysis.hpp"
#include "Dialogs/Task.hpp"
#include "Dialogs/Dialogs.h"
#include "Dialogs/AirspaceWarningDialog.hpp"
#include "Dialogs/DialogSettings.hpp"
#include "UIGlobals.hpp"
#include "Airspace/AirspaceParser.hpp"
#include "Airspace/AirspaceGlue.hpp"
#include "Profile/Profile.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/TaskEvents.hpp"
#include "Computer/BasicComputer.hpp"
#include "Computer/GlideComputer.hpp"
#include "Computer/GlideComputerInterface.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/ProtectedRoutePlanner.hpp"
#include "Task/TaskFile.hpp"
#include "LocalPath.hpp"
#include "Blackboard/InterfaceBlackboard.hpp"
#include "DebugReplay.hpp"
#include "IO/FileLineReader.hpp"
#include "Operation/Operation.hpp"
#include "Look/Look.hpp"
#include "OS/Args.hpp"

#ifdef WIN32
#include <shellapi.h>
#endif

/* fake symbols: */

#include "Computer/ConditionMonitor/ConditionMonitors.hpp"
#include "Input/InputQueue.hpp"
#include "Logger/Logger.hpp"
#include "LocalTime.hpp"

OrderedTask *
TaskFile::GetTask(const TCHAR *path, const TaskBehaviour &task_behaviour,
                  const Waypoints *waypoints, unsigned index)
{
  return NULL;
}

static DialogSettings dialog_settings;
static const DialogLook *dialog_look;

const DialogSettings &
UIGlobals::GetDialogSettings()
{
  return dialog_settings;
}

const DialogLook &
UIGlobals::GetDialogLook()
{
  return *dialog_look;
}

void dlgBasicSettingsShowModal() {}
void ShowWindSettingsDialog() {}

void
dlgAirspaceWarningsShowModal(SingleWindow &parent,
                             ProtectedAirspaceWarningManager &warnings,
                             bool auto_close)
{
}

void dlgTaskManagerShowModal(SingleWindow &parent) {}

void
ConditionMonitorsUpdate(const NMEAInfo &basic, const DerivedInfo &calculated,
                        const ComputerSettings &settings)
{
}

bool InputEvents::processGlideComputer(unsigned) { return false; }

void Logger::LogStartEvent(const NMEAInfo &gps_info) {}
void Logger::LogFinishEvent(const NMEAInfo &gps_info) {}
void Logger::LogPoint(const NMEAInfo &gps_info) {}

bool
InputEvents::processNmea(unsigned key)
{
  return true;
}

/* done with fake symbols. */

static RasterTerrain *terrain;

static void
LoadFiles(Airspaces &airspace_database)
{
  NullOperationEnvironment operation;

  terrain = RasterTerrain::OpenTerrain(NULL, operation);

  const AtmosphericPressure pressure = AtmosphericPressure::Standard();
  ReadAirspace(airspace_database, terrain, pressure, operation);
}

static void
LoadReplay(DebugReplay *replay, GlideComputer &glide_computer,
           InterfaceBlackboard &blackboard)
{
  unsigned i = 0;
  while (replay->Next()) {
    const MoreData &basic = replay->Basic();

    glide_computer.ReadBlackboard(basic);
    glide_computer.ProcessGPS();

    if (++i == 8) {
      i = 0;
      glide_computer.ProcessIdle();
    }
  }

  glide_computer.ProcessExhaustive();

  blackboard.ReadBlackboardBasic(glide_computer.Basic());
  blackboard.ReadBlackboardCalculated(glide_computer.Calculated());
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
  Args args(GetCommandLine(), "DRIVER FILE");
#else
  Args args(argc, argv, "DRIVER FILE");
#endif
  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    return EXIT_FAILURE;

  args.ExpectEnd();

  InitialiseDataPath();
  Profile::SetFiles(_T(""));
  Profile::Load();

  const Waypoints way_points;

  InterfaceBlackboard blackboard;
  blackboard.SetComputerSettings().SetDefaults();
  blackboard.SetUISettings().SetDefaults();

  TaskBehaviour task_behaviour;
  task_behaviour.SetDefaults();

  TaskManager task_manager(task_behaviour, way_points);
  GlideComputerTaskEvents task_events;
  task_manager.SetTaskEvents(task_events);

  Airspaces airspace_database;
  AirspaceWarningManager airspace_warning(airspace_database);

  ProtectedTaskManager protected_task_manager(task_manager,
                                              blackboard.GetComputerSettings().task);

  LoadFiles(airspace_database);

  GlideComputer glide_computer(way_points, airspace_database,
                               protected_task_manager,
                               task_events);
  glide_computer.ReadComputerSettings(blackboard.GetComputerSettings());
  glide_computer.SetTerrain(terrain);
  glide_computer.Initialise();

  ScreenGlobalInit screen_init;

  LoadReplay(replay, glide_computer, blackboard);
  delete replay;

#ifdef WIN32
  ResourceLoader::Init(hInstance);
#endif

  Layout::Initialize(640, 480);

  SingleWindow main_window;
  main_window.Create(_T("STATIC"), _T("RunAnalysis"),
                     PixelRect{0, 0, 640, 480});

  Fonts::Initialize();

  dialog_settings.SetDefaults();

  Look *look = new Look();
  look->Initialise();
  look->InitialiseConfigured(blackboard.GetUISettings());

  SetXMLDialogLook(look->dialog);

  dialog_look = &look->dialog;

  main_window.Show();

  dlgAnalysisShowModal(main_window, *look, blackboard, glide_computer,
                       &protected_task_manager,
                       &airspace_database,
                       terrain);

  delete look;
  Fonts::Deinitialize();

  delete terrain;

  DeinitialiseDataPath();

  return 0;
}
