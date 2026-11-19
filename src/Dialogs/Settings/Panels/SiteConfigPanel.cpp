// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SiteConfigPanel.hpp"
#include "ConfigPanel.hpp"
#include "Language/Language.hpp"
#include "LocalPath.hpp"
#include "Profile/Keys.hpp"
#include "Repository/FileType.hpp"
#include "Profile/Profile.hpp"
#include "UIGlobals.hpp"
#include "Repository/Glue.hpp"
#include "UtilsSettings.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Enum.hpp"
#include "system/Path.hpp"

#include <cstring>

enum ControlIndex {
  DataPath,
  MapFile,
  TerrainSource,
  WaypointFileList,
  WatchedWaypointFileList,
  AirfieldFileList,
  AirspaceFileList,
  FlarmFile,
  RaspFile,
  ChecklistFile,
  UserRepositoriesList,
#ifdef HAVE_DOWNLOAD_MANAGER
  AutoUpdate,
#endif
};

class SiteConfigPanel final : public RowFormWidget {
  enum Buttons {
    WAYPOINT_EDITOR,
  };

public:
  SiteConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
SiteConfigPanel::Prepare([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept
{
  WndProperty *wp = Add(_("XCSoar data path"), _("Click to view full path"), false);
  wp->SetText(GetPrimaryDataPath().c_str());
  wp->SetReadOnly(true);

  AddFile(_("Map database"),
          _("The name of the file (.xcm) containing terrain, topography, and optionally "
            "waypoints, their details and airspaces."),
          ProfileKeys::MapFile, GetFileTypePatterns(FileType::MAP),
          FileType::MAP);

  static constexpr StaticEnumChoice terrain_source_list[] = {
    { 0, N_("From Map database"),
      N_("Use terrain from the .xcm map database file.") },
    { 1, N_("SRTM GL1 (30m)"),
      N_("Download high-resolution (1 arc-second / ~30m) SRTM terrain tiles "
         "automatically from OpenTopography as needed. Requires internet connection.") },
    { 2, N_("SRTM GL3 (90m)"),
      N_("Download medium-resolution (3 arc-second / ~90m) SRTM terrain tiles "
         "automatically from OpenTopography as needed. Requires internet connection.") },
    nullptr
  };

  unsigned terrain_source = 0;
  const char *terrain_file = Profile::Get(ProfileKeys::TerrainFile);
  if (terrain_file != nullptr && *terrain_file != '\0') {
    if (strcmp(terrain_file, "srtm_gl1") == 0)
      terrain_source = 1;
    else if (strcmp(terrain_file, "srtm_gl3") == 0)
      terrain_source = 2;
  }

  AddEnum(_("Terrain source"),
          _("Choose the source of terrain elevation data. "
            "By default, terrain is loaded from the map database (.xcm file). "
            "Alternatively, terrain tiles can be downloaded automatically from "
            "online sources as you move around the map."),
          terrain_source_list, terrain_source);

  AddMultipleFiles(_("Waypoints"),
                   _("Primary waypoints files.  Supported file types are "
                     "Cambridge/WinPilot files (.dat), "
                     "Zander files (.wpz) or SeeYou files (.cup, .cupx)."),
                   ProfileKeys::WaypointFileList,
                   GetFileTypePatterns(FileType::WAYPOINT),
                   FileType::WAYPOINT);

  AddMultipleFiles(_("Watched WPTs"),
                   _("Waypoint files containing special waypoints for which "
                     "additional computations like "
                     "calculation of arrival height in map display always "
                     "takes place. Useful for "
                     "waypoints like known reliable thermal sources (e.g. "
                     "powerplants) or mountain passes."),
                   ProfileKeys::WatchedWaypointFileList,
                   GetFileTypePatterns(FileType::WAYPOINT),
                   FileType::WAYPOINT);
  SetExpertRow(WatchedWaypointFileList);

  AddMultipleFiles(_("WPT A/F details"),
                   _("The files may contain extracts from enroute supplements "
                     "or other contributed "
                     "information about individual waypoints and airfields."),
                   ProfileKeys::AirfieldFileList,
                   GetFileTypePatterns(FileType::WAYPOINTDETAILS),
                   FileType::WAYPOINTDETAILS);
  SetExpertRow(AirfieldFileList);

  AddMultipleFiles(_("Airspace"),
                   _("List of active airspace files. Use the Add and Remove "
                     "buttons to activate or deactivate"
                     " airspace files respectively. Supported file types are: "
                     "Openair (.openair /.txt /.air), and Tim Newport-Pearce (.sua)."),
                   ProfileKeys::AirspaceFileList,
                   GetFileTypePatterns(FileType::AIRSPACE),
                   FileType::AIRSPACE);

  AddFile(_("FLARM database"),
          _("The name of the file containing information about registered FLARM devices."),
          ProfileKeys::FlarmFile,
          GetFileTypePatterns(FileType::FLARMNET),
          FileType::FLARMNET);

  AddFile("RASP",
          _("Regional Atmospheric Soaring Prediction file providing "
            "weather forecasts for soaring. Displays color-coded map "
            "overlays for thermal strength, boundary layer winds, "
            "cloud cover, and other soaring-relevant parameters at "
            "various forecast times throughout the day."),
          ProfileKeys::RaspFile,
          GetFileTypePatterns(FileType::RASP),
          FileType::RASP);

  AddFile(_("Checklist"),
          _("The checklist file containing pre-flight and other checklists."),
          ProfileKeys::ChecklistFile,
          GetFileTypePatterns(FileType::CHECKLIST),
          FileType::CHECKLIST);
  
  const char *user_repositories_list_value = Profile::Get(ProfileKeys::UserRepositoriesList, "");

  AddText(_("User repositories"),
          _("List of additional user repository URIs, separated by '|' character."),
          user_repositories_list_value);
  SetExpertRow(UserRepositoriesList);

#ifdef HAVE_DOWNLOAD_MANAGER
  bool auto_update = false;
  Profile::Get(ProfileKeys::RepositoryAutoUpdate, auto_update);
  AddBoolean(C_("Setting", "Auto update"),
             _("At startup, download a newer copy of each file already "
               "on this device. The progress bar shows the download."),
             auto_update);
#endif
}

bool
SiteConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  MapFileChanged = SaveValueFileReader(MapFile, ProfileKeys::MapFile);

  unsigned old_terrain_source = 0;
  const char *old_terrain_file = Profile::Get(ProfileKeys::TerrainFile);
  if (old_terrain_file != nullptr && *old_terrain_file != '\0') {
    if (strcmp(old_terrain_file, "srtm_gl1") == 0)
      old_terrain_source = 1;
    else if (strcmp(old_terrain_file, "srtm_gl3") == 0)
      old_terrain_source = 2;
  }

  unsigned terrain_source = old_terrain_source;
  if (SaveValueEnum(TerrainSource, terrain_source) &&
      terrain_source != old_terrain_source) {
    const char *terrain_file = nullptr;
    switch (terrain_source) {
    case 0:
      terrain_file = "";
      break;
    case 1:
      terrain_file = "srtm_gl1";
      break;
    case 2:
      terrain_file = "srtm_gl3";
      break;
    }
    if (terrain_file != nullptr) {
      Profile::Set(ProfileKeys::TerrainFile, terrain_file);
      MapFileChanged = true;
    }
  }

  // WaypointFileChanged has already a meaningful value
  const bool primary_changed = SaveValueMultiFileReader(
      WaypointFileList, ProfileKeys::WaypointFileList);
  const bool watched_changed = SaveValueMultiFileReader(
      WatchedWaypointFileList, ProfileKeys::WatchedWaypointFileList);
  PrimaryWaypointFileChanged |= primary_changed;
  WatchedWaypointFileChanged |= watched_changed;
  WaypointFileChanged |= primary_changed || watched_changed;

  AirspaceFileChanged |= SaveValueMultiFileReader(
      AirspaceFileList, ProfileKeys::AirspaceFileList);

  FlarmFileChanged = SaveValueFileReader(FlarmFile, ProfileKeys::FlarmFile);

  AirfieldFileChanged = SaveValueMultiFileReader(
      AirfieldFileList, ProfileKeys::AirfieldFileList);

  RaspFileChanged = SaveValueFileReader(RaspFile, ProfileKeys::RaspFile);

  ChecklistFileChanged = SaveValueFileReader(ChecklistFile, ProfileKeys::ChecklistFile);

  const std::string old_repos{Profile::Get(ProfileKeys::UserRepositoriesList, "")};
  std::string new_repos = old_repos;
  UserRepositoriesListChanged = SaveValue(
      UserRepositoriesList, ProfileKeys::UserRepositoriesList, new_repos);
  if (UserRepositoriesListChanged)
    PurgeChangedUserRepositoryFiles(old_repos.c_str(), new_repos.c_str());

  changed = WaypointFileChanged || AirfieldFileChanged ||
            AirspaceFileChanged || MapFileChanged || FlarmFileChanged ||
            RaspFileChanged || ChecklistFileChanged ||
            UserRepositoriesListChanged;

#ifdef HAVE_DOWNLOAD_MANAGER
  bool auto_update = false;
  Profile::Get(ProfileKeys::RepositoryAutoUpdate, auto_update);
  changed |= SaveValue(AutoUpdate, ProfileKeys::RepositoryAutoUpdate,
                       auto_update);
#endif

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateSiteConfigPanel()
{
  return std::make_unique<SiteConfigPanel>();
}
