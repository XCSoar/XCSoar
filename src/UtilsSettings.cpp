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

#include "UtilsSettings.hpp"
#include "Profile/Current.hpp"
#include "Protection.hpp"
#include "Look/Look.hpp"
#include "MainWindow.hpp"
#include "Computer/Settings.hpp"
#include "MapSettings.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Waypoint/WaypointDetailsReader.hpp"
#include "Topography/TopographyStore.hpp"
#include "Topography/TopographyGlue.hpp"
#include "Dialogs/Dialogs.h"
#include "Device/device.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "Language/Language.hpp"
#include "LogFile.hpp"
#include "DrawThread.hpp"
#include "Airspace/AirspaceGlue.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Operation/VerboseOperationEnvironment.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Waypoint/WaypointGlue.hpp"
#include "Computer/GlideComputer.hpp"
#include "Language/LanguageGlue.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserGeoPointFormatter.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Audio/Features.hpp"
#include "Audio/GlobalVolumeController.hpp"
#include "Audio/VarioGlue.hpp"
#include "Audio/VolumeController.hpp"
#include "PageActions.hpp"

#if defined(__BORLANDC__)  // due to compiler bug
  #include "Waypoint/Waypoints.hpp"
#endif

bool DevicePortChanged = false;
bool MapFileChanged = false;
bool AirspaceFileChanged = false;
bool AirfieldFileChanged = false;
bool WaypointFileChanged = false;
bool InputFileChanged = false;
bool LanguageChanged = false;
bool require_restart;

static void
SettingsEnter()
{
  CommonInterface::main_window->SuspendThreads();

  // This prevents the map and calculation threads from doing anything
  // with shared data while it is being changed (also prevents drawing)

  MapFileChanged = false;
  AirspaceFileChanged = false;
  AirfieldFileChanged = false;
  WaypointFileChanged = false;
  InputFileChanged = false;
  DevicePortChanged = false;
  LanguageChanged = false;
  require_restart = false;
}

static void
SettingsLeave(const UISettings &old_ui_settings)
{
  if (!global_running)
    return;

  SuspendAllThreads();

  VerboseOperationEnvironment operation;

  MainWindow &main_window = *CommonInterface::main_window;

  if (LanguageChanged)
    ReadLanguageFile();

  bool TerrainFileChanged = false, TopographyFileChanged = false;
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
    operation.SetText(_("Loading Terrain File..."));

    /* just in case the bottom widget uses the old terrain object
       (e.g. the cross section) */
    main_window.SetBottomWidget(nullptr);

    main_window.SetTerrain(nullptr);
    glide_computer->SetTerrain(nullptr);

    // re-load terrain
    delete terrain;
    terrain = RasterTerrain::OpenTerrain(file_cache, operation);

    main_window.SetTerrain(terrain);
    glide_computer->SetTerrain(terrain);

    /* re-create the bottom widget if it was deleted here */
    PageActions::Update();
  }

  if (WaypointFileChanged || AirfieldFileChanged) {
    // re-load waypoints
    WaypointGlue::LoadWaypoints(way_points, terrain, operation);
    WaypointDetails::ReadFileFromProfile(way_points, operation);
  }

  if (WaypointFileChanged && protected_task_manager != nullptr) {
    ProtectedTaskManager::ExclusiveLease lease(*protected_task_manager);
    OrderedTask *task = lease->Clone(CommonInterface::GetComputerSettings().task);
    if (task) {
      // this must be done in thread lock because it potentially changes the
      // waypoints database
      task->CheckDuplicateWaypoints(way_points);

      /* XXX shall this task be committed if it has been modified? */
      delete task;

      way_points.Optimise();
    }
  }

  if (WaypointFileChanged || TerrainFileChanged) {
    // re-set home
    WaypointGlue::SetHome(way_points, terrain,
                          CommonInterface::SetComputerSettings().poi,
                          CommonInterface::SetComputerSettings().team_code,
                          device_blackboard, WaypointFileChanged);
    WaypointGlue::SaveHome(Profile::map,
                           CommonInterface::GetComputerSettings().poi,
                           CommonInterface::GetComputerSettings().team_code);
  }

  if (TopographyFileChanged) {
    main_window.SetTopography(nullptr);
    topography->Reset();
    LoadConfiguredTopography(*topography, operation);
    main_window.SetTopography(topography);
  }

  if (AirspaceFileChanged) {
    if (glide_computer != nullptr)
      glide_computer->GetAirspaceWarnings().Clear();

    if (glide_computer != nullptr)
      glide_computer->ClearAirspaces();

    airspace_database.Clear();
    ReadAirspace(airspace_database, terrain,
                 CommonInterface::GetComputerSettings().pressure,
                 operation);
  }

  if (DevicePortChanged)
    devRestart();

  const UISettings &ui_settings = CommonInterface::GetUISettings();

  Units::SetConfig(ui_settings.format.units);
  SetUserCoordinateFormat(ui_settings.format.coordinate_format);

  const MapSettings &old_settings_map = old_ui_settings.map;
  const MapSettings &settings_map = ui_settings.map;

  if (settings_map.trail.type != old_settings_map.trail.type ||
      settings_map.trail.scaling_enabled != old_settings_map.trail.scaling_enabled)
    main_window.SetLook().map.trail.Initialise(settings_map.trail);

  if (settings_map.waypoint.landable_style != old_settings_map.waypoint.landable_style)
    main_window.SetLook().map.waypoint.Reinitialise(settings_map.waypoint);

  ResumeAllThreads();
  main_window.ResumeThreads();
  // allow map and calculations threads to continue

  ActionInterface::SendMapSettings(true);

#ifdef HAVE_VOLUME_CONTROLLER
  volume_controller->SetVolume(ui_settings.sound.master_volume);
#endif

  AudioVarioGlue::Configure(CommonInterface::GetUISettings().sound.vario);

  operation.Hide();
  InfoBoxManager::SetDirty();
  main_window.FlushRendererCaches();
  main_window.FullRedraw();
  main_window.SetDefaultFocus();
}

void
SystemConfiguration()
{
  const UISettings old_ui_settings = CommonInterface::GetUISettings();

  SettingsEnter();
  dlgConfigurationShowModal();
  SettingsLeave(old_ui_settings);
}
