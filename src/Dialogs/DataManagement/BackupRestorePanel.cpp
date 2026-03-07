// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BackupRestorePanel.hpp"
#include "ArchiveUtil.hpp"
#include "StorageLocationPickerDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/JobDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Job/Job.hpp"
#include "Operation/Operation.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "system/FileUtil.hpp"
#include "Widget/Widget.hpp"
#include "Widget/ListWidget.hpp"
#include "Screen/Layout.hpp"
#include "Form/Edit.hpp"
#include "util/StaticString.hxx"
#include "Language/Language.hpp"
#include "LocalPath.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Formatter/ByteSizeFormatter.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "time/BrokenDateTime.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Form/DataField/ComboList.hpp"
#include "fmt/format.h"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// Patterns of files/directories to exclude from backup/restore operations
static constexpr std::string_view kExcludedPaths[] = {
  "*.log",
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

static std::vector<AllocatedPath>
EnumerateTarFiles(Path dir) noexcept
{
  std::vector<AllocatedPath> result;
  struct TarVisitor : public File::Visitor {
    std::vector<AllocatedPath> &out;
    explicit TarVisitor(std::vector<AllocatedPath> &o) noexcept : out(o) {}
    void Visit(Path full, Path) override {
      if (full.EndsWithIgnoreCase(".tar"))
        out.emplace_back(full);
    }
  } visitor(result);
  Directory::VisitFiles(dir, visitor, true);
  return result;
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
    env.SetText(_("Creating backup..."));

    AllocatedPath primary = GetPrimaryDataPath();
    if (primary == nullptr) {
      aborted = true;
      error_message = _("No primary data path.");
      return;
    }

    AllocatedPath dest = AllocatedPath::Build(target_device, tar_name.c_str());
    if (!CreateBackup(primary, dest, IsExcludedPath, env,
                      created_files, error_message)) {
      aborted = true;
      return;
    }
  }
};

// Restore job: extracts selected zip into primary data path (excludes patterns)
struct RestoreJob final : public Job {
  AllocatedPath zip_path;
  unsigned &restored_files;
  unsigned &failed_files;
  bool aborted{false};
  std::string error_message;

  RestoreJob(Path zip, unsigned &r, unsigned &f) noexcept
    : zip_path(zip), restored_files(r), failed_files(f) {}

  void Run(OperationEnvironment &env) override {
    env.SetText(_("Restoring backup..."));
    AllocatedPath primary = GetPrimaryDataPath();
    if (primary == nullptr) {
      aborted = true;
      error_message = _("No primary data path.");
      return;
    }

    if (!RestoreBackup(zip_path, primary, IsExcludedPath, env,
                       restored_files, failed_files, error_message)) {
      aborted = true;
      return;
    }
  }
};

static void
RunRestoreJob(Path chosen)
{
  int rr = ShowMessageBox(_("Restoring will overwrite existing data. Continue?"),
                          _("Restore"), MB_YESNO);
  if (rr != IDYES)
    return;

  unsigned restored = 0, failed = 0;
  RestoreJob job(chosen, restored, failed);
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

// Container widget showing current target only.
struct BackupContainer : public NullWidget {
  struct BackupItem {
    AllocatedPath path;
    std::string display_name;
    StaticString<32> size;
    StaticString<32> date;
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

  std::unique_ptr<WndProperty> target_property;
  std::unique_ptr<BackupListWidget> backup_list;
  PixelRect property_rect;
  PixelRect list_rect;
  AllocatedPath target_device_path;
  std::vector<BackupItem> available_backups;

  void CalculateLayout(const PixelRect &rc) noexcept {
    const unsigned prop_height = Layout::GetMinimumControlHeight();
    const unsigned spacing = Layout::GetTextPadding();

    property_rect = rc;
    property_rect.bottom = property_rect.top + prop_height;

    list_rect = rc;
    list_rect.top = property_rect.bottom + spacing;
    list_rect.bottom = rc.bottom;
  }

  void Initialise([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept override {
    const DialogLook &look = UIGlobals::GetDialogLook();
    if (!target_property)
      target_property = std::make_unique<WndProperty>(look);
    if (!backup_list)
      backup_list = std::make_unique<BackupListWidget>(available_backups);
  }

  void RefreshBackups() noexcept {
    available_backups.clear();

    if (target_device_path != nullptr) {
      for (const auto &path : EnumerateTarFiles(target_device_path)) {
        BackupItem item;
        item.path = Path(path);
        item.display_name = path.GetBase().c_str();

        char size_buf[32];
        FormatByteSize(size_buf, sizeof(size_buf), File::GetSize(path), false);
        item.size = size_buf;

        char date_buf[32];
        FormatISO8601(date_buf,
                      BrokenDateTime{File::GetLastModification(path)});
        item.date = date_buf;

        available_backups.emplace_back(std::move(item));
      }
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

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    CalculateLayout(rc);
    const DialogLook &look = UIGlobals::GetDialogLook();
    if (target_property)
      target_property->Create(parent, property_rect, _("Target"), 0, WindowStyle());
    else
      target_property = std::make_unique<WndProperty>(look);
    target_property->SetReadOnly(true);

    if (!backup_list)
      backup_list = std::make_unique<BackupListWidget>(available_backups);
    backup_list->Prepare(parent, list_rect);

    UpdateTargetCaption();
    RefreshBackups();
  }

  void Unprepare() noexcept override {
    if (backup_list)
      backup_list->Unprepare();
    backup_list.reset();
    target_property.reset();
    available_backups.clear();
  }

  bool Save([[maybe_unused]] bool &changed) noexcept override { return true; }
  bool Click() noexcept override {
    return backup_list != nullptr && backup_list->Click();
  }

  void ReClick() noexcept override {
    if (backup_list)
      backup_list->ReClick();
  }

  void Show([[maybe_unused]] const PixelRect &rc) noexcept override {
    if (target_property)
      target_property->MoveAndShow(property_rect);
    if (backup_list)
      backup_list->Show(list_rect);
  }

  void Hide() noexcept override {
    if (target_property) target_property->Hide();
    if (backup_list) backup_list->Hide();
  }

  void Move(const PixelRect &rc) noexcept override {
    CalculateLayout(rc);
    if (target_property && target_property->IsDefined() && target_property->IsVisible())
      target_property->Move(property_rect);
    if (backup_list)
      backup_list->Move(list_rect);
  }

  bool SetFocus() noexcept override {
    if (backup_list != nullptr) {
      backup_list->SetFocus();
      return true;
    }
    return false;
  }

  bool HasFocus() const noexcept override {
    return (backup_list != nullptr && backup_list->HasFocus()) ||
           (target_property != nullptr && target_property->HasFocus());
  }

  bool KeyPress(unsigned key_code) noexcept override {
    return backup_list != nullptr && backup_list->KeyPress(key_code);
  }

  bool Leave() noexcept override { return true; }

  void UpdateTargetCaption() noexcept {
    if (target_property == nullptr)
      return;
    if (target_device_path == nullptr) {
      target_property->SetText(_("(none)"));
      return;
    }
    target_property->SetText(target_device_path.c_str());
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

  // Prefer caller-provided target; otherwise fall back to last used target.
  {
    if (initial_target != nullptr) {
      container_ptr->target_device_path = Path(initial_target);
      container_ptr->UpdateTargetCaption();
    } else {
      const AllocatedPath &last = StorageLocationPickerDialog::GetLastTarget();
      if (last != nullptr) {
        container_ptr->target_device_path = Path(last);
        container_ptr->UpdateTargetCaption();
      }
    }
  }

  dialog.AddButton(_("Choose location"), [container_ptr]() {
    StorageLocationPickerDialog dlg(UIGlobals::GetDialogLook());
    AllocatedPath chosen = dlg.ShowModal();
    if (chosen == nullptr)
      return;
    StorageLocationPickerDialog::SetLastTarget(chosen);
    container_ptr->target_device_path = std::move(chosen);
    container_ptr->UpdateTargetCaption();
    container_ptr->RefreshBackups();
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
      ShowMessageBox(_("No backup files found in target."), _("Restore backup"),
                     MB_OK | MB_ICONINFORMATION);
      return;
    }

    RunRestoreJob(selected);
  });

  dialog.AddButton(_("Back"), mrCancel);

  dialog.FinishPreliminary(std::move(container));
  dialog.ShowModal();
}

static void
ShowRestoreDialogWithSource(const AllocatedPath &initial_source)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  AllocatedPath source;
  if (initial_source != nullptr)
    source = Path(initial_source);
  else {
    StorageLocationPickerDialog picker(look);
    source = picker.ShowModal();
    if (source == nullptr)
      return;
  }

  // Enumerate tar files in the selected source.
  const auto tar_files = EnumerateTarFiles(source);
  ComboList list;
  for (const auto &path : tar_files)
    list.Append(path.c_str(),
                path.GetBase().c_str());

  if (list.empty()) {
    ShowMessageBox(_("No backup files found in selected source."), _("Restore"), MB_OK | MB_ICONINFORMATION);
    return;
  }

  int idx = ComboPicker(_("Select backup file"), list, nullptr, false);
  if (idx < 0)
    return;

  const ComboList::Item &item = list[idx];
  AllocatedPath chosen(item.string_value.c_str());

  RunRestoreJob(chosen);
}

void ShowBackupManagerDialog()
{
  StorageLocationPickerDialog dlg(UIGlobals::GetDialogLook());
  AllocatedPath chosen = dlg.ShowModal();
  if (chosen == nullptr)
    return;

  StorageLocationPickerDialog::SetLastTarget(chosen);
  ShowBackupManagerDialogWithTarget(chosen);
}

void ShowRestoreDialog()
{
  ShowRestoreDialogWithSource(AllocatedPath());
}
