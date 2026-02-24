// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UtilsSettings.hpp"
#include "Protection.hpp"
#include "MainWindow.hpp"
#include "Computer/Settings.hpp"
#include "MapSettings.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Waypoint/WaypointDetailsReader.hpp"
#include "Topography/TopographyStore.hpp"
#include "Topography/TopographyGlue.hpp"
#include "Dialogs/Dialogs.h"
#include "Device/device.hpp"
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
#include "FLARM/Glue.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "Weather/Rasp/Configured.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"
#include "DataGlobals.hpp"

bool DevicePortChanged = false;
bool MapFileChanged = false;
bool AirspaceFileChanged = false;
bool AirfieldFileChanged = false;
bool WaypointFileChanged = false;
bool FlarmFileChanged = false;
bool RaspFileChanged = false;
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
  FlarmFileChanged = false;
  RaspFileChanged = false;
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

  if (TerrainFileChanged)
    main_window.LoadTerrain();

  auto &way_points = *data_components->waypoints;

  if (WaypointFileChanged || AirfieldFileChanged) {
    // re-load waypoints
    WaypointGlue::LoadWaypoints(way_points, data_components->terrain.get(),
                                operation);

    try {
      WaypointDetails::ReadFileFromProfile(way_points, operation);
    } catch (...) {
      LogError(std::current_exception());
    }
  }

  if (WaypointFileChanged &&
      backend_components->protected_task_manager) {
    ProtectedTaskManager::ExclusiveLease lease{*backend_components->protected_task_manager};
    auto task = lease->Clone(CommonInterface::GetComputerSettings().task);
    if (task) {
      // this must be done in thread lock because it potentially changes the
      // waypoints database
      task->CheckDuplicateWaypoints(way_points);

      /* XXX shall this task be committed if it has been modified? */

      way_points.Optimise();
    }
  }

  if (WaypointFileChanged) {
    // re-set home
    DataGlobals::UpdateHome(WaypointFileChanged);
  }

  if (TopographyFileChanged) {
    main_window.SetTopography(nullptr);

    auto &topography = *data_components->topography;
    topography.Reset();
    LoadConfiguredTopography(topography);
    main_window.SetTopography(&topography);
  }

  if (AirspaceFileChanged) {
    if (backend_components->glide_computer) {
      backend_components->glide_computer->GetAirspaceWarnings().Clear();
      backend_components->glide_computer->ClearAirspaces();
    }

    auto &airspace_database = *data_components->airspaces;
    airspace_database.Clear();
    ReadAirspace(airspace_database,
                 CommonInterface::GetComputerSettings().pressure,
                 operation);

    if (data_components->terrain)
      SetAirspaceGroundLevels(airspace_database, *data_components->terrain);
  }

  if (DevicePortChanged && backend_components->devices != nullptr)
    devRestart(*backend_components->devices,
               CommonInterface::GetSystemSettings());

  if (FlarmFileChanged) {
    ReloadFlarmDatabases();
  }

  if (RaspFileChanged)
    DataGlobals::SetRasp(LoadConfiguredRasp(false));

  const UISettings &ui_settings = CommonInterface::GetUISettings();

  Units::SetConfig(ui_settings.format.units);
  SetUserCoordinateFormat(ui_settings.format.coordinate_format);

  const MapSettings &old_settings_map = old_ui_settings.map;
  const MapSettings &settings_map = ui_settings.map;

  if (ui_settings.dark_mode != old_ui_settings.dark_mode ||
      ui_settings.info_boxes.use_colors != old_ui_settings.info_boxes.use_colors ||
      ui_settings.info_boxes.theme != old_ui_settings.info_boxes.theme ||
      settings_map.trail.type != old_settings_map.trail.type ||
      settings_map.trail.scaling_enabled != old_settings_map.trail.scaling_enabled ||
      settings_map.waypoint.landable_style != old_settings_map.waypoint.landable_style)
    main_window.ReinitialiseLook();

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
