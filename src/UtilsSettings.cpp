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

#include "UtilsSettings.hpp"
#include "Protection.hpp"
#include "MainWindow.hpp"
#include "SettingsComputer.hpp"
#include "SettingsMap.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "AirfieldDetails.h"
#include "Topography/TopographyStore.hpp"
#include "Topography/TopographyGlue.hpp"
#include "Dialogs/Dialogs.h"
#include "Device/device.hpp"
#include "Message.hpp"
#include "Polar/PolarGlue.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "Language.hpp"
#include "LogFile.hpp"
#include "Simulator.hpp"
#include "DrawThread.hpp"
#include "Airspace/AirspaceGlue.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "ProgressGlue.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "WayPoint/WayPointGlue.hpp"
#include "GlideComputer.hpp"
#include "LanguageGlue.hpp"

#if defined(__BORLANDC__)  // due to compiler bug
  #include "Waypoint/Waypoints.hpp"
#endif

bool DevicePortChanged = false;
bool MapFileChanged = false;
bool AirspaceFileChanged = false;
bool AirfieldFileChanged = false;
bool WaypointFileChanged = false;
bool TerrainFileChanged = false;
bool TopographyFileChanged = false;
bool PolarFileChanged = false;
bool LanguageFileChanged = false;
bool StatusFileChanged = false;
bool InputFileChanged = false;
bool LanguageChanged = false;

static void
SettingsEnter()
{
#ifndef ENABLE_OPENGL
  draw_thread->suspend();
#endif

  // This prevents the map and calculation threads from doing anything
  // with shared data while it is being changed (also prevents drawing)

  MapFileChanged = false;
  AirspaceFileChanged = false;
  AirfieldFileChanged = false;
  WaypointFileChanged = false;
  TerrainFileChanged = false;
  TopographyFileChanged = false;
  PolarFileChanged = false;
  LanguageFileChanged = false;
  StatusFileChanged = false;
  InputFileChanged = false;
  DevicePortChanged = false;
  LanguageChanged = false;
}

static void
SettingsLeave()
{
  if (!globalRunningEvent.test())
    return;

  XCSoarInterface::main_window.map.set_focus();

  SuspendAllThreads();

  if (LanguageChanged)
    ReadLanguageFile();

  if (MapFileChanged) {
    /* set these flags, because they may be loaded from the map
       file */
    AirspaceFileChanged = true;
    AirfieldFileChanged = true;
    WaypointFileChanged = true;
    TerrainFileChanged = true;
    TopographyFileChanged = true;
  }

  if (TerrainFileChanged) {
    ProgressGlue::Create(_("Loading Terrain File..."));

    XCSoarInterface::main_window.map.set_terrain(NULL);
    glide_computer->set_terrain(NULL);

    // re-load terrain
    delete terrain;
    terrain = RasterTerrain::OpenTerrain(file_cache);

    XCSoarInterface::main_window.map.set_terrain(terrain);
    glide_computer->set_terrain(terrain);
  }

  if (WaypointFileChanged || AirfieldFileChanged) {
    // re-load waypoints
    WayPointGlue::LoadWaypoints(way_points, terrain);
    ReadAirfieldFile(way_points);
  }

  if (WaypointFileChanged || TerrainFileChanged) {
    // re-set home
    WayPointGlue::SetHome(way_points, terrain,
                          XCSoarInterface::SetSettingsComputer(),
                          WaypointFileChanged);
  }

  if (TopographyFileChanged) {
    XCSoarInterface::main_window.map.set_topography(NULL);
    topography->Reset();
    LoadConfiguredTopography(*topography);
    XCSoarInterface::main_window.map.set_topography(topography);
  }

  if (AirspaceFileChanged) {
    if (airspace_warnings != NULL)
      airspace_warnings->clear();
    airspace_database.clear();
    ReadAirspace(airspace_database, terrain,
                 CommonInterface::SettingsComputer().pressure);
  }

  if (protected_task_manager != NULL) {
    ProtectedTaskManager::ExclusiveLease lease(*protected_task_manager);
    lease->set_contest(XCSoarInterface::SettingsComputer().contest);
  }

  if (AirfieldFileChanged
      || AirspaceFileChanged
      || WaypointFileChanged
      || TerrainFileChanged
      || TopographyFileChanged) {
    ProgressGlue::Close();
    XCSoarInterface::main_window.map.set_focus();
#ifndef ENABLE_OPENGL
    draw_thread->trigger_redraw();
#endif
  }

  if (DevicePortChanged)
    devRestart();

  ResumeAllThreads();
  // allow map and calculations threads to continue

  ActionInterface::SendSettingsMap(true);
}

void
SystemConfiguration()
{
  SettingsEnter();
  dlgConfigurationShowModal();
  SettingsLeave();
}
