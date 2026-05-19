// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ImportDataPanel.hpp"
#include "StorageLocationPickerDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/JobDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Formatter/FileMetadataFormatter.hpp"
#include "Job/Job.hpp"
#include "Operation/Operation.hpp"
#include "Operation/SubOperationEnvironment.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "system/FileUtil.hpp"
#include "Widget/FileMultiSelectWidget.hpp"
#include "Widget/PropertyWidgetContainer.hpp"
#include "util/StaticString.hxx"
#include "Language/Language.hpp"
#include "LocalPath.hpp"
#include "Compatibility/path.h"
#include "Storage/StorageUtil.hpp"
#include "Storage/StorageDevice.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <stdexcept>
#include <vector>

namespace {

/**
 * Compute the import destination path for a source file.
 * All files are placed directly into the XCSoar data root
 * (no subdirectory structure is preserved).
 */
static AllocatedPath
ComputeDestination(Path src) noexcept
{
  AllocatedPath dest_root = GetPrimaryDataPath();
  return AllocatedPath::Build(dest_root, src.GetBase().c_str());
}

struct ImportFile {
  AllocatedPath path;
  uint64_t size;
};

struct ImportJob : public Job {
  std::vector<ImportFile> files;
  AllocatedPath src_root;
  std::shared_ptr<StorageDevice> device;
  unsigned &completed;
  unsigned &failed;
  bool &cancelled;
  std::vector<std::string> &failed_files;

  ImportJob(std::vector<ImportFile> &&_files, Path root,
            std::shared_ptr<StorageDevice> dev,
            unsigned &c, unsigned &f,
            bool &was_cancelled,
            std::vector<std::string> &failed_list) noexcept
    : files(std::move(_files)),
      src_root(root),
      device(std::move(dev)),
      completed(c), failed(f),
      cancelled(was_cancelled),
      failed_files(failed_list) {}

  void Run(OperationEnvironment &env) override {
    uint64_t total_size = 0;
    for (const auto &f : files)
      total_size += f.size;

    const unsigned total_kb = static_cast<unsigned>(total_size / 1024);
    env.SetProgressRange(total_kb > 0 ? total_kb : 1);
    env.SetProgressPosition(0);

    completed = 0; failed = 0;
    cancelled = false;
    uint64_t bytes_done = 0;

    for (size_t i = 0; i < files.size(); ++i) {
      if (env.IsCancelled()) {
        cancelled = true;
        break;
      }

      if (!IsContentUri(src_root) && !Directory::Exists(src_root)) {
        failed += (unsigned)(files.size() - i);
        break;
      }

      const Path p = files[i].path;

      Path rel = p.RelativeTo(src_root);
      if (rel != nullptr && device->IsDirectoryEntry(rel))
        continue;

      AllocatedPath dest = ComputeDestination(p);

      StaticString<128> status;
      status.Format("%s\n%s", _("Importing"), p.GetBase().c_str());
      env.SetText(status);

      const unsigned start_kb = static_cast<unsigned>(bytes_done / 1024);
      const unsigned end_kb = static_cast<unsigned>((bytes_done + files[i].size) / 1024);
      SubOperationEnvironment sub_env(env,
                                      std::min(start_kb, total_kb),
                                      std::min(end_kb, total_kb));

      try {
        if (rel == nullptr)
          throw std::runtime_error("bad relative path");
        device->CopyToLocal(rel, dest, &sub_env);

        ++completed;
      } catch (...) {
        ++failed;
        if (failed_files.size() < 5) {
          const auto base = p.GetBase();
          failed_files.emplace_back(base != nullptr ? base.c_str() : p.c_str());
        }
      }

      bytes_done += files[i].size;
      env.SetProgressPosition(static_cast<unsigned>(bytes_done / 1024));
    }

    env.SetProgressPosition(total_kb > 0 ? total_kb : 1);
  }
};

/**
 * Container widget that composes a source property field and the
 * FileMultiSelectWidget for browsing an external storage location.
 * It forwards lifecycle methods to the contained list widget.
 *
 * Matches the FlightContainer pattern from ExportFlightsPanel.
 */
struct ImportContainer : public PropertyWidgetContainer {
  std::unique_ptr<FileMultiSelectWidget> file_list;
  AllocatedPath current_path;
  AllocatedPath base_root;
  std::shared_ptr<StorageDevice> device;
  FileMetadataFormatter file_metadata;

  explicit ImportContainer(AllocatedPath initial, AllocatedPath base) noexcept
    : PropertyWidgetContainer(_("Source")),
      current_path(std::move(initial)), base_root(std::move(base)),
      device(FindDeviceByName(base_root)) {
    auto loader = [this]() -> std::vector<FileMultiSelectWidget::FileItem> {
      std::vector<FileMultiSelectWidget::FileItem> out;

      if (Path(current_path) != Path(base_root)) {
        AllocatedPath parent = current_path.GetParent();
        if (parent != nullptr && parent != current_path) {
          FileMultiSelectWidget::FileItem it;
          it.path = std::move(parent);
          it.is_dir = true;
          it.is_up = true;
          out.push_back(std::move(it));
        }
      }

      {
        Path rel = Path(current_path).RelativeTo(base_root);
        std::string rel_str = rel != nullptr ? rel.c_str() : "";

        auto dir_entries = device->ListEntries(Path(rel_str.c_str()));

        for (const auto &entry : dir_entries) {
          std::string full = current_path.c_str();
          full.push_back('/');
          full.append(entry.name);

          FileMultiSelectWidget::FileItem it;
          it.path = AllocatedPath(full.c_str());
          it.is_dir = entry.is_directory;

          out.emplace_back(std::move(it));
        }

        file_metadata.Build(dir_entries, current_path);
      }

      std::sort(out.begin(), out.end(), FileMultiSelectWidget::FileItem::Compare);

      return out;
    };

    file_list = std::make_unique<FileMultiSelectWidget>(loader,
                      nullptr,
                      _("Files"), nullptr);

    file_list->SetSecondLeftProvider([this](const FileMultiSelectWidget::FileItem &it) noexcept {
      if (it.is_dir)
        return (const char *)nullptr;

      return file_metadata.GetSizeText(it.path);
    });
  }

  Widget &GetContentWidget() noexcept override { return *file_list; }
  const Widget &GetContentWidget() const noexcept override { return *file_list; }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    PropertyWidgetContainer::Prepare(parent, rc);
    UpdateSourceCaption();
  }

  void Unprepare() noexcept override {
    PropertyWidgetContainer::Unprepare();
  }

  void UpdateSourceCaption() noexcept {
    if (current_path == nullptr) {
      UpdatePropertyText(_("(none)"));
      return;
    }
    const std::string caption = FormatStorageCaption(current_path);
    UpdatePropertyText(caption.c_str());
  }

  void SetCurrent(AllocatedPath p) noexcept {
    current_path = std::move(p);
    UpdateSourceCaption();
    if (file_list)
      file_list->Refresh();
  }

  void SetSource(AllocatedPath p) noexcept {
    base_root = Path(p);
    device = FindDeviceByName(base_root);
    SetLastStorageTarget(p);
    SetCurrent(std::move(p));
  }

  Path GetSource() const noexcept {
    return base_root;
  }
};

} // anonymous namespace

void
ShowImportDataDialog()
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  // Reuse last target if available and still valid; otherwise prompt.
  AllocatedPath initial_source;
  {
    const AllocatedPath &last = GetLastStorageTarget();
    if (last != nullptr &&
        (IsContentUri(last) || Directory::Exists(last))) {
      initial_source = Path(last);
    } else {
      initial_source = PickStorageLocation();
      if (initial_source == nullptr)
        return;
    }
  }

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(), look,
                      _("Import data - choose files"));

  AllocatedPath base_copy{Path(initial_source)};
  auto container = std::make_unique<ImportContainer>(std::move(initial_source),
                                                     std::move(base_copy));
  ImportContainer &import_container = *container;
  FileMultiSelectWidget &file_list = *import_container.file_list;
  file_list.SetNavigateCallback([&import_container](AllocatedPath d) {
    import_container.SetCurrent(std::move(d));
  });

  dialog.AddButton(_("Choose location"), [&import_container]() {
    PickStorageLocationAndApply([&import_container](AllocatedPath new_src) {
      import_container.SetSource(std::move(new_src));
    });
  });

  dialog.AddButton(_("Select all"), [&file_list]() { file_list.SelectAllFiles(); });
  dialog.AddButton(_("Select none"), [&file_list]() { file_list.ClearSelection(); });
  dialog.AddButton(_("Import"), [&import_container, &file_list]() {
    const auto selected = file_list.GetSelectedPaths();
    if (selected.empty())
      return;

    std::vector<Path> selected_files;
    selected_files.reserve(selected.size());
    bool skipped_directories = false;
    for (const auto &p : selected) {
      if (!IsContentUri(p) && Directory::Exists(p)) {
        skipped_directories = true;
        continue;
      }

      selected_files.push_back(p);
    }

    if (selected_files.empty()) {
      ShowMessageBox(_("Only files can be imported."), _("Import data"),
                     MB_OK | MB_ICONINFORMATION);
      return;
    }

    if (skipped_directories) {
      ShowMessageBox(_("Selected directories are skipped. Only files will be imported."),
                     _("Import data"), MB_OK | MB_ICONINFORMATION);
    }

    // Check for huge files > 250 MB
    const uint64_t HUGE_THRESHOLD = 250ULL * 1024ULL * 1024ULL;
    bool has_huge = false;
    for (const auto &p : selected_files) {
      auto sz = import_container.file_metadata.GetRawSize(p);
      if (sz && *sz > HUGE_THRESHOLD) {
        has_huge = true;
        break;
      }
    }
    if (has_huge) {
      int rr = ShowMessageBox(_("One or more selected files exceed 250 MB. Continue?"),
                              _("Import data"), MB_YESNO);
      if (rr != IDYES)
        return;
    }

    // Per-file overwrite check
    {
      auto it = selected_files.begin();
      while (it != selected_files.end()) {
        AllocatedPath dest = ComputeDestination(*it);
        if (File::Exists(dest)) {
          StaticString<256> prompt;
          const auto base = Path(dest).GetBase();
          prompt.Format(_("'%s' already exists. Overwrite?"),
                        base != nullptr ? base.c_str() : dest.c_str());
          int answer = ShowMessageBox(prompt, _("Import data"), MB_YESNOCANCEL);
          if (answer == IDCANCEL)
            return;
          if (answer != IDYES) {
            it = selected_files.erase(it);
            continue;
          }
        }
        ++it;
      }
    }

    if (selected_files.empty())
      return;

    unsigned completed = 0, failed = 0;
    bool cancelled = false;
    std::vector<std::string> failed_files;

    std::vector<ImportFile> import_files;
    import_files.reserve(selected_files.size());
    for (const auto &p : selected_files) {
      auto sz = import_container.file_metadata.GetRawSize(p);
      import_files.push_back(ImportFile{AllocatedPath(p),
                                        sz.value_or(0)});
    }

    ImportJob job(std::move(import_files), import_container.GetSource(),
                  import_container.device,
                  completed, failed,
                  cancelled, failed_files);
    if (!JobDialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
                   _("Importing"), job, true))
      return;

    if (cancelled || (completed + failed) < selected_files.size()) {
      StaticString<256> msg;
      msg.Format(_("Import canceled. Imported %u file(s)."), completed);
      if (failed > 0)
        msg.AppendFormat(_("\nFailed %u."), failed);
      ShowMessageBox(msg, _("Import data"), MB_OK | MB_ICONINFORMATION);
      return;
    }

    if (failed > 0) {
      StaticString<256> em;
      em.Format(_("Import finished: %u imported, %u failed."), completed, failed);
      for (const auto &name : failed_files)
        em.AppendFormat("\n- %s", name.c_str());
      ShowMessageBox(em, _("Import data"), MB_OK);
    } else {
      StaticString<256> msg;
      msg.Format(_("Import finished: %u imported."), completed);
      ShowMessageBox(msg, _("Import data"), MB_OK);
    }
  });
  dialog.AddButton(_("Back"), mrCancel);

  dialog.FinishPreliminary(std::move(container));
  dialog.ShowModal();
}
