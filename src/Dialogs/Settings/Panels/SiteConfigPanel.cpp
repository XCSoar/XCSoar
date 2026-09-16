// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SiteConfigPanel.hpp"
#include "Dialogs/DialogSettings.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Form/DataField/File.hpp"
#include "Form/DataField/MultiFile.hpp"
#include "Language/Language.hpp"
#include "LocalPath.hpp"
#include "Look/DialogLook.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Repository/FileType.hpp"
#include "Repository/Glue.hpp"
#include "UIGlobals.hpp"
#include "UtilsSettings.hpp"
#include "Widget/GroupedListWidget.hpp"
#include "net/http/Features.hpp"
#include "system/Path.hpp"
#include "util/StaticString.hxx"
#include "util/StringAPI.hxx"

#ifdef HAVE_DOWNLOAD_MANAGER
#include "Dialogs/DownloadFilePicker.hpp"
#endif

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

using SelectionMode = GroupedListWidget::SelectionMode;
using ItemOptions = GroupedListWidget::ItemOptions;
using TextFont = GroupedListWidget::TextFont;
using TextSize = GroupedListWidget::TextSize;

/**
 * Let the user pick one of the files the field knows, or none.  A
 * tap on a file chooses it and closes the view, like the combo
 * picker does.  The explanation of the setting introduces the page.
 */
static void
PickFile(const char *caption, const char *help, FileDataField &df)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, caption);

  auto list = std::make_unique<GroupedListWidget>(look);
  list->AddHero(caption, help);
  list->AddGroup(nullptr, {.selection_mode = SelectionMode::SINGLE});

  const Path value = df.GetValue();

  for (unsigned i = 0, n = df.size(); i < n; ++i) {
    const auto &item = df.GetItem(i);
    const bool checked = item.path == value;
    if (checked)
      list->SetCursorIndex(i);

    /* the first item is the empty one, which stands for no file */
    list->AddItem(item.path.empty() ? _("(none)") : item.filename.c_str(),
                  [&df, &dialog, i](){
      df.SetIndex(i);
      dialog.SetModalResult(mrOK);
    }, {.checked = checked});
  }

#ifdef HAVE_DOWNLOAD_MANAGER
  if (FileTypeSupportsDownload(df.GetFileType()))
    list->AddButton(_("Download"), [&df, &dialog](){
      const auto path = DownloadFilePicker(df.GetFileType());
      if (path == nullptr)
        return;

      df.ForceModify(path);
      dialog.SetModalResult(mrOK);
    });
#endif

  dialog.FinishPreliminary(std::move(list));
  dialog.AddButton(_("Cancel"), mrCancel);
  dialog.ShowModal();
}

/**
 * Let the user check the files the field shall use.  The choice is
 * taken over with OK and dropped with Cancel.
 */
static void
PickFiles(const char *caption, const char *help, MultiFileDataField &df)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, caption);

  auto _list = std::make_unique<GroupedListWidget>(look);
  GroupedListWidget &list = *_list;

  /* the files which the Download button has fetched; they are used
     right away, and dropped again if the view is cancelled */
  std::vector<AllocatedPath> downloaded;

  std::function<void()> fill = [&](){
    list.Clear();
    list.AddHero(caption, help);
    list.AddGroup(nullptr, {.selection_mode = SelectionMode::MULTIPLE});

    const auto selected = df.GetPathFiles();
    for (const Path path : df.GetAllPaths())
      list.AddItem(path.GetBase().c_str(),
                   {.checked = std::find(selected.begin(), selected.end(),
                                         path) != selected.end()});

#ifdef HAVE_DOWNLOAD_MANAGER
    if (FileTypeSupportsDownload(df.GetFileType()))
      list.AddButton(_("Download"), [&](){
        auto path = DownloadFilePicker(df.GetFileType());
        if (path == nullptr)
          return;

        df.ForceModify(path);
        downloaded.emplace_back(std::move(path));

        fill();
        list.UpdateLayout();
      });
#endif
  };

  fill();

  dialog.FinishPreliminary(std::move(_list));
  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);

  if (dialog.ShowModal() == mrOK) {
    const auto paths = df.GetAllPaths();
    for (unsigned i = 0, n = paths.size(); i < n; ++i) {
      if (list.IsItemChecked(i))
        df.AddValue(paths[i]);
      else
        df.UnSet(paths[i]);
    }
  } else
    for (const Path path : downloaded)
      df.UnSet(path);
}

/**
 * The files XCSoar loads, one item per kind, which opens the view
 * where the files are chosen.
 */
class SiteConfigPanel final : public GroupedListWidget {
  FileDataField map_file, flarm_file, rasp_file, checklist_file;

  MultiFileDataField waypoint_files, watched_waypoint_files,
    airfield_files, airspace_files;

  /** the URIs of the user repositories, separated by '|' */
  StaticString<1024> user_repositories;

  /** the user level the list was filled for */
  bool expert;

public:
  SiteConfigPanel() noexcept
    :GroupedListWidget(UIGlobals::GetDialogLook()) {}

private:
  void AddFileItem(const char *caption, const char *help,
                   FileDataField &df) noexcept;

  void AddFilesItem(const char *caption, const char *help,
                    MultiFileDataField &df) noexcept;

  void Fill() noexcept;

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
SiteConfigPanel::AddFileItem(const char *caption, const char *help,
                             FileDataField &df) noexcept
{
  /* the file as a small value, with every line of it */
  ItemOptions options{.value_size = TextSize::SMALL,
                      .value_all_lines = true,
                      .chevron = true};

  const char *name = df.GetAsDisplayString();
  if (*name != '\0')
    options.value = name;
  else
    options.badge = C_("Badge", "none");

  AddItem(caption, [this, caption, help, &df](){
    PickFile(caption, help, df);
    Fill();
    UpdateLayout();
  }, options);
}

void
SiteConfigPanel::AddFilesItem(const char *caption, const char *help,
                              MultiFileDataField &df) noexcept
{
  /* one file per line below the caption */
  std::string names;
  for (const Path path : df.GetPathFiles()) {
    if (!names.empty())
      names.push_back('\n');

    const Path base = path.GetBase();
    names += (base != nullptr ? base : path).c_str();
  }

  ItemOptions options{.value_size = TextSize::SMALL,
                      .value_all_lines = true,
                      .chevron = true};
  if (!names.empty())
    options.value = names.c_str();
  else
    options.badge = C_("Badge", "none");

  AddItem(caption, [this, caption, help, &df](){
    PickFiles(caption, help, df);
    Fill();
    UpdateLayout();
  }, options);
}

void
SiteConfigPanel::Fill() noexcept
{
  Clear();

  expert = UIGlobals::GetDialogSettings().expert;

  AddGroup();
  AddItem(_("XCSoar data path"),
          {.value = GetPrimaryDataPath().c_str(),
           .value_below = true,
           .value_font = TextFont::MONO,
           .value_size = TextSize::SMALL,
           .value_all_lines = true});

  AddGroup();

  AddFileItem(_("Map database"),
              _("The name of the file (.xcm) containing terrain, topography, and optionally "
                "waypoints, their details and airspaces."),
              map_file);

  AddFilesItem(_("Waypoints"),
               _("Primary waypoints files.  Supported file types are "
                 "Cambridge/WinPilot files (.dat), "
                 "Zander files (.wpz) or SeeYou files (.cup, .cupx)."),
               waypoint_files);

  if (expert) {
    AddFilesItem(_("Watched WPTs"),
                 _("Waypoint files containing special waypoints for which "
                   "additional computations like "
                   "calculation of arrival height in map display always "
                   "takes place. Useful for "
                   "waypoints like known reliable thermal sources (e.g. "
                   "powerplants) or mountain passes."),
                 watched_waypoint_files);

    AddFilesItem(_("WPT A/F details"),
                 _("The files may contain extracts from enroute supplements "
                   "or other contributed "
                   "information about individual waypoints and airfields."),
                 airfield_files);
  }

  AddFilesItem(_("Airspace"),
               _("List of active airspace files. Use the Add and Remove "
                 "buttons to activate or deactivate"
                 " airspace files respectively. Supported file types are: "
                 "Openair (.openair /.txt /.air), and Tim Newport-Pearce (.sua)."),
               airspace_files);

  AddFileItem(_("FLARM database"),
              _("The name of the file containing information about registered FLARM devices."),
              flarm_file);

  AddFileItem("RASP",
              _("Regional Atmospheric Soaring Prediction file providing "
                "weather forecasts for soaring. Displays color-coded map "
                "overlays for thermal strength, boundary layer winds, "
                "cloud cover, and other soaring-relevant parameters at "
                "various forecast times throughout the day."),
              rasp_file);

  AddFileItem(_("Checklist"),
              _("The checklist file containing pre-flight and other checklists."),
              checklist_file);

  if (expert) {
    /* one URI per line below the caption */
    std::string uris = user_repositories.c_str();
    std::replace(uris.begin(), uris.end(), '|', '\n');

    ItemOptions options{
      .value_size = TextSize::SMALL,
      .value_all_lines = true,
      .chevron = true,
      .help = _("List of additional user repository URIs, separated by '|' character."),
    };

    if (!uris.empty())
      options.value = uris.c_str();
    else
      options.badge = C_("Badge", "none");

    AddGroup();
    AddItem(_("User repositories"), [this](){
      if (!TextEntryDialog(user_repositories, _("User repositories")))
        return;

      Fill();
      UpdateLayout();
    }, options);
  }
}

/**
 * Let the field know the files of its kind, and which one the
 * profile names.
 */
static void
LoadFile(FileDataField &df, FileType type, std::string_view key) noexcept
{
  df.SetFileType(type);
  df.AddNull();
  df.ScanMultiplePatterns(GetFileTypePatterns(type));
  df.Sort(FileDataField::SortOrder::ASCENDING, true);

  const auto path = Profile::GetPath(key);
  if (path != nullptr)
    df.SetValue(path);
}

static void
LoadFiles(MultiFileDataField &df, FileType type,
          std::string_view key) noexcept
{
  const char *patterns = GetFileTypePatterns(type);

  df.SetFileType(type);
  df.ScanMultiplePatterns(patterns);
  df.GetFileDataField().Sort();

  for (const auto &path : Profile::GetMultiplePaths(key, patterns))
    df.AddInitialPath(path);
}

void
SiteConfigPanel::Prepare(ContainerWindow &parent,
                         const PixelRect &rc) noexcept
{
  LoadFile(map_file, FileType::MAP, ProfileKeys::MapFile);
  LoadFiles(waypoint_files, FileType::WAYPOINT,
            ProfileKeys::WaypointFileList);
  LoadFiles(watched_waypoint_files, FileType::WAYPOINT,
            ProfileKeys::WatchedWaypointFileList);
  LoadFiles(airfield_files, FileType::WAYPOINTDETAILS,
            ProfileKeys::AirfieldFileList);
  LoadFiles(airspace_files, FileType::AIRSPACE,
            ProfileKeys::AirspaceFileList);
  LoadFile(flarm_file, FileType::FLARMNET, ProfileKeys::FlarmFile);
  LoadFile(rasp_file, FileType::RASP, ProfileKeys::RaspFile);
  LoadFile(checklist_file, FileType::CHECKLIST, ProfileKeys::ChecklistFile);

  user_repositories = Profile::Get(ProfileKeys::UserRepositoriesList, "");

  Fill();

  GroupedListWidget::Prepare(parent, rc);
}

void
SiteConfigPanel::Show(const PixelRect &rc) noexcept
{
  /* the user level may have changed on the menu since the list was
     filled */
  if (expert != UIGlobals::GetDialogSettings().expert) {
    Fill();
    UpdateLayout();
  }

  GroupedListWidget::Show(rc);
}

bool
SiteConfigPanel::Save(bool &_changed) noexcept
{
  MapFileChanged = Profile::SetPath(ProfileKeys::MapFile, map_file.GetValue());

  // WaypointFileChanged has already a meaningful value
  WaypointFileChanged |=
    Profile::SetMultiplePaths(ProfileKeys::WaypointFileList,
                              waypoint_files.GetPathFiles());
  WaypointFileChanged |=
    Profile::SetMultiplePaths(ProfileKeys::WatchedWaypointFileList,
                              watched_waypoint_files.GetPathFiles());

  AirspaceFileChanged |=
    Profile::SetMultiplePaths(ProfileKeys::AirspaceFileList,
                              airspace_files.GetPathFiles());

  FlarmFileChanged = Profile::SetPath(ProfileKeys::FlarmFile,
                                      flarm_file.GetValue());

  AirfieldFileChanged =
    Profile::SetMultiplePaths(ProfileKeys::AirfieldFileList,
                              airfield_files.GetPathFiles());

  RaspFileChanged = Profile::SetPath(ProfileKeys::RaspFile,
                                     rasp_file.GetValue());

  ChecklistFileChanged = Profile::SetPath(ProfileKeys::ChecklistFile,
                                          checklist_file.GetValue());

  const std::string old_repositories{
    Profile::Get(ProfileKeys::UserRepositoriesList, "")};
  UserRepositoriesListChanged =
    !StringIsEqual(old_repositories.c_str(), user_repositories.c_str());
  if (UserRepositoriesListChanged) {
    Profile::Set(ProfileKeys::UserRepositoriesList, user_repositories.c_str());
    PurgeChangedUserRepositoryFiles(old_repositories.c_str(),
                                    user_repositories.c_str());
  }

  _changed |= WaypointFileChanged || AirfieldFileChanged ||
    AirspaceFileChanged || MapFileChanged || FlarmFileChanged ||
    RaspFileChanged || ChecklistFileChanged ||
    UserRepositoriesListChanged;

  return true;
}

std::unique_ptr<Widget>
CreateSiteConfigPanel()
{
  return std::make_unique<SiteConfigPanel>();
}
