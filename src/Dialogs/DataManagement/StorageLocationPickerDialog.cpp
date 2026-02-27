// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StorageLocationPickerDialog.hpp"

#include "Dialogs/WidgetDialog.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "system/Path.hpp"
#include "Storage/PlatformStorageMonitor.hpp"
#include "Storage/StorageEvents.hpp"
#include "Storage/StorageManager.hpp"
#include "BackendComponents.hpp"
#include "Components.hpp"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Widget/ListWidget.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "system/FileUtil.hpp"
#include "Language/Language.hpp"
#include "Formatter/ByteSizeFormatter.hpp"

#include <optional>
#include <limits>
#include <string>
#include <vector>
#include <cstdint>

namespace {

struct DeviceEntry {
  std::string device_label; /* storage label if available (UTF-8) */
  AllocatedPath mount;      /* mount path (UTF-8) */
  std::string device_id;    /* stable id/path (e.g. /dev/sdb1 on Linux, D:\ on Windows) (UTF-8) */
  std::optional<StorageDevice::Space> space; /* optional space info */
};

std::string
FormatSpaceString(const std::optional<StorageDevice::Space> &space)
{
  std::string right_first;
  if (!space)
    return right_first;

  char free_buf[64];
  char total_buf[64];
  const uint64_t fv = space->free_bytes;
  const uint64_t tv = space->total_bytes;
  FormatByteSize(free_buf, sizeof(free_buf)/sizeof(free_buf[0]), fv, false);
  FormatByteSize(total_buf, sizeof(total_buf)/sizeof(total_buf[0]), tv, false);
  right_first = free_buf;
  right_first.append(" / ");
  right_first.append(total_buf);
  return right_first;
}

/* Forward declaration — defined further below in this TU. */
std::vector<DeviceEntry> DiscoverTargets() noexcept;

/**
 * Custom list widget for the storage location picker.
 *
 * Paints device entries via TwoTextRowsRenderer and registers itself
 * as a StorageEventListener so the list refreshes automatically when
 * the OS hotplug monitor detects a device arrival or removal —
 * no need for the user to press "Scan".
 */
class StorageListWidget final : public ListWidget,
                                public StorageEventListener {
  TwoTextRowsRenderer row_renderer_;
  std::vector<DeviceEntry> options_;
  WndForm *dialog_ = nullptr;
  Button *select_button_ = nullptr;

  void SyncSelectButton() noexcept {
    if (select_button_ != nullptr)
      select_button_->SetEnabled(!options_.empty());
  }

public:
  /** Must be called before ShowModal so double-click can close the dialog. */
  void SetDialog(WndForm &d) noexcept { dialog_ = &d; }

  /** Hook up the "Select" button so it can be enabled/disabled automatically. */
  void SetSelectButton(Button &b) noexcept { select_button_ = &b; }

  /** Public accessor so PickTarget can read the cursor without touching protected GetList(). */
  [[nodiscard]] unsigned GetCursorIndex() const noexcept {
    return GetList().GetCursorIndex();
  }

  [[nodiscard]] std::vector<DeviceEntry> &GetOptions() noexcept {
    return options_;
  }

  /** Re-enumerate storage devices and update the visible list in-place. */
  void Refresh() noexcept {
    options_ = DiscoverTargets();
    GetList().SetLength(options_.size());
    GetList().Invalidate();
    SyncSelectButton();
  }

  /* StorageEventListener — called on the UI thread by StorageManager */
  void OnStorageEvent(const StorageEventInfo &) noexcept override {
    Refresh();
  }

  /* Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    const auto &look = UIGlobals::GetDialogLook();
    const unsigned row_height =
      row_renderer_.CalculateLayout(look.text_font, look.small_font);
    options_ = DiscoverTargets();
    CreateList(parent, look, rc, row_height).SetLength(options_.size());
    SyncSelectButton();

    if (backend_components != nullptr &&
        backend_components->storage_manager != nullptr)
      backend_components->storage_manager->AddEventListener(*this);
  }

  void Unprepare() noexcept override {
    if (backend_components != nullptr &&
        backend_components->storage_manager != nullptr)
      backend_components->storage_manager->RemoveEventListener(*this);
    ListWidget::Unprepare();
  }

  /* ListControl::Handler */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override {
    if (idx >= options_.size())
      return;
    const auto &it = options_[idx];
    const std::string mount_text = it.mount.ToUTF8();

    const std::string first_text = it.device_label.empty()
      ? mount_text
      : it.device_label;

    std::string second_text;
    if (!it.device_id.empty() && it.device_id != first_text)
      second_text = it.device_id;
    else if (mount_text != first_text)
      second_text = mount_text;

    const auto right_first = FormatSpaceString(it.space);
    row_renderer_.DrawFirstRow(canvas, rc, first_text.c_str());
    if (!right_first.empty())
      row_renderer_.DrawRightFirstRow(canvas, rc, right_first.c_str());
    if (!second_text.empty())
      row_renderer_.DrawSecondRow(canvas, rc, second_text.c_str());
  }

  bool CanActivateItem([[maybe_unused]] unsigned) const noexcept override {
    return true;
  }

  void OnActivateItem([[maybe_unused]] unsigned) noexcept override {
    if (dialog_ != nullptr)
      dialog_->SetModalResult(mrOK);
  }
};

std::vector<DeviceEntry>
DiscoverTargets() noexcept
{
  std::vector<DeviceEntry> result;

  auto monitor = CreatePlatformStorageMonitor();
  if (monitor) {
    for (const auto &dev : monitor->Enumerate()) {
      if (dev == nullptr)
        continue;
      if (!dev->IsWritable())
        continue;

      const std::string dev_id = dev->Id();
      const std::string dev_name = dev->Label();
      const std::string mount = dev->Name();
      if (mount.empty())
        continue;

      DeviceEntry e;
      e.device_label = dev_name;
      e.mount = AllocatedPath(mount.c_str());
      e.device_id = dev_id;
      if (auto s = dev->GetSpace())
        e.space = *s;
      result.push_back(std::move(e));
    }
  }

  return result;
}
}

StorageLocationPickerDialog::StorageLocationPickerDialog(const DialogLook &look) noexcept
  : dialog_look(look) {}

AllocatedPath PickTarget(const DialogLook &look) noexcept;

AllocatedPath StorageLocationPickerDialog::last_target;

const AllocatedPath &StorageLocationPickerDialog::GetLastTarget() noexcept {
  return StorageLocationPickerDialog::last_target;
}

AllocatedPath
StorageLocationPickerDialog::GetDefaultTarget() noexcept {
  auto options = DiscoverTargets();
  if (!options.empty())
    return std::move(options[0].mount);
  return AllocatedPath();
}

void StorageLocationPickerDialog::SetLastTarget(const AllocatedPath &p) noexcept {
  StorageLocationPickerDialog::last_target = Path(p);
}

AllocatedPath
StorageLocationPickerDialog::ShowModal() noexcept {
  AllocatedPath chosen = PickTarget(dialog_look);
  if (chosen != nullptr)
    StorageLocationPickerDialog::last_target = Path(chosen);
  return chosen;
}

AllocatedPath
PickTarget(const DialogLook &look) noexcept {
  auto *list_widget = new StorageListWidget();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Choose location"));
  list_widget->SetDialog(dialog);

  auto *select_btn = dialog.AddButton(_("Select"), mrOK);
  list_widget->SetSelectButton(*select_btn);
  /* "Scan" is kept for manual refresh; hotplug also triggers Refresh(). */
  dialog.AddButton(_("Scan"), [list_widget]{ list_widget->Refresh(); });
  dialog.AddButton(_("Cancel"), mrCancel);
  dialog.EnableCursorSelection();
  dialog.FinishPreliminary(list_widget);

  const int result = dialog.ShowModal();

  if (result != mrOK)
    return AllocatedPath();

  const unsigned sel = list_widget->GetCursorIndex();
  auto &opts = list_widget->GetOptions();
  if (sel >= opts.size())
    return AllocatedPath();

  return std::move(opts[sel].mount);
}
