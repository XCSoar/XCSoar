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
#include "system/Path.hpp"

enum ControlIndex {
  DataPath,
  MapFile,
  WaypointFileList,
  WatchedWaypointFileList,
  AirfieldFileList,
  AirspaceFileList,
  FlarmFile,
  FrequenciesFile,
  RaspFile,
  ChecklistFile,
  UserRepositoriesList
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

  AddMultipleFiles(_("Waypoints"),
                   _("Primary waypoints files.  Supported file types are "
                     "Cambridge/WinPilot files (.dat), "
                     "Zander files (.wpz) or SeeYou files (.cup)."),
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

 AddFile(_("Radio Frequency Database"),
          _("The file containing a list of radio frequencies."),
          ProfileKeys::FrequenciesFile, "*.frq\0",
          FileType::FREQUENCIES);

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
}

bool
SiteConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  MapFileChanged = SaveValueFileReader(MapFile, ProfileKeys::MapFile);

  // WaypointFileChanged has already a meaningful value
  WaypointFileChanged |= SaveValueMultiFileReader(
      WaypointFileList, ProfileKeys::WaypointFileList);
  WaypointFileChanged |= SaveValueMultiFileReader(
      WatchedWaypointFileList, ProfileKeys::WatchedWaypointFileList);

  AirspaceFileChanged |= SaveValueMultiFileReader(
      AirspaceFileList, ProfileKeys::AirspaceFileList);

  FlarmFileChanged = SaveValueFileReader(FlarmFile, ProfileKeys::FlarmFile);
  FrequenciesFileChanged = SaveValueFileReader(FrequenciesFile, ProfileKeys::FrequenciesFile);

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
            FrequencyFileChanged||RaspFileChanged || ChecklistFileChanged ||
            UserRepositoriesListChanged;

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateSiteConfigPanel()
{
  return std::make_unique<SiteConfigPanel>();
}
