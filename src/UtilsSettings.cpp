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
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "system/FileUtil.hpp"
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
#include "Repository/Glue.hpp"
#include "Repository/FileType.hpp"
#include "Input/InputEvents.hpp"
#include "LocalPath.hpp"
#include "net/http/Features.hpp"

#ifdef __APPLE__
#include "Apple/DarkMode.hpp"
#endif

bool DevicePortChanged = false;
bool MapFileChanged = false;
bool AirspaceFileChanged = false;
bool AirfieldFileChanged = false;
bool WaypointFileChanged = false;
bool PrimaryWaypointFileChanged = false;
bool WatchedWaypointFileChanged = false;
bool FlarmFileChanged = false;
bool RaspFileChanged = false;
bool ChecklistFileChanged = false;
bool InputFileChanged = false;
bool LanguageChanged = false;
bool UserRepositoriesListChanged = false;

void
SettingsEnter() noexcept
{
  CommonInterface::main_window->SuspendThreads();

  // This prevents the map and calculation threads from doing anything
  // with shared data while it is being changed (also prevents drawing)

  MapFileChanged = false;
  AirspaceFileChanged = false;
  AirfieldFileChanged = false;
  WaypointFileChanged = false;
  PrimaryWaypointFileChanged = false;
  WatchedWaypointFileChanged = false;
  FlarmFileChanged = false;
  RaspFileChanged = false;
  ChecklistFileChanged = false;
  InputFileChanged = false;
  DevicePortChanged = false;
  LanguageChanged = false;
  UserRepositoriesListChanged = false;
}

void
SettingsLeave(const UISettings &old_ui_settings)
{
  if (!global_running)
    return;

  SuspendAllThreads();

  VerboseOperationEnvironment operation;

  MainWindow &main_window = *CommonInterface::main_window;

  if (LanguageChanged) {
    ReadLanguageFile();
    InfoBoxManager::InvalidateAfterLanguageChange();
  }

  if (MapFileChanged) {
    /* set these flags, because they may be loaded from the map
       file */
    AirspaceFileChanged = true;
    AirfieldFileChanged = true;
    WaypointFileChanged = true;
  }

  /* The map path is shared. Terrain and topography are separate
     reloads. OnTerrainLoaded() installs terrain only. The retry
     below loads terrain when the file appears, and leaves
     topography alone. */
  if (MapFileChanged)
    main_window.LoadTerrain();

  /* If the terrain wasn't changed via the settings UI but no terrain is
     currently loaded (e.g. a previous automated load failed), attempt
     to load the configured map file when it now exists. */
  if (!MapFileChanged && data_components->terrain == nullptr) {
    if (const auto path = Profile::GetPath(ProfileKeys::MapFile); path != nullptr) {
      if (File::Exists(path))
        main_window.LoadTerrain();
    }
  }

  auto &way_points = *data_components->waypoints;

  const bool waypoint_files_changed =
    PrimaryWaypointFileChanged || WatchedWaypointFileChanged ||
    MapFileChanged;

  if (waypoint_files_changed)
    WaypointGlue::ReloadWaypointFiles(way_points,
                                      data_components->terrain.get(),
                                      operation,
                                      PrimaryWaypointFileChanged,
                                      WatchedWaypointFileChanged,
                                      MapFileChanged);

  if (waypoint_files_changed || AirfieldFileChanged) {
    try {
      WaypointDetails::ReadFileFromProfile(way_points, operation);
    } catch (...) {
      LogError(std::current_exception());
    }
  }

  if ((waypoint_files_changed || WaypointFileChanged) &&
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

  if (waypoint_files_changed || WaypointFileChanged) {
    // re-set home
    DataGlobals::UpdateHome(true);
  }

  if (MapFileChanged) {
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
    airspace_database.EraseOrigin(AirspaceOrigin::FILE);
    airspace_database.EraseOrigin(AirspaceOrigin::MAP);
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

  if (ChecklistFileChanged)
    dlgChecklistNotifySiteFileChanged();

  if (InputFileChanged) {
    try {
      InputEvents::readFile();
    } catch (...) {
      LogError(std::current_exception(), "Failed to load input file");
    }
  }

#ifdef HAVE_HTTP
  if (UserRepositoriesListChanged) {
#ifdef HAVE_DOWNLOAD_MANAGER
    DownloadRepositoriesModal(false, true);
#else
    EnqueueRepositoryDownload(true, false, true);
#endif
  }
#endif

  const UISettings &ui_settings = CommonInterface::GetUISettings();

  Units::SetConfig(ui_settings.format.units);
  SetUserCoordinateFormat(ui_settings.format.coordinate_format);

  const MapSettings &old_settings_map = old_ui_settings.map;
  const MapSettings &settings_map = ui_settings.map;

#ifdef __APPLE__
  /* the system appearance is not polled while "Auto" is disabled;
     refresh it now, because it may have changed since */
  UpdateAppleDarkMode();
#endif

  if (ui_settings.dark_mode != old_ui_settings.dark_mode ||
      ui_settings.info_boxes.theme != old_ui_settings.info_boxes.theme) {
    main_window.ReinitialiseLook();
    main_window.ReinitialiseLayout();
  } else if (ui_settings.info_boxes.use_colors != old_ui_settings.info_boxes.use_colors ||
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

static bool
ConfiguredPathMatches(std::string_view key, Path updated) noexcept
{
  const auto configured = Profile::GetPath(key);
  return configured != nullptr && configured == updated;
}

static bool
ConfiguredListMatches(std::string_view key, const char *patterns,
                      Path updated) noexcept
{
  for (const auto &configured : Profile::GetMultiplePaths(key, patterns))
    if (configured != nullptr && configured == updated)
      return true;
  return false;
}

void
ApplyConfiguredDataFiles(std::span<const std::string> relative_paths)
{
  if (!global_running)
    return;

  const char *const waypoint_patterns =
    GetFileTypePatterns(FileType::WAYPOINT);
  const char *const airspace_patterns =
    GetFileTypePatterns(FileType::AIRSPACE);

  bool map = false;
  bool primary = false;
  bool watched = false;
  bool airspace = false;
  bool flarm = false;
  bool rasp = false;
  bool checklist = false;
  bool input = false;

  for (const auto &relative : relative_paths) {
    const auto updated = LocalPath(relative.c_str());
    if (updated == nullptr)
      continue;

    map |= ConfiguredPathMatches(ProfileKeys::MapFile, updated);
    primary |= ConfiguredListMatches(ProfileKeys::WaypointFileList,
                                    waypoint_patterns, updated);
    watched |= ConfiguredListMatches(ProfileKeys::WatchedWaypointFileList,
                                    waypoint_patterns, updated);
    airspace |= ConfiguredListMatches(ProfileKeys::AirspaceFileList,
                                     airspace_patterns, updated);
    flarm |= ConfiguredPathMatches(ProfileKeys::FlarmFile, updated);
    rasp |= ConfiguredPathMatches(ProfileKeys::RaspFile, updated);
    checklist |= ConfiguredPathMatches(ProfileKeys::ChecklistFile, updated);
    input |= ConfiguredPathMatches(ProfileKeys::InputFile, updated);
  }

  if (!map && !primary && !watched && !airspace &&
      !flarm && !rasp && !checklist && !input)
    return;

  /* Same flags Setup sets. A map file still reloads the waypoint
     and airspace rows that live inside the zip. */
  SettingsEnter();
  MapFileChanged = map;
  PrimaryWaypointFileChanged = primary;
  WatchedWaypointFileChanged = watched;
  AirspaceFileChanged = airspace;
  FlarmFileChanged = flarm;
  RaspFileChanged = rasp;
  ChecklistFileChanged = checklist;
  InputFileChanged = input;
  SettingsLeave(CommonInterface::GetUISettings());
}

void
SystemConfiguration()
{
  const UISettings old_ui_settings = CommonInterface::GetUISettings();

  SettingsEnter();
  dlgConfigurationShowModal();
  SettingsLeave(old_ui_settings);
}
