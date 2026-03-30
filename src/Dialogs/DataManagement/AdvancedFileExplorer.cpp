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
#include "system/FileUtil.hpp"
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

  dialog.AddButton(_("Delete"), [&file_list]() { PerformDelete(file_list); });
  dialog.AddButton(_("Rename"), [&file_list]() { PerformRename(file_list); });
  dialog.AddButton(_("Select all"), [&file_list]() { file_list.SelectAllFiles(); });
  dialog.AddButton(_("Select none"), [&file_list]() { file_list.ClearSelection(); });
  dialog.AddButton(_("Back"), mrCancel);

  dialog.FinishPreliminary(std::move(container));
  dialog.ShowModal();
}
