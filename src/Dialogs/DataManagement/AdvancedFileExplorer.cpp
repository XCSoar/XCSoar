// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AdvancedFileExplorer.hpp"
#include "DataLayoutMigration.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/FileMultiSelectWidget.hpp"
#include "Widget/PropertyWidgetContainer.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Language/Language.hpp"
#include "LocalPath.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Form/Form.hpp"
#include "Profile/Profile.hpp"
#include "Repository/FileType.hpp"
#include "UtilsSettings.hpp"
#include "io/CopyFile.hxx"
#include "io/FileTransaction.hpp"
#include "system/FileUtil.hpp"
#include "Storage/DirEntry.hpp"
#include "Formatter/FileMetadataFormatter.hpp"
#include "util/StaticString.hxx"
#include "util/TruncateString.hpp"

#include <vector>
#include <string>
#include <algorithm>
#include <memory>

using FileMultiSelectWidgetItem = FileMultiSelectWidget::FileItem;

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

      try {
        const Path dir = Path(current_path);
        const auto dir_entries = ListDirEntries(dir);

        for (const auto &de : dir_entries) {
          FileMultiSelectWidgetItem it;
          it.path = AllocatedPath::Build(dir, de.name.c_str());
          it.is_dir = de.is_directory;
          out.emplace_back(std::move(it));
        }

        file_metadata.Build(dir_entries, dir);
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

  [[nodiscard]] Path GetCurrentPath() const noexcept {
    return current_path;
  }

  [[nodiscard]] Path GetBaseRoot() const noexcept {
    return base_root;
  }
};

enum class TransferMode {
  CANCEL,
  MOVE,
  COPY,
};

struct DirectoryPickerContainer final : public PropertyWidgetContainer {
  std::unique_ptr<FileMultiSelectWidget> file_list;
  AllocatedPath current_path;
  AllocatedPath base_root;

  DirectoryPickerContainer(AllocatedPath initial, AllocatedPath root) noexcept
    : PropertyWidgetContainer(_("Destination")),
      current_path(std::move(initial)),
      base_root(std::move(root))
  {
    auto loader = [this]() -> std::vector<FileMultiSelectWidgetItem> {
      std::vector<FileMultiSelectWidgetItem> out;

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

      try {
        const Path dir = Path(current_path);
        for (const auto &de : ListDirEntries(dir)) {
          if (!de.is_directory)
            continue;

          FileMultiSelectWidgetItem it;
          it.path = AllocatedPath::Build(dir, de.name.c_str());
          it.is_dir = true;
          out.emplace_back(std::move(it));
        }
      } catch (...) {
      }

      std::sort(out.begin(), out.end(), FileMultiSelectWidgetItem::Compare);
      return out;
    };

    file_list = std::make_unique<FileMultiSelectWidget>(loader, nullptr,
                                                        _("Folders"), nullptr);
  }

  Widget &GetContentWidget() noexcept override { return *file_list; }
  const Widget &GetContentWidget() const noexcept override { return *file_list; }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    PropertyWidgetContainer::Prepare(parent, rc);
    UpdateLocationCaption();
  }

  void UpdateLocationCaption() noexcept {
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

  [[nodiscard]] Path GetCurrentPath() const noexcept {
    return current_path;
  }
};

struct TransferDestination {
  TransferMode mode = TransferMode::CANCEL;
  AllocatedPath destination;
};

[[gnu::pure]]
static bool
IsPathInsideRoot(Path root, Path path) noexcept
{
  return path != nullptr &&
    (path == root || path.RelativeTo(root) != nullptr);
}

static TransferDestination
PickTransferDestination(Path base_root, Path initial_path)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(), look,
                      _("Transfer files"));

  auto container = std::make_unique<DirectoryPickerContainer>(
    AllocatedPath(initial_path), AllocatedPath(base_root));
  DirectoryPickerContainer &picker = *container;
  picker.file_list->SetNavigateCallback([&picker](AllocatedPath d) {
    picker.SetCurrent(std::move(d));
  });

  static constexpr int mrMove = 101;
  static constexpr int mrCopy = 102;

  dialog.AddButton(_("Create folder"), [&picker]() {
    char name[256] = "";
    if (!TextEntryDialog(name, sizeof(name), _("Create folder")))
      return;

    if (name[0] == '\0')
      return;

    const Path base_name(name);
    if (!base_name.IsValidFilename()) {
      ShowMessageBox(_("Invalid folder name."), _("Create folder"),
                     MB_OK | MB_ICONERROR);
      return;
    }

    auto destination = AllocatedPath::Build(picker.GetCurrentPath(), base_name);
    if (destination == nullptr || Path(destination).HasPathTraversal()) {
      ShowMessageBox(_("Invalid folder name."), _("Create folder"),
                     MB_OK | MB_ICONERROR);
      return;
    }

    if (File::ExistsAny(destination)) {
      ShowMessageBox(_("Folder already exists."), _("Create folder"),
                     MB_OK | MB_ICONINFORMATION);
      return;
    }

    Directory::Create(destination);
    if (!Directory::Exists(destination)) {
      ShowMessageBox(_("Failed to create folder."), _("Create folder"),
                     MB_OK | MB_ICONERROR);
      return;
    }

    picker.SetCurrent(std::move(destination));
  });
  dialog.AddButton(_("Move"), mrMove);
  dialog.AddButton(_("Copy"), mrCopy);
  dialog.AddButton(_("Cancel"), mrCancel);
  dialog.FinishPreliminary(std::move(container));

  const int result = dialog.ShowModal();

  TransferDestination out;
  if (result == mrMove)
    out.mode = TransferMode::MOVE;
  else if (result == mrCopy)
    out.mode = TransferMode::COPY;
  else
    return out;

  out.destination = picker.GetCurrentPath();
  return out;
}

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

static bool
PromptOverwrite(Path source, Path destination, const char *caption) noexcept
{
  Path source_base = source.GetBase();
  Path destination_base = destination.GetBase();

  StaticString<256> prompt;
  prompt.Format(_("Overwrite '%s' with '%s'?"),
                destination_base != nullptr ? destination_base.c_str() : destination.c_str(),
                source_base != nullptr ? source_base.c_str() : source.c_str());
  return ShowMessageBox(prompt, caption, MB_YESNO | MB_ICONQUESTION) == IDYES;
}

static void
PerformTransferTo(FileMultiSelectWidget &file_list, Path base_root, Path initial_path)
{
  const auto selected = file_list.GetSelectedPaths();
  if (selected.empty()) {
    ShowMessageBox(_("Select at least one file."), _("Transfer to"),
                   MB_OK | MB_ICONINFORMATION);
    return;
  }

  const auto choice = PickTransferDestination(base_root, initial_path);
  if (choice.mode == TransferMode::CANCEL || choice.destination == nullptr)
    return;

  if (!Directory::Exists(choice.destination) ||
      !IsPathInsideRoot(base_root, choice.destination)) {
    ShowMessageBox(_("Invalid destination folder."), _("Transfer to"),
                   MB_OK | MB_ICONERROR);
    return;
  }

  const char *caption = choice.mode == TransferMode::MOVE
    ? _("Move files")
    : _("Copy files");

  unsigned transferred = 0, skipped = 0, failed = 0, profile_updates = 0;

  for (const auto &source_path : selected) {
    if (Directory::Exists(source_path)) {
      ++skipped;
      continue;
    }

    const auto base = source_path.GetBase();
    if (base == nullptr) {
      ++skipped;
      continue;
    }

    const auto destination_path = AllocatedPath::Build(choice.destination, base);
    if (destination_path == nullptr || destination_path == source_path) {
      ++skipped;
      continue;
    }

    if (!IsPathInsideRoot(base_root, destination_path) ||
        Path(destination_path).HasPathTraversal()) {
      ++skipped;
      continue;
    }

    if (File::ExistsAny(destination_path) &&
        !PromptOverwrite(source_path, destination_path, caption)) {
      ++skipped;
      continue;
    }

    bool success = false;
    if (choice.mode == TransferMode::COPY) {
      try {
        FileTransaction transaction(destination_path);
        CopyFile(source_path, transaction.GetTemporaryPath());
        transaction.Commit();
        success = true;
      } catch (...) {
        success = false;
      }
    } else {
      if (File::ExistsAny(destination_path))
        success = File::Replace(source_path, destination_path);
      else
        success = File::Rename(source_path, destination_path);
    }

    if (!success) {
      ++failed;
      continue;
    }

    ++transferred;

    if (choice.mode == TransferMode::MOVE &&
        UpdateProfileReferences(source_path, destination_path))
      ++profile_updates;
  }

  if (profile_updates > 0)
    Profile::Save();

  StaticString<256> msg;
  msg.Format(choice.mode == TransferMode::MOVE
             ? _("Moved %u file(s). Skipped %u. Failed %u.")
             : _("Copied %u file(s). Skipped %u. Failed %u."),
             transferred, skipped, failed);
  if (profile_updates > 0)
    msg.append(_("\nRestart recommended for moved configured files."));

  ShowMessageBox(msg, caption,
                 failed > 0 ? MB_OK | MB_ICONERROR : MB_OK | MB_ICONINFORMATION);
  file_list.Refresh();
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
    profile_updates += UpdateProfileReferences(entry.source_path,
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
  dialog.AddButton(_("Transfer to"),
                   [&file_list, &explorer]() {
                     PerformTransferTo(file_list, explorer.GetBaseRoot(),
                                       explorer.GetCurrentPath());
                   });
  dialog.AddButton(_("Delete"), [&file_list]() { PerformDelete(file_list); });
  dialog.AddButton(_("Rename"), [&file_list]() { PerformRename(file_list); });
  dialog.AddButton(_("Select all"), [&file_list]() { file_list.SelectAllFiles(); });
  dialog.AddButton(_("Select none"), [&file_list]() { file_list.ClearSelection(); });
  dialog.AddButton(_("Back"), mrCancel);

  dialog.FinishPreliminary(std::move(container));
  dialog.ShowModal();
}
