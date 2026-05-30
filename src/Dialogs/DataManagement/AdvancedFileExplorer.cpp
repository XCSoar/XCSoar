// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AdvancedFileExplorer.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/FileMultiSelectWidget.hpp"
#include "Widget/PropertyWidgetContainer.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Language/Language.hpp"
#include "LocalPath.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Profile/Profile.hpp"
#include "Repository/FileType.hpp"
#include "UtilsSettings.hpp"
#include "system/FileUtil.hpp"
#include "Formatter/FileMetadataFormatter.hpp"
#include "util/IterableSplitString.hxx"
#include "util/StaticString.hxx"
#include "util/TruncateString.hpp"

#include <vector>
#include <string>
#include <algorithm>
#include <memory>

using FileMultiSelectWidgetItem = FileMultiSelectWidget::FileItem;

struct MigrationEntry {
  AllocatedPath source_path;
  AllocatedPath destination_path;
  FileType type = FileType::UNKNOWN;

  MigrationEntry(AllocatedPath &&_source_path,
                 AllocatedPath &&_destination_path,
                 FileType _type) noexcept
    : source_path(std::move(_source_path)),
      destination_path(std::move(_destination_path)),
      type(_type) {}
};

struct MigrationPlan {
  std::vector<MigrationEntry> moves;
  unsigned skipped_unknown = 0;
  unsigned skipped_conflicts = 0;
};

struct AdvancedExplorerContainer : public PropertyWidgetContainer {
  std::unique_ptr<FileMultiSelectWidget> file_list;
  AllocatedPath current_path;
  AllocatedPath base_root;
  FileMetadataFormatter file_metadata;

  explicit AdvancedExplorerContainer(AllocatedPath initial) noexcept
    : PropertyWidgetContainer(_("Location")),
      current_path(std::move(initial)),
      base_root(Path(current_path))
  {
    auto loader = [this]() -> std::vector<FileMultiSelectWidgetItem> {
      std::vector<FileMultiSelectWidgetItem> out;

      // Add parent entry when not at the base root
      if (Path(current_path) != Path(base_root)) {
        AllocatedPath parent = current_path.GetParent();
        if (parent != nullptr && parent != current_path) {
          FileMultiSelectWidgetItem it;
          it.path = std::move(parent);
          it.is_dir = true;
          it.is_up = true;
          out.emplace_back(std::move(it));
        }
      }

      // Enumerate local filesystem
      struct Visitor final : public Directory::DirEntryVisitor {
        std::vector<FileMultiSelectWidgetItem> &out;
        std::vector<Path> files;
        Path current;

        Visitor(std::vector<FileMultiSelectWidgetItem> &o, Path c) noexcept
          : out(o), current(c) {}

        void Visit(Path /*full*/, Path filename, bool is_dir) noexcept override {
          FileMultiSelectWidgetItem it;
          it.path = AllocatedPath::Build(current, filename.c_str());
          it.is_dir = is_dir;
          out.emplace_back(std::move(it));
          if (!is_dir)
            files.emplace_back(out.back().path);
        }
      };

      try {
        Visitor v(out, Path(current_path));
        Directory::VisitDirectoriesAndFiles(Path(current_path), v, false);
        file_metadata.Build(v.files);
      } catch (...) {
      }

      std::sort(out.begin(), out.end(), FileMultiSelectWidgetItem::Compare);

      return out;
    };

    file_list = std::make_unique<FileMultiSelectWidget>(loader, nullptr, _("Files"), nullptr);

    // Show size and last-modified on a second row like other file dialogs
    file_list->SetSecondLeftProvider([this](const FileMultiSelectWidget::FileItem &it) noexcept {
      if (it.is_dir)
        return (const char *)nullptr;
      return file_metadata.GetSizeText(it.path);
    });
    file_list->SetSecondRightProvider([this](const FileMultiSelectWidget::FileItem &it) noexcept {
      if (it.is_dir)
        return (const char *)nullptr;
      return file_metadata.GetLastModifiedText(it.path);
    });
  }

  Widget &GetContentWidget() noexcept override { return *file_list; }
  const Widget &GetContentWidget() const noexcept override { return *file_list; }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    PropertyWidgetContainer::Prepare(parent, rc);
    UpdateLocationCaption();
  }

  void Unprepare() noexcept override {
    PropertyWidgetContainer::Unprepare();
  }

  void UpdateLocationCaption() noexcept {
    if (current_path == nullptr) {
      UpdatePropertyText(_("(none)"));
      return;
    }
    Path rel = Path(current_path).RelativeTo(base_root);
    if (rel != nullptr)
      UpdatePropertyText(rel.c_str());
    else
      UpdatePropertyText("/");
  }

  void SetCurrent(AllocatedPath p) noexcept {
    current_path = std::move(p);
    UpdateLocationCaption();
    if (file_list)
      file_list->Refresh();
  }
};

static void
PerformDelete(FileMultiSelectWidget &file_list)
{
  const auto selected = file_list.GetSelectedPaths();
  if (selected.empty()) {
    ShowMessageBox(_("Select at least one file."), _("Delete"), MB_OK | MB_ICONINFORMATION);
    return;
  }

  StaticString<256> prompt;
  if (selected.size() == 1) {
    const Path p = selected[0];
    Path base = p.GetBase();
    prompt.Format(_("Delete '%s'?"), base != nullptr ? base.c_str() : p.c_str());
  } else {
    prompt.Format(_("Delete %u items?"), (unsigned)selected.size());
  }

  int rr = ShowMessageBox(prompt, _("Delete"), MB_YESNO);
  if (rr != IDYES)
    return;

  unsigned deleted = 0, failed = 0;
  std::vector<std::string> failed_names;

  for (const auto &p : selected) {
    bool ok;
    if (Directory::Exists(p))
      ok = Directory::Remove(p);
    else
      ok = File::Delete(p);

    if (ok) {
      ++deleted;
    } else {
      ++failed;
      if (failed_names.size() < 5) {
        Path base = p.GetBase();
        failed_names.emplace_back(base != nullptr ? base.c_str() : p.c_str());
      }
    }
  }

  if (failed > 0) {
    StaticString<256> msg;
    msg.Format(_("Deleted %u file(s). Failed %u."), deleted, failed);
    for (const auto &n : failed_names)
      msg.AppendFormat("\n- %s", n.c_str());
    ShowMessageBox(msg, _("Delete"), MB_OK | MB_ICONERROR);
  } else {
    StaticString<128> msg;
    msg.Format(_("Deleted %u item(s)."), deleted);
    ShowMessageBox(msg, _("Delete"), MB_OK | MB_ICONINFORMATION);
  }

  file_list.Refresh();
}

static void
PerformRename(FileMultiSelectWidget &file_list)
{
  const auto selected = file_list.GetSelectedPaths();
  if (selected.size() != 1) {
    ShowMessageBox(_("Select a single file to rename."), _("Rename"), MB_OK | MB_ICONINFORMATION);
    return;
  }

  const Path oldp = selected[0];

  char newname[256];
  // Prefill with current base name to avoid uninitialized buffer / invalid UTF-8
  Path old_base = oldp.GetBase();
  if (old_base != nullptr)
    CopyTruncateString(newname, sizeof(newname), old_base.c_str());
  else
    CopyTruncateString(newname, sizeof(newname), oldp.c_str());

  if (!TextEntryDialog(newname, sizeof(newname), _("Rename file")))
    return;

  if (newname[0] == '\0')
    return;

  // disallow path separators in the new name
  if (std::strchr(newname, '/') != nullptr || std::strchr(newname, '\\') != nullptr) {
    ShowMessageBox(_("Invalid name."), _("Rename"), MB_OK | MB_ICONERROR);
    return;
  }

  AllocatedPath parent = Path(oldp).GetParent();
  if (parent == nullptr) {
    ShowMessageBox(_("Cannot determine parent path."), _("Rename"), MB_OK | MB_ICONERROR);
    return;
  }

  AllocatedPath dest_path = AllocatedPath::Build(parent, newname);

  // Prevent path traversal via crafted names
  if (Path(dest_path).HasPathTraversal()) {
    ShowMessageBox(_("Invalid name."), _("Rename"), MB_OK | MB_ICONERROR);
    return;
  }

  if (File::ExistsAny(dest_path)) {
    int rr = ShowMessageBox(_("Destination exists. Overwrite?"), _("Rename"), MB_YESNO);
    if (rr != IDYES)
      return;
    if (!File::Replace(oldp, dest_path)) {
      ShowMessageBox(_("Rename failed."), _("Rename"), MB_OK | MB_ICONERROR);
      return;
    }
  } else {
    if (!File::Rename(oldp, dest_path)) {
      ShowMessageBox(_("Rename failed."), _("Rename"), MB_OK | MB_ICONERROR);
      return;
    }
  }

  file_list.Refresh();
}

[[gnu::pure]]
static AllocatedPath
ResolveMigrationRelativePath(Path source_path) noexcept
{
  const auto base = source_path.GetBase();
  if (base == nullptr)
    return nullptr;

  const Path base_path(base);
  if (!base_path.IsValidFilename())
    return nullptr;

  const auto type = DetectFileTypeByFilename(base.c_str());
  if (type == FileType::UNKNOWN)
    return nullptr;

  const auto subdir = GetFileTypeDefaultDir(type);
  if (subdir == nullptr)
    return nullptr;

  return AllocatedPath::Build(Path(subdir), base_path);
}

static MigrationPlan
BuildMigrationPlan(Path root)
{
  MigrationPlan plan;

  struct Visitor final : Directory::DirEntryVisitor {
    Path root;
    MigrationPlan &plan;

    Visitor(Path _root, MigrationPlan &_plan) noexcept
      : root(_root), plan(_plan) {}

    void Visit(Path full, Path filename, bool is_dir) noexcept override {
      if (is_dir || filename == nullptr)
        return;

      const auto relative_path = ResolveMigrationRelativePath(filename);
      if (relative_path == nullptr) {
        ++plan.skipped_unknown;
        return;
      }

      auto destination_path = LocalPath(relative_path);
      if (File::ExistsAny(destination_path)) {
        ++plan.skipped_conflicts;
        return;
      }

      plan.moves.emplace_back(AllocatedPath(full),
                              std::move(destination_path),
                              DetectFileTypeByFilename(filename.c_str()));
    }
  };

  Visitor visitor(root, plan);
  Directory::VisitDirectoriesAndFiles(root, visitor, false);
  return plan;
}

static bool
UpdateSingleProfilePath(std::string_view key, Path old_path, Path new_path)
{
  if (!Profile::GetPathIsEqual(key, old_path))
    return false;

  Profile::SetPath(key, new_path);
  return true;
}

static bool
UpdateMultipleProfilePaths(std::string_view key, Path old_path, Path new_path)
{
  std::string value;
  if (!Profile::Get(key, value) || value.empty())
    return false;

  const auto contracted_new_path = ContractLocalPath(new_path);
  const char *new_value = contracted_new_path != nullptr
    ? contracted_new_path.c_str()
    : new_path.c_str();

  bool changed = false;
  std::string updated_value;
  for (const auto part : TIterableSplitString(value.c_str(), '|')) {
    if (!updated_value.empty())
      updated_value.push_back('|');

    const std::string current(part);
    const auto expanded = ExpandLocalPath(Path(current.c_str()));
    if (expanded != nullptr && expanded == old_path) {
      updated_value.append(new_value);
      changed = true;
    } else {
      updated_value.append(current);
    }
  }

  if (changed)
    Profile::Set(key, updated_value);

  return changed;
}

static bool
UpdateProfileReferences(FileType type, Path old_path, Path new_path)
{
  bool changed = false;

  switch (type) {
  case FileType::MAP:
    changed |= UpdateSingleProfilePath(ProfileKeys::MapFile, old_path, new_path);
    MapFileChanged |= changed;
    break;

  case FileType::WAYPOINT:
    changed |= UpdateMultipleProfilePaths(ProfileKeys::WaypointFileList,
                                          old_path, new_path);
    changed |= UpdateMultipleProfilePaths(ProfileKeys::WatchedWaypointFileList,
                                          old_path, new_path);
    WaypointFileChanged |= changed;
    break;

  case FileType::WAYPOINTDETAILS:
    changed |= UpdateMultipleProfilePaths(ProfileKeys::AirfieldFileList,
                                          old_path, new_path);
    AirfieldFileChanged |= changed;
    break;

  case FileType::AIRSPACE:
    changed |= UpdateMultipleProfilePaths(ProfileKeys::AirspaceFileList,
                                          old_path, new_path);
    AirspaceFileChanged |= changed;
    break;

  case FileType::FLARMNET:
  case FileType::FLARMDB:
    changed |= UpdateSingleProfilePath(ProfileKeys::FlarmFile, old_path, new_path);
    FlarmFileChanged |= changed;
    break;

  case FileType::RASP:
    changed |= UpdateSingleProfilePath(ProfileKeys::RaspFile, old_path, new_path);
    RaspFileChanged |= changed;
    break;

  case FileType::CHECKLIST:
    changed |= UpdateSingleProfilePath(ProfileKeys::ChecklistFile,
                                       old_path, new_path);
    ChecklistFileChanged |= changed;
    break;

  case FileType::XCI:
    changed |= UpdateSingleProfilePath(ProfileKeys::InputFile, old_path, new_path);
    InputFileChanged |= changed;
    break;

  case FileType::PLANE:
    changed |= UpdateSingleProfilePath("PlanePath", old_path, new_path);
    break;

  case FileType::PROFILE:
    if (Profile::GetPath() == old_path) {
      Profile::SetFiles(new_path);
      changed = true;
    }
    break;

  case FileType::UNKNOWN:
  case FileType::IGC:
  case FileType::NMEA:
  case FileType::TASK:
  case FileType::LUA:
  case FileType::COUNT:
    break;
  }

  return changed;
}

static void
PerformOrganizeFiles(FileMultiSelectWidget &file_list)
{
  const auto root = GetPrimaryDataPath();
  const auto plan = BuildMigrationPlan(root);

  if (plan.moves.empty()) {
    StaticString<256> msg;
    msg.Format(_("No files to organize.\nSkipped unknown: %u\nSkipped conflicts: %u"),
               plan.skipped_unknown, plan.skipped_conflicts);
    ShowMessageBox(msg, _("Organize files"), MB_OK | MB_ICONINFORMATION);
    return;
  }

  StaticString<256> prompt;
  prompt.Format(_("Organize %u file(s)?\nSkipped unknown: %u\nSkipped conflicts: %u"),
                (unsigned)plan.moves.size(),
                plan.skipped_unknown,
                plan.skipped_conflicts);
  if (ShowMessageBox(prompt, _("Organize files"), MB_YESNO) != IDYES)
    return;

  unsigned moved = 0, failed = 0, profile_updates = 0;
  for (const auto &entry : plan.moves) {
    const auto parent = entry.destination_path.GetParent();
    if (parent != nullptr)
      Directory::CreateRecursive(parent);

    if (!File::Rename(entry.source_path, entry.destination_path)) {
      ++failed;
      continue;
    }

    ++moved;
    profile_updates += UpdateProfileReferences(entry.type,
                                               entry.source_path,
                                               entry.destination_path)
      ? 1u : 0u;
  }

  if (profile_updates > 0)
    Profile::Save();

  StaticString<256> msg;
  msg.Format(_("Organized %u file(s). Failed %u.\nSkipped unknown: %u\nSkipped conflicts: %u"),
             moved, failed, plan.skipped_unknown, plan.skipped_conflicts);
  if (profile_updates > 0)
    msg.append(_("\nRestart recommended for moved configured files."));

  ShowMessageBox(msg, _("Organize files"), MB_OK | MB_ICONINFORMATION);
  file_list.Refresh();
}

void
ShowAdvancedFileExplorerDialog()
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(), look,
                      _("Advanced File Explorer"));

  AllocatedPath initial = Path(GetPrimaryDataPath());
  auto container = std::make_unique<AdvancedExplorerContainer>(std::move(initial));
  AdvancedExplorerContainer &explorer = *container;
  FileMultiSelectWidget &file_list = *explorer.file_list;

  file_list.SetNavigateCallback([&explorer](AllocatedPath d) {
    explorer.SetCurrent(std::move(d));
  });

  dialog.AddButton(_("Organize files"),
                   [&file_list]() { PerformOrganizeFiles(file_list); });
  dialog.AddButton(_("Delete"), [&file_list]() { PerformDelete(file_list); });
  dialog.AddButton(_("Rename"), [&file_list]() { PerformRename(file_list); });
  dialog.AddButton(_("Select all"), [&file_list]() { file_list.SelectAllFiles(); });
  dialog.AddButton(_("Select none"), [&file_list]() { file_list.ClearSelection(); });
  dialog.AddButton(_("Back"), mrCancel);

  dialog.FinishPreliminary(std::move(container));
  dialog.ShowModal();
}
