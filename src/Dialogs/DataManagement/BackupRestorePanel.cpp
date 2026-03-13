// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BackupRestorePanel.hpp"
#include "StorageLocationPickerDialog.hpp"
#include "Formatter/FileMetadataFormatter.hpp"
#include "io/TarBackup.hpp"
#include "io/OutputStream.hxx"
#include "io/Reader.hxx"
#include "Dialogs/Message.hpp"
#include "Dialogs/JobDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Job/Job.hpp"
#include "Operation/Operation.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Widget/PropertyWidgetContainer.hpp"
#include "Widget/ListWidget.hpp"
#include "util/StaticString.hxx"
#include "Language/Language.hpp"
#include "LocalPath.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "time/BrokenDateTime.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Form/DataField/ComboList.hpp"
#include "fmt/format.h"
#include "Storage/StorageUtil.hpp"
#include "Storage/StorageDevice.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

// Patterns of files/directories to exclude from backup/restore operations
static constexpr std::string_view kExcludedPaths[] = {
  "*.log",
  "*.tar",
  "repository",
  "cache/",
};

static bool
IsExcludedPath(std::string_view path) noexcept
{
  for (const auto pattern : kExcludedPaths)
    if (pattern.starts_with("*.")) {
      // extension glob: matches any path ending with the suffix
      if (path.size() >= pattern.size() - 1 &&
          path.ends_with(pattern.substr(1)))
        return true;
    } else if (pattern.ends_with('/')) {
      // directory: matches the name itself and all children
      const auto dir = pattern.substr(0, pattern.size() - 1);
      if (path == dir || path.starts_with(pattern))
        return true;
    } else {
      // exact match
      if (path == pattern)
        return true;
    }
  return false;
}

// Backup job: creates a tarball of primary data path
struct BackupJob final : public Job {
  AllocatedPath target_device;
  std::string tar_name;
  unsigned &created_files;
  bool aborted{false};
  std::string error_message;

  BackupJob(Path target, const std::string &name, unsigned &c) noexcept
    : target_device(target), tar_name(name), created_files(c) {}

  void Run(OperationEnvironment &env) override {
    env.SetText(_("Creating backup"));

    AllocatedPath primary = GetPrimaryDataPath();
    if (primary == nullptr) {
      aborted = true;
      error_message = _("No primary data path.");
      return;
    }

    auto dev = FindDeviceByName(target_device);
    if (!dev) {
      aborted = true;
      error_message = _("Target device not found.");
      return;
    }

    try {
      auto writer = dev->OpenWrite(Path(tar_name.c_str()), true);

      if (!CreateBackup(primary, *writer, IsExcludedPath, env,
                        created_files, error_message)) {
        aborted = true;
        return;
      }

      writer->Commit();
    } catch (const std::exception &e) {
      aborted = true;
      error_message = e.what();
    }
  }
};

// Restore job: extracts selected zip into primary data path (excludes patterns)
struct RestoreJob final : public Job {
  AllocatedPath device_path;
  AllocatedPath tar_name;
  unsigned &restored_files;
  unsigned &failed_files;
  bool aborted{false};
  std::string error_message;

  RestoreJob(Path device, Path name, unsigned &r, unsigned &f) noexcept
    : device_path(device), tar_name(name),
      restored_files(r), failed_files(f) {}

  void Run(OperationEnvironment &env) override {
    env.SetText(_("Restoring backup"));

    AllocatedPath primary = GetPrimaryDataPath();
    if (primary == nullptr) {
      aborted = true;
      error_message = _("No primary data path.");
      return;
    }

    auto dev = FindDeviceByName(device_path);
    if (!dev) {
      aborted = true;
      error_message = _("Source device not found.");
      return;
    }

    try {
      auto reader = dev->OpenRead(tar_name);

      if (!RestoreBackup(*reader, primary, IsExcludedPath, env,
                         restored_files, failed_files, error_message))
        aborted = true;
    } catch (const std::exception &e) {
      aborted = true;
      error_message = e.what();
    }
  }
};

static void
RunRestoreJob(Path device_root, Path tar_name)
{
  int rr = ShowMessageBox(_("Restoring will overwrite existing data. Continue?"),
                          _("Restore"), MB_YESNO);
  if (rr != IDYES)
    return;

  unsigned restored = 0, failed = 0;
  RestoreJob job(device_root, tar_name, restored, failed);
  if (!JobDialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
                 _("Restoring backup"), job, true))
    return;

  if (job.aborted) {
    const std::string fullmsg = std::string(_("Restore failed.")) +
      "\n\n" + job.error_message;
    ShowMessageBox(fullmsg.c_str(), _("Restore"), MB_OK | MB_ICONERROR);
  } else if (failed > 0) {
    StaticString<256> msg;
    msg.Format(_("Restore finished with %u failed files. Restart XCSoar to apply restored settings."), failed);
    ShowMessageBox(msg, _("Restore"), MB_OK | MB_ICONINFORMATION);
  } else {
    ShowMessageBox(_("Restore complete. Restart XCSoar to apply restored settings."),
                   _("Restore"), MB_OK | MB_ICONINFORMATION);
  }
}

struct BackupContainer : public PropertyWidgetContainer {
  struct BackupItem {
    AllocatedPath path;
    std::string display_name;
    std::string size;
    std::string date;
  };

  struct BackupListWidget final : public ListWidget {
    const std::vector<BackupItem> &items;
    TwoTextRowsRenderer row_renderer;

    explicit BackupListWidget(const std::vector<BackupItem> &_items) noexcept
      : items(_items) {}

    void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
      ListWidget::Prepare(parent, rc);

      const DialogLook &look = UIGlobals::GetDialogLook();
      CreateList(parent, look, rc,
                 row_renderer.CalculateLayout(*look.list.font_bold,
                                              look.small_font));
    }

    void RefreshItems() noexcept {
      auto &list = GetList();
      const unsigned old = list.GetCursorIndex();
      list.SetLength(items.size());
      if (items.empty())
        return;

      list.SetCursorIndex(old < items.size() ? old : 0);
    }

    unsigned GetSelectedIndex() const noexcept {
      return GetList().GetCursorIndex();
    }

    unsigned OnListResized() noexcept override {
      const DialogLook &look = UIGlobals::GetDialogLook();
      return row_renderer.CalculateLayout(*look.list.font_bold,
                                          look.small_font);
    }

    void OnPaintItem(Canvas &canvas, const PixelRect rc,
                     unsigned idx) noexcept override {
      if (idx >= items.size())
        return;

      const auto &item = items[idx];
      row_renderer.DrawFirstRow(canvas, rc, item.display_name.c_str());
      row_renderer.DrawRightSecondRow(canvas, rc, item.size.c_str());
      row_renderer.DrawSecondRow(canvas, rc, item.date.c_str());
    }
  };

  std::unique_ptr<BackupListWidget> backup_list;
  AllocatedPath target_device_path;
  std::vector<BackupItem> available_backups;

  BackupContainer()
    : PropertyWidgetContainer(_("Target")),
      backup_list(std::make_unique<BackupListWidget>(available_backups))
  {}

  Widget &GetContentWidget() noexcept override { return *backup_list; }
  const Widget &GetContentWidget() const noexcept override { return *backup_list; }

  void SetTarget(AllocatedPath path) noexcept {
    target_device_path = std::move(path);
    UpdateTargetCaption();
    RefreshBackups();
  }

  void RefreshBackups() noexcept {
    available_backups.clear();

    try {
    if (target_device_path != nullptr) {
      for (const auto &entry : EnumerateTarFiles(target_device_path)) {
        BackupItem item;

        item.path = AllocatedPath(entry.name.c_str());
        item.display_name = entry.name;

        auto meta = FileMetadataFormatter::FormatEntry(entry);
        item.size = meta.size.c_str();
        item.date = meta.last_modified.c_str();

        available_backups.emplace_back(std::move(item));
      }
    }
    } catch (...) {
    }

    std::sort(available_backups.begin(), available_backups.end(),
              [](const auto &a, const auto &b) {
                return std::string_view{a.path.GetBase().c_str()} >
                       std::string_view{b.path.GetBase().c_str()};
              });

    if (backup_list)
      backup_list->RefreshItems();
  }

  bool GetSelectedBackup(AllocatedPath &out) const noexcept {
    if (backup_list == nullptr || available_backups.empty())
      return false;

    const unsigned selected = backup_list->GetSelectedIndex();
    if (selected >= available_backups.size())
      return false;

    out = Path(available_backups[selected].path);
    return out != nullptr;
  }

  void RemoveSelectedBackup() noexcept {
    if (backup_list == nullptr || available_backups.empty())
      return;

    const unsigned selected = backup_list->GetSelectedIndex();
    if (selected >= available_backups.size())
      return;

    available_backups.erase(available_backups.begin() + selected);
    backup_list->RefreshItems();
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    PropertyWidgetContainer::Prepare(parent, rc);
    UpdateTargetCaption();
    RefreshBackups();
  }

  void Unprepare() noexcept override {
    PropertyWidgetContainer::Unprepare();
    available_backups.clear();
  }

  void UpdateTargetCaption() noexcept {
    if (target_device_path == nullptr) {
      UpdatePropertyText(_("(none)"));
      return;
    }
    const std::string caption = FormatStorageCaption(target_device_path);
    UpdatePropertyText(caption.c_str());
  }
};

static void
ShowBackupManagerDialogWithTarget(const AllocatedPath &initial_target)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(), look,
                      _("Backup manager"));
  auto container = std::make_unique<BackupContainer>();
  BackupContainer *container_ptr = container.get();

  // Set path only; Prepare() handles caption and list refresh.
  {
    if (initial_target != nullptr) {
      container_ptr->target_device_path = Path(initial_target);
    } else {
      const AllocatedPath &last = GetLastStorageTarget();
      if (last != nullptr)
        container_ptr->target_device_path = Path(last);
    }
  }

  dialog.AddButton(_("Choose location"), [container_ptr]() {
    PickStorageLocationAndApply([container_ptr](AllocatedPath chosen) {
      container_ptr->SetTarget(std::move(chosen));
    });
  });

  dialog.AddButton(_("Create backup"), [container_ptr]() {
    if (container_ptr->target_device_path == nullptr) {
      ShowMessageBox(_("Select a target first."), _("Create backup"), MB_OK | MB_ICONERROR);
      return;
    }

    // Auto-generate filename using date + time (seconds precision).
    const auto now = BrokenDateTime::NowLocal();
    const std::string filename = fmt::format(
      "xcsoar_backup_{:04d}{:02d}{:02d}_{:02d}{:02d}{:02d}.tar",
      now.year, now.month, now.day,
      now.hour, now.minute, now.second);

    unsigned created = 0;
    BackupJob job(container_ptr->target_device_path, filename, created);
    if (!JobDialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(), _("Creating backup"), job, true))
      return;

    if (job.aborted) {
      const std::string fullmsg = std::string(_("Backup failed.")) +
        "\n\n" + job.error_message;
      ShowMessageBox(fullmsg.c_str(), _("Create backup"), MB_OK | MB_ICONERROR);
    } else {
      container_ptr->RefreshBackups();
    }
  });

  dialog.AddButton(_("Restore backup"), [container_ptr]() {
    if (container_ptr->target_device_path == nullptr) {
      ShowMessageBox(_("Select a target first."), _("Restore backup"), MB_OK | MB_ICONERROR);
      return;
    }

    AllocatedPath selected;
    if (!container_ptr->GetSelectedBackup(selected)) {
      ShowMessageBox(_("No backup files found in selected location."), _("Restore backup"),
                     MB_OK | MB_ICONINFORMATION);
      return;
    }

    RunRestoreJob(container_ptr->target_device_path, selected);
  });

  dialog.AddButton(_("Delete backup"), [container_ptr]() {
    if (container_ptr->target_device_path == nullptr) {
      ShowMessageBox(_("Select a target first."), _("Delete backup"), MB_OK | MB_ICONERROR);
      return;
    }

    AllocatedPath selected;
    if (!container_ptr->GetSelectedBackup(selected)) {
      ShowMessageBox(_("No backup files found in selected location."), _("Delete backup"),
                     MB_OK | MB_ICONINFORMATION);
      return;
    }

    if (ShowMessageBox(_("Delete this backup?"),
                       _("Delete backup"), MB_YESNO) != IDYES)
      return;

    auto dev = FindDeviceByName(container_ptr->target_device_path);
    if (!dev) {
      ShowMessageBox(_("Target device not found."), _("Delete backup"), MB_OK | MB_ICONERROR);
      return;
    }

    container_ptr->RemoveSelectedBackup();

    std::thread([dev = std::move(dev),
                 path = AllocatedPath(std::move(selected))]() {
      dev->DeleteEntry(path);
    }).detach();
  });

  dialog.AddButton(_("Back"), mrCancel);

  dialog.FinishPreliminary(std::move(container));
  dialog.ShowModal();
}

static void
ShowRestoreDialogWithSource(const AllocatedPath &initial_source)
{
  AllocatedPath source;
  if (initial_source != nullptr)
    source = Path(initial_source);
  else {
    source = PickStorageLocation();
    if (source == nullptr)
      return;
  }

  // Enumerate tar files in the selected source.
  const auto entries = EnumerateTarFiles(source);
  ComboList list;
  for (const auto &entry : entries) {
    list.Append(entry.name.c_str(), entry.name.c_str());
  }

  if (list.empty()) {
    ShowMessageBox(_("No backup files found in selected location."), _("Restore"), MB_OK | MB_ICONINFORMATION);
    return;
  }

  int idx = ComboPicker(_("Select backup file"), list, nullptr, false);
  if (idx < 0)
    return;

  const ComboList::Item &item = list[idx];

  RunRestoreJob(source, Path(item.string_value.c_str()));
}

void ShowBackupManagerDialog()
{
  AllocatedPath chosen = PickStorageLocation();
  if (chosen == nullptr)
    return;

  ShowBackupManagerDialogWithTarget(chosen);
}

void ShowRestoreDialog()
{
  ShowRestoreDialogWithSource(AllocatedPath());
}
